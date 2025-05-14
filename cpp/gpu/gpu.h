#pragma once

#include "image.h"
#include "match.h"
#include "span.h"
#include <unordered_set>
#include <utility>
#include <webgpu/webgpu_cpp.h>

#include <cassert>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace MR::GPU {
// A workgroup is a collection of threads that execute the same kernel
// function in parallel. Each thread within a workgroup can cooperate with others
// through shared memory.
struct WorkgroupSize {
    uint32_t x = 1;
    uint32_t y = 1;
    uint32_t z = 1;

    // As a rule of thumb, for optimal performance across different hardware, the
    // total number of threads in a workgroup should be a multiple of 64.
    uint32_t threadCount() const { return x * y * z; }
};

// The dispatch grid defines the number of workgroups to be dispatched for a
// kernel. The total number of threads dispatched is the product of the number of
// workgroups in each dimension and the number of threads per workgroup.
struct DispatchGrid {
    // Number of workgroups for each dimension.
    uint32_t wgCountX = 1;
    uint32_t wgCountY = 1;
    uint32_t wgCountZ = 1;

    uint32_t workgroupCount() const { return wgCountX * wgCountY * wgCountZ; }
};

// Absolute/relative (to working dir) path  of a WGSL file.
struct ShaderFile { std::filesystem::path filePath; };

struct InlineShaderText { std::string text; };

using ShaderSource = std::variant<ShaderFile, InlineShaderText>;

struct ShaderEntry {
    ShaderSource shaderSource;

    std::string entryPoint = "main";

    std::string name = MR::match(shaderSource,
        [](const ShaderFile& file) { return file.filePath.stem().string(); },
        [](const InlineShaderText&){ return std::string("inline_shader"); });

    // Convenience property to set the {{workgroup_size}} placeholder.
    // Only relevant for compute shaders.
    std::optional<WorkgroupSize> workgroupSize;

    // Map of placeholders to their values. The values will be replaced in the shader source code.
    // Placeholders must be in the format {{placeholder_name}}.
    std::unordered_map<std::string, std::string> placeholders;

    // Set of macro definitions to be defined in the shader.
    std::unordered_set<std::string> macros;
};


template<typename T>
struct Buffer {
    wgpu::Buffer wgpuHandle;

    static_assert(
        std::is_same_v<T, float> || std::is_same_v<T, int32_t> ||
        std::is_same_v<T, uint32_t>,
        "GPU::Buffer<T> only supports float, int32_t or uint32_t"
    );
};

using BufferVariant = std::variant<Buffer<float>,
                                   Buffer<int32_t>,
                                   Buffer<uint32_t>>;

struct TextureUsage {
    bool storageBinding = false;
    bool renderTarget = false;
};

enum class TextureFormat : uint8_t {
    R32Float,
};

struct TextureSpec {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;
    TextureFormat format = TextureFormat::R32Float;
    TextureUsage usage;
};

struct Texture {
    wgpu::Texture wgpuHandle;
    TextureSpec spec;
};

struct KernelSpec {
    // NOTE: The order in the shader must match the lists below:
    // 1. Uniform buffers
    // 2. Read-only buffers
    // 3. Read-write buffers
    // 4. Read-only textures
    // 5. Write-only textures
    // 6. Samplers
    // List order must also match the shader's binding points.
    ShaderEntry computeShader;
    std::vector<BufferVariant> uniformBuffers;
    std::vector<BufferVariant> readOnlyBuffers;
    std::vector<BufferVariant> readWriteBuffers;
    std::vector<Texture> readOnlyTextures;
    std::vector<Texture> writeOnlyTextures;
    std::vector<wgpu::Sampler> samplers;
};

struct Kernel {
    std::string name;
    wgpu::ComputePipeline pipeline;
    wgpu::BindGroup bindGroup;
    // For debugging purposes, the shader source code is stored here.
    std::string shaderSource;
};

struct ComputeContext {
    explicit ComputeContext();

    template<typename T>
    Buffer<T> newEmptyBuffer(size_t size) const {
        return { innerNewEmptyBuffer(size * sizeof(T)) };
    }

    template<typename T>
    Buffer<T> newBufferFromHostMemory(tcb::span<const T> srcMemory) const {
        return { innerNewBufferFromHostMemory(srcMemory.data(), srcMemory.size_bytes()) };
    }

    template<typename T>
    Buffer<T> newBufferFromHostMemory(const void* srcMemory, size_t byteSize) const {
        return { innerNewBufferFromHostMemory(srcMemory, byteSize) };
    }

    template<typename T>
    Buffer<T> newBufferFromHostMemory(const std::vector<tcb::span<const T>> &srcMemoryRegions) const {
        size_t totalBytes = 0;
        for (const auto& region : srcMemoryRegions) totalBytes += region.size_bytes();

        auto buffer = innerNewEmptyBuffer(totalBytes);
        size_t offset = 0;
        for (const auto& region : srcMemoryRegions) {
            innerWriteToBuffer(buffer, region.data(), region.size_bytes(), uint32_t(offset));
            offset += region.size_bytes();
        }
        return Buffer<T>{ std::move(buffer) };
    }

    // This function blocks until the download is complete.
    template<typename T>
    void downloadBuffer(const Buffer<T>& buffer, tcb::span<T> dstMemoryRegion) const {
        return downloadBuffer(buffer, dstMemoryRegion.data(), dstMemoryRegion.size_bytes());
    }

    // This function blocks until the download is complete.
    template<typename T>
    void downloadBuffer(const Buffer<T>& buffer, void* data, size_t dstByteSize) const {
        return innerDownloadBuffer(buffer.wgpuHandle, data, dstByteSize);
    }

    template<typename T>
    void writeToBuffer(const Buffer<T>& buffer, tcb::span<T> dstMemoryRegion, uint32_t offset = 0) const {
        return writeToBuffer(buffer, dstMemoryRegion.data(), dstMemoryRegion.size_bytes(), offset);
    }

    template<typename T>
    void writeToBuffer(const Buffer<T>& buffer, const void* data, size_t size, uint32_t bytesOffset = 0) const {
        return innerWriteToBuffer(buffer.wgpuHandle, data, size, bytesOffset);
    }

    Texture newEmptyTexture(const TextureSpec& textureSpec) const;

    Texture newTextureFromHostMemory(const TextureSpec& textureDesc,
                                     tcb::span<const float> srcMemoryRegion) const;

    Texture newTextureFromHostImage(const MR::Image<float>& image, const TextureUsage& usage = {}) const;

    // This function blocks until the download is complete.
    void downloadTexture(const Texture& texture, tcb::span<float> dstMemoryRegion) const;

    Kernel newKernel(const KernelSpec& kernelSpec) const;

    void dispatchKernel(const Kernel& kernel, const DispatchGrid& dispatchGrid) const;

    wgpu::Sampler newLinearSampler() const;

private:
    wgpu::Buffer innerNewEmptyBuffer(size_t byteSize) const;
    wgpu::Buffer innerNewBufferFromHostMemory(const void* srcMemory, size_t srcByteSize) const;
    void innerDownloadBuffer(const wgpu::Buffer& buffer, void* dstMemory, size_t dstByteSize) const;
    void innerWriteToBuffer(const wgpu::Buffer& buffer, const void* data, size_t size, uint32_t dstByteSize) const;

    wgpu::Instance instance;
    wgpu::Adapter adapter;
    wgpu::Device device;
};
}
