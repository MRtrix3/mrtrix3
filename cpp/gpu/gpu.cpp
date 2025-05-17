#include "gpu.h"
#include "exception.h"
#include "image_helpers.h"
#include "match.h"
#include "span.h"
#include "utils.h"
#include "wgslprocessing.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <webgpu/webgpu_cpp.h>

#include <array>
#include <cassert>
#include <cstring>
#include <limits>
#include <utility>
#include <vector>

namespace MR::GPU {

constexpr auto GPUBackendType
#ifdef __APPLE__
    = wgpu::BackendType::Metal;
#else
    = wgpu::BackendType::Vulkan;
#endif

namespace {
uint32_t pixelSizeInBytes(const wgpu::TextureFormat format) {
    switch(format) {
    case wgpu::TextureFormat::R8Unorm:
        return 1;
    case wgpu::TextureFormat::R16Float:
        return 2;
    case wgpu::TextureFormat::R32Float:
        return 4;
    default:
        throw MR::Exception("Only R8Unorm, R16Float and R32Float textures are supported!");
    }
}


wgpu::ShaderModule makeShaderModule(const std::string &name, const std::string &code, const wgpu::Device &device)
{
    wgpu::ShaderModuleWGSLDescriptor wgslDescriptor {};
    wgslDescriptor.code = code.c_str();
    wgpu::ShaderModuleDescriptor descriptor {};
    descriptor.nextInChain = &wgslDescriptor;
    descriptor.label = name.c_str();

    return device.CreateShaderModule(&descriptor);
}

wgpu::TextureFormat toWGPUFormat(const MR::GPU::TextureFormat& format) {
    switch(format) {
    case MR::GPU::TextureFormat::R32Float: return wgpu::TextureFormat::R32Float;
    default: wgpu::TextureFormat::Undefined;
    };
}

wgpu::TextureUsage toWGPUUsage(const MR::GPU::TextureUsage& usage) {
    wgpu::TextureUsage textureUsage = wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding;

    if(usage.storageBinding) {
        textureUsage |= wgpu::TextureUsage::StorageBinding;
    }
    if(usage.renderTarget) {
        textureUsage |= wgpu::TextureUsage::RenderAttachment;
    }
    return textureUsage;
}

}

ComputeContext::ComputeContext()
{
    constexpr std::array dawnToggles {
        "allow_unsafe_apis",
        "enable_immediate_error_handling"
    };

    wgpu::DawnTogglesDescriptor dawnTogglesDesc;
    dawnTogglesDesc.enabledToggles = dawnToggles.data();
    dawnTogglesDesc.enabledToggleCount = dawnToggles.size();

    constexpr wgpu::InstanceDescriptor instanceDescriptor {
        .nextInChain = nullptr,
        .capabilities = {.timedWaitAnyEnable = true}
    };
    const wgpu::Instance instance = wgpu::CreateInstance(&instanceDescriptor);
    wgpu::Adapter adapter;

    const wgpu::RequestAdapterOptions adapterOptions {
        .powerPreference = wgpu::PowerPreference::HighPerformance,
        .backendType = GPUBackendType
    };


    struct RequestAdapterResult {
        wgpu::RequestAdapterStatus status = wgpu::RequestAdapterStatus::Error;
        wgpu::Adapter adapter = nullptr;
        std::string message;
    } requestAdapterResult;

    const auto adapterCallback = [&requestAdapterResult](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message) {
        requestAdapterResult = { status, std::move(adapter), std::string(message) };
    };

    const wgpu::Future adapterRequest = instance.RequestAdapter(&adapterOptions,
                                                                wgpu::CallbackMode::WaitAnyOnly,
                                                                adapterCallback);
    const wgpu::WaitStatus waitStatus = instance.WaitAny(adapterRequest, -1);

    if(waitStatus == wgpu::WaitStatus::Success) {
        if(requestAdapterResult.status != wgpu::RequestAdapterStatus::Success) {
            throw MR::Exception("Failed to get adapter: " + requestAdapterResult.message);
        }
    } else {
        throw MR::Exception("Failed to get adapter: wgpu::Instance::WaitAny failed");
    }

    adapter = requestAdapterResult.adapter;

    const std::vector<wgpu::FeatureName> requiredDeviceFeatures = {
        wgpu::FeatureName::R8UnormStorage,
        wgpu::FeatureName::Float32Filterable
    };

    const wgpu::Limits requiredDeviceLimits {
        .maxComputeWorkgroupStorageSize = 32768,
        .maxComputeInvocationsPerWorkgroup = 1024,
    };

    wgpu::DeviceDescriptor deviceDescriptor {};
    deviceDescriptor.nextInChain = &dawnTogglesDesc;
    deviceDescriptor.requiredFeatures = requiredDeviceFeatures.data();
    deviceDescriptor.requiredFeatureCount = requiredDeviceFeatures.size();
    deviceDescriptor.requiredLimits = &requiredDeviceLimits;

    deviceDescriptor.SetDeviceLostCallback(
        wgpu::CallbackMode::AllowSpontaneous,
        [](const wgpu::Device&, wgpu::DeviceLostReason reason, wgpu::StringView message) {
            const char* reasonName = "";
            if(reason != wgpu::DeviceLostReason::Destroyed &&
                reason != wgpu::DeviceLostReason::InstanceDropped) {
                throw MR::Exception("GPU device lost: " + std::string(reasonName) + " : " + message.data);
            }
        });
    deviceDescriptor.SetUncapturedErrorCallback(
        [](const wgpu::Device&, wgpu::ErrorType type, wgpu::StringView message) {
            (void)type;
            FAIL("Uncaptured gpu error: " + std::string(message));
            throw MR::Exception("Uncaptured gpu error: " + std::string(message));
        });

    this->instance = instance;
    this->adapter = adapter;
    this->device = adapter.CreateDevice(&deviceDescriptor);
}

wgpu::Buffer ComputeContext::innerNewEmptyBuffer(size_t byteSize) const
{
    const wgpu::BufferDescriptor bufferDescriptor {
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::Storage,
        .size = byteSize,
    };

    return device.CreateBuffer(&bufferDescriptor);
}

wgpu::Buffer ComputeContext::innerNewBufferFromHostMemory(const void *srcMemory, size_t srcByteSize) const
{
    const auto buffer = innerNewEmptyBuffer(srcByteSize);
    innerWriteToBuffer(buffer, srcMemory, srcByteSize, 0);
    return buffer;
}

void ComputeContext::innerDownloadBuffer(const wgpu::Buffer &buffer, void *dstMemory, size_t dstByteSize) const
{
    assert(buffer.GetSize() == dstByteSize);

    const wgpu::BufferDescriptor stagingBufferDescriptor {
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead,
        .size = dstByteSize,
    };

    const wgpu::Buffer stagingBuffer = device.CreateBuffer(&stagingBufferDescriptor);
    const wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    encoder.CopyBufferToBuffer(buffer, 0, stagingBuffer, 0, dstByteSize);
    const wgpu::CommandBuffer commands = encoder.Finish();
    device.GetQueue().Submit(1, &commands);

    auto mappingCallback = [](wgpu::MapAsyncStatus status, const char* message) {
        if(status != wgpu::MapAsyncStatus::Success) {
            throw MR::Exception("Failed to map buffer: " + std::string(message));
        }
    };
    const wgpu::Future mappingFuture = stagingBuffer.MapAsync(wgpu::MapMode::Read,
                                                              0,
                                                              stagingBuffer.GetSize(),
                                                              wgpu::CallbackMode::WaitAnyOnly,
                                                              mappingCallback);
    const wgpu::WaitStatus waitStatus = instance.WaitAny(mappingFuture, std::numeric_limits<uint64_t>::max());
    if(waitStatus != wgpu::WaitStatus::Success) {
        throw MR::Exception("Failed to map buffer to host memory: wgpu::Instance::WaitAny failed");
    }

    const void* data = stagingBuffer.GetConstMappedRange();
    if(dstMemory != nullptr) {
        std::memcpy(dstMemory, data, dstByteSize);
        stagingBuffer.Unmap();
    } else {
        throw MR::Exception("Failed to map buffer to host memory: wgpu::Buffer::GetMappedRange returned nullptr");
    }
}

void ComputeContext::innerWriteToBuffer(const wgpu::Buffer &buffer, const void *data, size_t srcByteSize, uint64_t offset) const
{
    device.GetQueue().WriteBuffer(buffer, offset, data, srcByteSize);
}


Texture ComputeContext::newEmptyTexture(const TextureSpec &textureSpec) const
{
    const wgpu::TextureDescriptor wgpuTextureDesc {
        .usage = toWGPUUsage(textureSpec.usage),
        .dimension = textureSpec.depth > 1 ? wgpu::TextureDimension::e3D : wgpu::TextureDimension::e2D,
        .size = { textureSpec.width, textureSpec.height, textureSpec.depth },
        .format = toWGPUFormat(textureSpec.format)
    };
    return { device.CreateTexture(&wgpuTextureDesc), textureSpec };
}

Texture ComputeContext::newTextureFromHostMemory(const TextureSpec &textureSpec, tcb::span<const float> srcMemoryRegion) const
{
    const Texture texture = newEmptyTexture(textureSpec);
    const wgpu::TexelCopyTextureInfo imageCopyTexture { texture.wgpuHandle };
    const wgpu::TexelCopyBufferLayout textureDataLayout {
        .bytesPerRow = textureSpec.width * pixelSizeInBytes(texture.wgpuHandle.GetFormat()),
        .rowsPerImage = textureSpec.height,
    };

    const wgpu::Extent3D textureSize { textureSpec.width, textureSpec.height, textureSpec.depth };
    device.GetQueue().WriteTexture(&imageCopyTexture,
                                   srcMemoryRegion.data(),
                                   srcMemoryRegion.size_bytes(),
                                   &textureDataLayout,
                                   &textureSize);
    return texture;
}

Texture ComputeContext::newTextureFromHostImage(const MR::Image<float> &image, const TextureUsage &usage) const {
    const TextureSpec textureSpec = {
        .width = static_cast<uint32_t>(image.size(0)),
        .height = static_cast<uint32_t>(image.size(1)),
        .depth = static_cast<uint32_t>(image.size(2)),
        .usage = usage,
    };
    const auto imageSize = MR::voxel_count(image);
    return newTextureFromHostMemory(textureSpec, tcb::span<float>(image.address(), imageSize));
}

void ComputeContext::downloadTexture(const Texture &texture, tcb::span<float> dstMemoryRegion) const
{
    assert(dstMemoryRegion.size_bytes() >= static_cast<size_t>(texture.wgpuHandle.GetWidth()) *
                                               texture.wgpuHandle.GetHeight() *
                                               texture.wgpuHandle.GetDepthOrArrayLayers()
           && "Memory region size is too small for the texture");

    const uint32_t bytesPerRow = Utils::nextMultipleOf(texture.wgpuHandle.GetWidth() *
                                                           pixelSizeInBytes(texture.wgpuHandle.GetFormat()), 256);
    const size_t paddedDataSize = static_cast<size_t>(bytesPerRow) * texture.wgpuHandle.GetHeight() *
                                  texture.wgpuHandle.GetDepthOrArrayLayers();
    const wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    const wgpu::BufferDescriptor stagingBufferDesc {
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead,
        .size = paddedDataSize,
    };
    const wgpu::Buffer stagingBuffer = device.CreateBuffer(&stagingBufferDesc);

    const wgpu::TexelCopyTextureInfo imageCopyTexture { texture.wgpuHandle };
    const wgpu::TexelCopyBufferInfo imageCopyBuffer {
        .layout = wgpu::TexelCopyBufferLayout {
            .bytesPerRow = bytesPerRow,
            .rowsPerImage = texture.wgpuHandle.GetHeight(),
        },
        .buffer = stagingBuffer
    };

    const wgpu::Extent3D imageCopySize {
        .width = texture.wgpuHandle.GetWidth(),
        .height = texture.wgpuHandle.GetHeight(),
        .depthOrArrayLayers = texture.wgpuHandle.GetDepthOrArrayLayers(),
    };
    encoder.CopyTextureToBuffer(&imageCopyTexture, &imageCopyBuffer, &imageCopySize);
    const wgpu::CommandBuffer commands = encoder.Finish();
    device.GetQueue().Submit(1, &commands);

    auto mappingCallback = [](wgpu::MapAsyncStatus status, const char* message) {
        if(status != wgpu::MapAsyncStatus::Success) {
            throw MR::Exception("Failed to map buffer: " + std::string(message));
        }
    };

    const wgpu::Future mappingFuture = stagingBuffer.MapAsync(wgpu::MapMode::Read,
                                                              0,
                                                              stagingBuffer.GetSize(),
                                                              wgpu::CallbackMode::WaitAnyOnly,
                                                              mappingCallback);

    const wgpu::WaitStatus waitStatus = instance.WaitAny(mappingFuture, -1);
    if(waitStatus != wgpu::WaitStatus::Success) {
        throw MR::Exception("Failed to map buffer to host memory: wgpu::Instance::WaitAny failed");
    }

    const void* data = stagingBuffer.GetConstMappedRange();

    // Copy the unpadded data
    if(data != nullptr) {
        const size_t paddedRowWidth = bytesPerRow / sizeof(float);
        const size_t numRows = static_cast<size_t>(texture.wgpuHandle.GetDepthOrArrayLayers()) * texture.wgpuHandle.GetHeight();
        const tcb::span<const float> srcSpan(static_cast<const float*>(data), paddedRowWidth * numRows);
        const size_t width = texture.wgpuHandle.GetWidth();
        const tcb::span<float> dstSpan(dstMemoryRegion.data(), width * numRows);

        for (size_t row = 0; row < numRows; ++row) {
            const auto rowSrc = srcSpan.subspan(row * paddedRowWidth, width);
            auto rowDst = dstSpan.subspan(row * width, width);
            // copy exactly 'width' pixels
            std::copy_n(rowSrc.begin(), width, rowDst.begin());
        }
    } else {
        throw MR::Exception("Failed to map buffer to host memory: wgpu::Buffer::GetMappedRange returned nullptr");
    }
}

Kernel ComputeContext::newKernel(const KernelSpec &kernelSpec) const
{
    struct BindingEntries {
        std::vector<wgpu::BindGroupEntry> bindGroupEntries;
        std::vector<wgpu::BindGroupLayoutEntry> bindGroupLayoutEntries;

        void add(const wgpu::BindGroupEntry& bindGroupEntry,
                 const wgpu::BindGroupLayoutEntry& bindGroupLayoutEntry) {
            bindGroupEntries.push_back(bindGroupEntry);
            bindGroupLayoutEntries.push_back(bindGroupLayoutEntry);
        }
    };

    BindingEntries bindingEntries;


    uint32_t bindingIndex = 0;

    auto getBufferWGPUHandle = [](const BufferVariant& buffer) {
        return MR::match(buffer, [](auto &&arg) { return arg.wgpuHandle; });
    };
    // Uniform buffers
    for(const auto& buffer : kernelSpec.uniformBuffers) {
        const wgpu::BindGroupLayoutEntry layoutEntry {
            .binding = bindingIndex++,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = { .type = wgpu::BufferBindingType::Uniform }
        };

        const wgpu::BindGroupEntry bindGroupEntry {
            .binding = layoutEntry.binding,
            .buffer = getBufferWGPUHandle(buffer),
        };
        bindingEntries.add(bindGroupEntry, layoutEntry);
    }

    // Read-only buffers
    for(const auto& buffer : kernelSpec.readOnlyBuffers) {
        const wgpu::BindGroupLayoutEntry layoutEntry {
            .binding = bindingIndex++,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = { .type = wgpu::BufferBindingType::ReadOnlyStorage }
        };

        const wgpu::BindGroupEntry bindGroupEntry {
            .binding = layoutEntry.binding,
            .buffer = getBufferWGPUHandle(buffer),
        };
        bindingEntries.add(bindGroupEntry, layoutEntry);
    }

    // Read-write (storage) buffers
    for(const auto& buffer : kernelSpec.readWriteBuffers) {
        const wgpu::BindGroupLayoutEntry layoutEntry {
            .binding = bindingIndex++,
            .visibility = wgpu::ShaderStage::Compute,
            .buffer = { .type = wgpu::BufferBindingType::Storage }
        };

        const wgpu::BindGroupEntry bindGroupEntry {
            .binding = layoutEntry.binding,
            .buffer = getBufferWGPUHandle(buffer),
        };
        bindingEntries.add(bindGroupEntry, layoutEntry);
    }

    // Read-only textures
    for(const auto& texture : kernelSpec.readOnlyTextures) {
        const wgpu::BindGroupLayoutEntry layoutEntry {
            .binding = bindingIndex++,
            .visibility = wgpu::ShaderStage::Compute,
            .texture = {
                .sampleType = wgpu::TextureSampleType::Float,
                .viewDimension = texture.wgpuHandle.GetDepthOrArrayLayers() > 1
                                     ? wgpu::TextureViewDimension::e3D
                                     : wgpu::TextureViewDimension::e2D,
            }
        };

        const wgpu::BindGroupEntry bindGroupEntry {
            .binding = layoutEntry.binding,
            .textureView = texture.wgpuHandle.CreateView(),
        };
        bindingEntries.add(bindGroupEntry, layoutEntry);
    }

    // Write-only textures
    for(const auto& texture : kernelSpec.writeOnlyTextures) {
        const wgpu::BindGroupLayoutEntry layoutEntry {
            .binding = bindingIndex++,
            .visibility = wgpu::ShaderStage::Compute,
            .storageTexture = {
                .access = wgpu::StorageTextureAccess::WriteOnly,
                .format = texture.wgpuHandle.GetFormat(),
                .viewDimension = texture.wgpuHandle.GetDepthOrArrayLayers() > 1
                                     ? wgpu::TextureViewDimension::e3D
                                     : wgpu::TextureViewDimension::e2D
            }
        };

        const wgpu::BindGroupEntry bindGroupEntry {
            .binding = layoutEntry.binding,
            .textureView = texture.wgpuHandle.CreateView(),
        };
        bindingEntries.add(bindGroupEntry, layoutEntry);
    }

    // Samplers
    for(const auto& sampler : kernelSpec.samplers) {
        const wgpu::BindGroupLayoutEntry layoutEntry {
            .binding = bindingIndex++,
            .visibility = wgpu::ShaderStage::Compute,
            .sampler = { .type = wgpu::SamplerBindingType::Filtering }
        };

        const wgpu::BindGroupEntry bindGroupEntry {
            .binding = layoutEntry.binding,
            .sampler = sampler,
        };
        bindingEntries.add(bindGroupEntry, layoutEntry);
    }

    const auto layoutDescLabel = kernelSpec.computeShader.name + " layout descriptor";

    const wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc {
        .label = layoutDescLabel.c_str(),
        .entryCount = bindingEntries.bindGroupLayoutEntries.size(),
        .entries = bindingEntries.bindGroupLayoutEntries.data(),
    };

    const wgpu::BindGroupLayout bindGroupLayout = device.CreateBindGroupLayout(&bindGroupLayoutDesc);

    const wgpu::PipelineLayoutDescriptor pipelineLayoutDesc {
        .bindGroupLayoutCount = 1,
        .bindGroupLayouts = &bindGroupLayout,
    };
    const wgpu::PipelineLayout pipelineLayout = device.CreatePipelineLayout(&pipelineLayoutDesc);

    auto shaderPlaceHolders = kernelSpec.computeShader.placeholders;
    if(kernelSpec.computeShader.workgroupSize.has_value()) {
        shaderPlaceHolders["workgroup_size"] = std::to_string(kernelSpec.computeShader.workgroupSize->x) + ", " +
                                               std::to_string(kernelSpec.computeShader.workgroupSize->y) + ", " +
                                               std::to_string(kernelSpec.computeShader.workgroupSize->z);
    }
    const std::string shaderSource = [&](){
        return MR::match(kernelSpec.computeShader.shaderSource,
                         [&](const ShaderFile &shaderFile) {
                             return preprocessWGSLFile(shaderFile.filePath,
                                                       shaderPlaceHolders,
                                                       kernelSpec.computeShader.macros);
                         },
                         [&](const InlineShaderText &inlineString) {
                             return preprocessWGSLString(inlineString.text,
                                                       shaderPlaceHolders,
                                                       kernelSpec.computeShader.macros);
                         });
    }();

    const std::string computePipelineLabel = kernelSpec.computeShader.name + " compute pipeline";
    const wgpu::ComputePipelineDescriptor computePipelineDesc {
        .label = computePipelineLabel.c_str(),
        .layout = pipelineLayout,
        .compute = {
            .module = makeShaderModule(kernelSpec.computeShader.name, shaderSource, device),
            .entryPoint = kernelSpec.computeShader.entryPoint.c_str()
        }
    };

    const wgpu::BindGroupDescriptor bindGroupDesc {
        .layout = bindGroupLayout,
        .entryCount = bindingEntries.bindGroupEntries.size(),
        .entries = bindingEntries.bindGroupEntries.data(),
    };

    return Kernel {
        .name = kernelSpec.computeShader.name,
        .pipeline = device.CreateComputePipeline(&computePipelineDesc),
        .bindGroup = device.CreateBindGroup(&bindGroupDesc),
        .shaderSource = shaderSource
    };
}

void ComputeContext::dispatchKernel(const Kernel &kernel, const DispatchGrid &dispatchGrid) const
{
    const wgpu::ComputePassDescriptor passDesc {
        .label = kernel.name.c_str(),
    };
    const wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    const wgpu::ComputePassEncoder pass = encoder.BeginComputePass(&passDesc);
    pass.SetPipeline(kernel.pipeline);
    pass.SetBindGroup(0, kernel.bindGroup);
    pass.DispatchWorkgroups(dispatchGrid.wgCountX, dispatchGrid.wgCountY, dispatchGrid.wgCountZ);
    pass.End();

    const wgpu::CommandBuffer commands = encoder.Finish();
    device.GetQueue().Submit(1, &commands);
}

wgpu::Sampler ComputeContext::newLinearSampler() const
{
    const wgpu::SamplerDescriptor samplerDesc {
        .magFilter = wgpu::FilterMode::Linear,
        .minFilter = wgpu::FilterMode::Linear,
        .mipmapFilter = wgpu::MipmapFilterMode::Linear,
        .maxAnisotropy = 1
    };

    return device.CreateSampler(&samplerDesc);
}


}
