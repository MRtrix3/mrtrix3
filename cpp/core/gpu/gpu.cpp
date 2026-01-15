/* Copyright (c) 2008-2025 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "gpu.h"
#include "exception.h"
#include "image_helpers.h"
#include "match_variant.h"
#include "platform.h"
#include "slangcodegen.h"

#include <tcb/span.hpp>

#include <cstdlib>
#include <memory>
#include <slang-com-ptr.h>
#include <slang.h>
#include <webgpu/webgpu_cpp.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <future>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace MR::GPU {

struct SlangSessionInfo {
  Slang::ComPtr<slang::IGlobalSession> globalSession;
  Slang::ComPtr<slang::ISession> session;
};

constexpr auto GPUBackendType
#ifdef __APPLE__
    = wgpu::BackendType::Metal;
#else
    = wgpu::BackendType::Vulkan;
#endif

namespace {

uint32_t next_multiple_of(const uint32_t value, const uint32_t multiple) {
  if (value > std::numeric_limits<uint32_t>::max() - multiple) {
    return std::numeric_limits<uint32_t>::max();
  }
  return (value + multiple - 1) / multiple * multiple;
}

uint32_t pixel_size_in_bytes(const TextureFormat format) {
  switch (format) {
  case TextureFormat::R32Float:
    return 4;
  case TextureFormat::RGBA32Float:
    return 16;
  default:
    throw MR::Exception("Only R32Float and RGBA32Float textures are supported!");
  }
}

wgpu::ShaderModule make_wgsl_shader_module(std::string_view name, std::string_view code, const wgpu::Device &device) {
  wgpu::ShaderSourceWGSL wgsl;
  wgsl.code = code;

  const wgpu::ShaderModuleDescriptor shader_module_descriptor{
      .nextInChain = &wgsl,
      .label = name,
  };

  return device.CreateShaderModule(&shader_module_descriptor);
}

wgpu::ShaderModule
make_spirv_shader_module(std::string_view name, tcb::span<const uint32_t> spirvCode, const wgpu::Device &device) {
  wgpu::ShaderSourceSPIRV spirv;
  spirv.codeSize = spirvCode.size_bytes();
  spirv.code = spirvCode.data();

  const wgpu::ShaderModuleDescriptor shader_module_descriptor{
      .nextInChain = &spirv,
      .label = name,
  };

  return device.CreateShaderModule(&shader_module_descriptor);
}

wgpu::TextureFormat to_wgpu_format(const MR::GPU::TextureFormat &format) {
  switch (format) {
  case MR::GPU::TextureFormat::R32Float:
    return wgpu::TextureFormat::R32Float;
  case MR::GPU::TextureFormat::RGBA32Float:
    return wgpu::TextureFormat::RGBA32Float;
  default:
    return wgpu::TextureFormat::Undefined;
  };
}

wgpu::TextureUsage to_wgpu_usage(const MR::GPU::TextureUsage &usage) {
  wgpu::TextureUsage textureUsage =
      wgpu::TextureUsage::CopySrc | wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding;

  if (usage.storage_binding) {
    textureUsage |= wgpu::TextureUsage::StorageBinding;
  }
  if (usage.render_target) {
    textureUsage |= wgpu::TextureUsage::RenderAttachment;
  }
  return textureUsage;
}

std::future<Slang::ComPtr<slang::IGlobalSession>> request_slang_global_session_async() {
  return SlangCodegen::request_slang_global_session_async();
}
} // namespace

ComputeContext::ComputeContext() : m_slang_session_info(std::make_unique<SlangSessionInfo>()) {
  // We request the creation of the slang global session asynchronously
  // as it can take some time to complete. This allows the WebGPU instance
  // and adapter to be created in parallel with the Slang global session.
  auto slang_global_session_request = request_slang_global_session_async();

  {
    std::vector dawn_toggles{"allow_unsafe_apis", "enable_immediate_error_handling", "disable_robustness"};

    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    const char *dawn_gpu_debug_env = std::getenv("MRTRIX_GPU_DEBUG_TRACE"); // check_syntax off
    if (dawn_gpu_debug_env != nullptr && std::string(dawn_gpu_debug_env) == "1") {
      dawn_toggles.emplace_back("dump_shaders");
      dawn_toggles.emplace_back("disable_symbol_renaming");
    }

    wgpu::DawnTogglesDescriptor dawn_toggles_desc;
    dawn_toggles_desc.enabledToggles = dawn_toggles.data();
    dawn_toggles_desc.enabledToggleCount = dawn_toggles.size();

    constexpr std::array<wgpu::InstanceFeatureName, 1> instance_features = {wgpu::InstanceFeatureName::TimedWaitAny};
    const wgpu::InstanceDescriptor instance_descriptor{
        .nextInChain = nullptr,
        .requiredFeatureCount = instance_features.size(),
        .requiredFeatures = instance_features.data(),
    };

    const wgpu::Instance wgpu_instance = wgpu::CreateInstance(&instance_descriptor);
    wgpu::Adapter wgpu_adapter;

    const wgpu::RequestAdapterOptions adapter_options{.powerPreference = wgpu::PowerPreference::HighPerformance,
                                                      .backendType = GPUBackendType};

    struct RequestAdapterResult {
      wgpu::RequestAdapterStatus status = wgpu::RequestAdapterStatus::Error;
      wgpu::Adapter adapter = nullptr;
      std::string message;
    } request_adapter_result;

    const auto adapter_callback = [&request_adapter_result](wgpu::RequestAdapterStatus status,
                                                            wgpu::Adapter foundAdapter,
                                                            wgpu::StringView message) {
      request_adapter_result = {status, std::move(foundAdapter), std::string(message)};
    };

    const wgpu::Future adapter_request =
        wgpu_instance.RequestAdapter(&adapter_options, wgpu::CallbackMode::WaitAnyOnly, adapter_callback);
    const wgpu::WaitStatus wait_status = wgpu_instance.WaitAny(adapter_request, -1);

    if (wait_status == wgpu::WaitStatus::Success) {
      if (request_adapter_result.status != wgpu::RequestAdapterStatus::Success) {
        throw MR::Exception("Failed to get adapter: " + request_adapter_result.message);
      }
    } else {
      throw MR::Exception("Failed to get adapter: wgpu::Instance::WaitAny failed");
    }

    wgpu_adapter = request_adapter_result.adapter;
    const std::vector<wgpu::FeatureName> required_device_features = {
        wgpu::FeatureName::R8UnormStorage, wgpu::FeatureName::Float32Filterable, wgpu::FeatureName::Subgroups};

    wgpu::Limits supported_limits;
    wgpu_adapter.GetLimits(&supported_limits);

    const uint64_t desired_max_storage_buffer_binding_size = 1'073'741'824ULL; // 1 GiB
    const uint64_t desired_max_buffer_size = 1'073'741'824ULL;                 // 1 GiB

    const wgpu::Limits required_device_limits{
        .maxStorageTexturesPerShaderStage = 8,
        .maxStorageBufferBindingSize =
            std::min(desired_max_storage_buffer_binding_size, supported_limits.maxStorageBufferBindingSize),
        .maxBufferSize = std::min(desired_max_buffer_size, supported_limits.maxBufferSize),
        .maxComputeWorkgroupStorageSize = 32768,
        .maxComputeInvocationsPerWorkgroup = 1024,
        .maxComputeWorkgroupSizeX = 1024,
    };

    wgpu::DeviceDescriptor device_descriptor{};
    device_descriptor.nextInChain = &dawn_toggles_desc;
    device_descriptor.requiredFeatures = required_device_features.data();
    device_descriptor.requiredFeatureCount = required_device_features.size();
    device_descriptor.requiredLimits = &required_device_limits;

    device_descriptor.SetDeviceLostCallback(
        wgpu::CallbackMode::AllowSpontaneous,
        [](const wgpu::Device &, wgpu::DeviceLostReason reason, wgpu::StringView message) {
          if (reason != wgpu::DeviceLostReason::Destroyed) {
            throw MR::Exception(std::string("GPU device lost: ") + message.data);
          }
        });
    device_descriptor.SetUncapturedErrorCallback(
        [](const wgpu::Device &, wgpu::ErrorType type, wgpu::StringView message) {
          (void)type;
          FAIL("Uncaptured gpu error: " + std::string(message));
          throw MR::Exception("Uncaptured gpu error: " + std::string(message));
        });

    m_instance = wgpu_instance;
    m_adapter = wgpu_adapter;
    m_device = wgpu_adapter.CreateDevice(&device_descriptor);
    wgpu::AdapterInfo adapter_info;
    wgpu_adapter.GetInfo(&adapter_info);

    m_device_info = DeviceInfo{.subgroup_min_size = adapter_info.subgroupMinSize};
  }
  m_slang_session_info->globalSession = std::move(slang_global_session_request.get());

  const slang::TargetDesc target_desc{.format = SLANG_WGSL};

  const auto executable_path = MR::Platform::get_executable_path();
  const std::string executable_dir_string = (std::filesystem::path(executable_path).parent_path() / "shaders").string();
  const char *executable_dir_cstr = executable_dir_string.c_str(); // check_syntax off

  std::vector<slang::CompilerOptionEntry> slang_compiler_options;
  {
    const slang::CompilerOptionEntry uniformity_analysis_option = {
        .name = slang::CompilerOptionName::ValidateUniformity,
        .value = {.intValue0 = 1},
    };
    slang_compiler_options.push_back(uniformity_analysis_option);
  }

  const slang::SessionDesc slang_session_desc{
      .targets = &target_desc,
      .targetCount = 1,
      .defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
      .searchPaths = &executable_dir_cstr,
      .searchPathCount = 1,
      .compilerOptionEntries = slang_compiler_options.data(),
  };

  {
    const SlangResult slang_res = m_slang_session_info->globalSession->createSession(
        slang_session_desc, m_slang_session_info->session.writeRef());
    if (SLANG_FAILED(slang_res)) {
      throw MR::Exception("Failed to create Slang session!");
    }
  }
}

ComputeContext &ComputeContext::operator=(ComputeContext &&) noexcept = default;
ComputeContext::ComputeContext(ComputeContext &&) noexcept = default;
ComputeContext::~ComputeContext() = default;

std::future<ComputeContext> ComputeContext::request_async() {
  return std::async(std::launch::async, []() { return ComputeContext(); });
}

void ComputeContext::copy_buffer_to_buffer(const BufferVariant &srcBuffer,
                                           const BufferVariant &dstBuffer,
                                           const BufferCopyInfo &info) const {
  const auto extract_handle = [](const BufferVariant &buffer) -> wgpu::Buffer {
    return MR::match_v(buffer, [](auto &&arg) { return arg.wgpu_handle; });
  };

  const wgpu::Buffer src_handle = extract_handle(srcBuffer);
  const wgpu::Buffer dst_handle = extract_handle(dstBuffer);

  assert(dst_handle.GetUsage() & wgpu::BufferUsage::CopyDst &&
         "Destination buffer must have CopyDst usage for copyBufferToBuffer");

  const uint64_t src_size = src_handle.GetSize();
  const uint64_t dst_size = dst_handle.GetSize();

  uint64_t final_byte_size = info.byteSize;
  // If byteSize == 0, copy as much as possible from src->dst given offsets
  if (info.byteSize == 0) {
    if (info.srcOffset >= src_size || info.dstOffset >= dst_size) {
      // Nothing to copy
      return;
    }
    final_byte_size = static_cast<size_t>(std::min<uint64_t>(src_size - info.srcOffset, dst_size - info.dstOffset));
  }

  if (info.srcOffset + final_byte_size > src_size) {
    throw MR::Exception("copyBufferToBuffer: source range out of bounds");
  }
  if (info.dstOffset + final_byte_size > dst_size) {
    throw MR::Exception("copyBufferToBuffer: destination range out of bounds");
  }

  if (final_byte_size == 0) {
    return;
  }

  const wgpu::CommandEncoder command_encoder = m_device.CreateCommandEncoder();
  command_encoder.CopyBufferToBuffer(
      src_handle, static_cast<uint64_t>(info.srcOffset), dst_handle, info.dstOffset, final_byte_size);
  const wgpu::CommandBuffer command_buffer = command_encoder.Finish();
  m_device.GetQueue().Submit(1, &command_buffer);
}

wgpu::Buffer ComputeContext::inner_new_empty_buffer(size_t byteSize, BufferType bufferType) const {
  wgpu::BufferUsage buffer_usage = wgpu::BufferUsage::None;
  size_t buffer_byte_size = byteSize;

  switch (bufferType) {
  case BufferType::StorageBuffer:
    buffer_usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::Storage;
    break;
  case BufferType::UniformBuffer:
    buffer_usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::Uniform;
    // Align buffer size to 16 bytes for uniform buffers
    buffer_byte_size = next_multiple_of(byteSize, 16);
    break;
  };

  const wgpu::BufferDescriptor buffer_descriptor{
      .usage = buffer_usage,
      .size = buffer_byte_size,
  };

  return m_device.CreateBuffer(&buffer_descriptor);
}

wgpu::Buffer ComputeContext::inner_new_buffer_from_host_memory(const void *srcMemory,
                                                               size_t srcByteSize,
                                                               BufferType bufferType) const {
  const auto buffer = inner_new_empty_buffer(srcByteSize, bufferType);
  inner_write_to_buffer(buffer, srcMemory, srcByteSize, 0);
  return buffer;
}

void ComputeContext::inner_download_buffer(const wgpu::Buffer &buffer, void *dstMemory, size_t dstByteSize) const {
  assert(buffer.GetSize() == dstByteSize);
  assert(dstByteSize % 4 == 0 && "Destination buffer size must be a multiple of 4 bytes");
  const wgpu::BufferDescriptor staging_buffer_descriptor{
      .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead,
      .size = dstByteSize,
  };

  const wgpu::Buffer staging_buffer = m_device.CreateBuffer(&staging_buffer_descriptor);
  const wgpu::CommandEncoder command_encoder = m_device.CreateCommandEncoder();
  command_encoder.CopyBufferToBuffer(buffer, 0, staging_buffer, 0, dstByteSize);
  const wgpu::CommandBuffer command_buffer = command_encoder.Finish();
  m_device.GetQueue().Submit(1, &command_buffer);

  auto mapping_callback = [](wgpu::MapAsyncStatus status, wgpu::StringView message) {
    if (status != wgpu::MapAsyncStatus::Success) {
      throw MR::Exception("Failed to map buffer: " + std::string(message));
    }
  };
  const wgpu::Future mapping_future = staging_buffer.MapAsync(
      wgpu::MapMode::Read, 0, staging_buffer.GetSize(), wgpu::CallbackMode::WaitAnyOnly, mapping_callback);
  const wgpu::WaitStatus wait_status = m_instance.WaitAny(mapping_future, std::numeric_limits<uint64_t>::max());
  if (wait_status != wgpu::WaitStatus::Success) {
    throw MR::Exception("Failed to map buffer to host memory: wgpu::Instance::WaitAny failed");
  }

  const void *mapped_data = staging_buffer.GetConstMappedRange();
  if (dstMemory != nullptr) {
    std::memcpy(dstMemory, mapped_data, dstByteSize);
    staging_buffer.Unmap();
  } else {
    throw MR::Exception("Failed to map buffer to host memory: wgpu::Buffer::GetMappedRange returned nullptr");
  }
}

void ComputeContext::inner_write_to_buffer(const wgpu::Buffer &buffer,
                                           const void *data,
                                           size_t srcByteSize,
                                           uint64_t offset) const {
  if (buffer.GetUsage() & wgpu::BufferUsage::Uniform) {
    // TODO: fix
    if (offset != 0) {
      throw MR::Exception("Cannot write to a uniform buffer with non-zero offset");
    }
    const size_t original_size = srcByteSize;
    const size_t padded_size = next_multiple_of(original_size, 16);
    std::vector<std::byte> padded_data(padded_size);
    std::memcpy(padded_data.data(), data, original_size);
    m_device.GetQueue().WriteBuffer(buffer, 0, padded_data.data(), padded_size);
  } else {
    m_device.GetQueue().WriteBuffer(buffer, offset, data, srcByteSize);
  }
}

void ComputeContext::inner_clear_buffer(const wgpu::Buffer &buffer) const {
  const wgpu::CommandEncoder command_encoder = m_device.CreateCommandEncoder();
  command_encoder.ClearBuffer(buffer);
  const wgpu::CommandBuffer command_buffer = command_encoder.Finish();
  m_device.GetQueue().Submit(1, &command_buffer);
}

Texture ComputeContext::new_empty_texture(const TextureSpec &textureSpec) const {
  const wgpu::TextureDescriptor wgpu_texture_desc{.usage = to_wgpu_usage(textureSpec.usage),
                                                  .dimension = textureSpec.depth > 1 ? wgpu::TextureDimension::e3D
                                                                                     : wgpu::TextureDimension::e2D,
                                                  .size = {textureSpec.width, textureSpec.height, textureSpec.depth},
                                                  .format = to_wgpu_format(textureSpec.format)};
  return {textureSpec, m_device.CreateTexture(&wgpu_texture_desc)};
}

Texture ComputeContext::new_texture_from_host_memory(const TextureSpec &texture_desc,
                                                     tcb::span<const float> srcMemoryRegion) const {
  const Texture texture = new_empty_texture(texture_desc);
  const wgpu::TexelCopyTextureInfo image_copy_texture{.texture = texture.wgpu_handle};
  const wgpu::TexelCopyBufferLayout texture_data_layout{
      .bytesPerRow = texture_desc.width * pixel_size_in_bytes(texture_desc.format),
      .rowsPerImage = texture_desc.height,
  };

  const wgpu::Extent3D texture_size{texture_desc.width, texture_desc.height, texture_desc.depth};
  m_device.GetQueue().WriteTexture(
      &image_copy_texture, srcMemoryRegion.data(), srcMemoryRegion.size_bytes(), &texture_data_layout, &texture_size);
  return texture;
}

Texture ComputeContext::new_texture_from_host_image(const MR::Image<float> &image, const TextureUsage &usage) const {
  const TextureSpec textureSpec = {
      .width = static_cast<uint32_t>(image.size(0)),
      .height = static_cast<uint32_t>(image.size(1)),
      .depth = static_cast<uint32_t>(image.size(2)),
      .usage = usage,
  };
  const auto image_size = MR::voxel_count(image);
  return new_texture_from_host_memory(textureSpec, tcb::span<float>(image.address(), image_size));
}

void ComputeContext::download_texture(const Texture &texture, tcb::span<float> dst_memory_region) const {
  const uint32_t components_per_texel = pixel_size_in_bytes(texture.spec.format) / sizeof(float);
  assert(dst_memory_region.size() >= static_cast<size_t>(texture.wgpu_handle.GetWidth()) *
                                         texture.wgpu_handle.GetHeight() * texture.wgpu_handle.GetDepthOrArrayLayers() *
                                         components_per_texel &&
         "Memory region size is too small for the texture");

  const uint32_t bytes_per_row =
      next_multiple_of(texture.wgpu_handle.GetWidth() * pixel_size_in_bytes(texture.spec.format), 256);
  const size_t padded_data_size = static_cast<size_t>(bytes_per_row) * texture.wgpu_handle.GetHeight() *
                                  texture.wgpu_handle.GetDepthOrArrayLayers();
  const wgpu::CommandEncoder command_encoder = m_device.CreateCommandEncoder();
  const wgpu::BufferDescriptor staging_buffer_desc{
      .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead,
      .size = padded_data_size,
  };
  const wgpu::Buffer staging_buffer = m_device.CreateBuffer(&staging_buffer_desc);

  const wgpu::TexelCopyTextureInfo image_copy_texture{.texture = texture.wgpu_handle};
  const wgpu::TexelCopyBufferInfo image_copy_buffer{.layout =
                                                        wgpu::TexelCopyBufferLayout{
                                                            .bytesPerRow = bytes_per_row,
                                                            .rowsPerImage = texture.wgpu_handle.GetHeight(),
                                                        },
                                                    .buffer = staging_buffer};

  const wgpu::Extent3D image_copy_size{
      .width = texture.wgpu_handle.GetWidth(),
      .height = texture.wgpu_handle.GetHeight(),
      .depthOrArrayLayers = texture.wgpu_handle.GetDepthOrArrayLayers(),
  };
  command_encoder.CopyTextureToBuffer(&image_copy_texture, &image_copy_buffer, &image_copy_size);
  const wgpu::CommandBuffer command_buffer = command_encoder.Finish();
  m_device.GetQueue().Submit(1, &command_buffer);

  auto mapping_callback = [](wgpu::MapAsyncStatus status, wgpu::StringView message) {
    if (status != wgpu::MapAsyncStatus::Success) {
      throw MR::Exception("Failed to map buffer: " + std::string(message));
    }
  };

  const wgpu::Future mapping_future = staging_buffer.MapAsync(
      wgpu::MapMode::Read, 0, staging_buffer.GetSize(), wgpu::CallbackMode::WaitAnyOnly, mapping_callback);

  const wgpu::WaitStatus wait_status = m_instance.WaitAny(mapping_future, -1);
  if (wait_status != wgpu::WaitStatus::Success) {
    throw MR::Exception("Failed to map buffer to host memory: wgpu::Instance::WaitAny failed");
  }

  const void *mapped_data = staging_buffer.GetConstMappedRange();

  // Copy the unpadded data
  if (mapped_data != nullptr) {
    // This is amount of data we will copy per row
    const size_t texture_width_in_floats = static_cast<size_t>(texture.wgpu_handle.GetWidth()) * components_per_texel;
    // The full distance in floats from the start of one row to the start of the next in the source GPU buffer
    const size_t padded_row_width_in_floats = bytes_per_row / sizeof(float);
    const size_t num_rows =
        static_cast<size_t>(texture.wgpu_handle.GetDepthOrArrayLayers()) * texture.wgpu_handle.GetHeight();

    const tcb::span<const float> src_span(static_cast<const float *>(mapped_data),
                                          padded_row_width_in_floats * num_rows);
    const tcb::span<float> dst_span(dst_memory_region.data(), texture_width_in_floats * num_rows);

    for (size_t row = 0; row < num_rows; ++row) {
      const auto row_src = src_span.subspan(row * padded_row_width_in_floats, texture_width_in_floats);
      auto row_dst = dst_span.subspan(row * texture_width_in_floats, texture_width_in_floats);
      // copy exactly 'textureWidthInFloats' texels
      std::copy_n(row_src.begin(), texture_width_in_floats, row_dst.begin());
    }
  } else {
    throw MR::Exception("Failed to map buffer to host memory: wgpu::Buffer::GetMappedRange returned nullptr");
  }

  staging_buffer.Unmap();
}

Kernel ComputeContext::new_kernel(const KernelSpec &kernel_spec) const {
  struct BindingEntries {
    std::vector<wgpu::BindGroupEntry> bind_group_entries;
    std::vector<wgpu::BindGroupLayoutEntry> bind_group_layout_entries;

    void add(const wgpu::BindGroupEntry &bindGroupEntry, const wgpu::BindGroupLayoutEntry &bindGroupLayoutEntry) {
      bind_group_entries.push_back(bindGroupEntry);
      bind_group_layout_entries.push_back(bindGroupLayoutEntry);
    }
  };
  BindingEntries binding_entries;

  const auto &slang_session = m_slang_session_info->session;
  const auto &[wgsl_shader_code, linked_slang_program] =
      SlangCodegen::compile_kernel_code_to_wgsl(kernel_spec, slang_session.get(), m_shader_cache);

  const auto reflected_bindings_map = SlangCodegen::reflect_bindings(linked_slang_program->getLayout());

  const auto reflected_wg_size = SlangCodegen::workgroup_size(linked_slang_program->getLayout());

  for (const auto &[name, resource] : kernel_spec.bindings_map) {
    auto it = reflected_bindings_map.find(name);
    if (it == reflected_bindings_map.end()) {
      throw MR::Exception("Slang reflection failed to find binding: " + name + " in " +
                          kernel_spec.compute_shader.name + " with entry point " +
                          kernel_spec.compute_shader.entryPoint);
    }

    const auto &binding_info = it->second;
    const uint32_t binding_index = binding_info.binding_index;
    auto *type_layout = binding_info.layout->getTypeLayout();

    const auto access = type_layout->getResourceAccess();
    MR::match_v(
        resource,
        // we can't capture structure bindings till C++20
        [&, name = name](const BufferVariant &buffer) {
          DEBUG("Buffer binding: " + name);
          auto binding_kind = type_layout->getKind();
          wgpu::BufferBindingType buffer_binding_type = wgpu::BufferBindingType::Undefined;
          if (binding_kind == slang::TypeReflection::Kind::ConstantBuffer) {
            buffer_binding_type = wgpu::BufferBindingType::Uniform;
          } else if (binding_kind == slang::TypeReflection::Kind::Resource ||
                     binding_kind == slang::TypeReflection::Kind::ShaderStorageBuffer) {
            switch (access) {
            case SLANG_RESOURCE_ACCESS_READ:
              buffer_binding_type = wgpu::BufferBindingType::ReadOnlyStorage;
              break;
            case SLANG_RESOURCE_ACCESS_READ_WRITE:
              buffer_binding_type = wgpu::BufferBindingType::Storage;
              break;
            default:
              throw MR::Exception("Unsupported buffer access type for '" + name + "'");
            }
          } else {
            throw MR::Exception("Cannot determine WGPU buffer binding type for '" + name +
                                "'. Its Slang type kind is not a recognized buffer type.");
          }
          const wgpu::BindGroupLayoutEntry layout_entry{.binding = binding_index,
                                                        .visibility = wgpu::ShaderStage::Compute,
                                                        .buffer = {.type = buffer_binding_type}};
          const wgpu::BindGroupEntry bind_group_entry{
              .binding = layout_entry.binding,
              .buffer = MR::match_v(buffer, [](auto &&arg) { return arg.wgpu_handle; })};
          binding_entries.add(bind_group_entry, layout_entry);
        },
        [&, name = name](const Texture &texture) {
          wgpu::BindGroupLayoutEntry layout_entry;
          if (access == SLANG_RESOURCE_ACCESS_READ) {
            layout_entry = {.binding = binding_index,
                            .visibility = wgpu::ShaderStage::Compute,
                            .texture = {
                                .sampleType = wgpu::TextureSampleType::Float,
                                .viewDimension = texture.wgpu_handle.GetDepthOrArrayLayers() > 1
                                                     ? wgpu::TextureViewDimension::e3D
                                                     : wgpu::TextureViewDimension::e2D,
                            }};
          } else if (access == SLANG_RESOURCE_ACCESS_WRITE || access == SLANG_RESOURCE_ACCESS_READ_WRITE) {
            layout_entry = {.binding = binding_index,
                            .visibility = wgpu::ShaderStage::Compute,
                            .storageTexture = {.access = access == SLANG_RESOURCE_ACCESS_WRITE
                                                             ? wgpu::StorageTextureAccess::WriteOnly
                                                             : wgpu::StorageTextureAccess::ReadWrite,
                                               .format = texture.wgpu_handle.GetFormat(),
                                               .viewDimension = texture.wgpu_handle.GetDepthOrArrayLayers() > 1
                                                                    ? wgpu::TextureViewDimension::e3D
                                                                    : wgpu::TextureViewDimension::e2D}};
          } else {
            throw MR::Exception("Unsupported texture access type for '" + name + "'");
          }
          const wgpu::BindGroupEntry bind_group_entry{
              .binding = layout_entry.binding,
              .textureView = texture.wgpu_handle.CreateView(),
          };
          binding_entries.add(bind_group_entry, layout_entry);
        },
        [&](const Sampler &sampler) {
          const wgpu::BindGroupLayoutEntry layout_entry{
              .binding = binding_index,
              .visibility = wgpu::ShaderStage::Compute,
              .sampler = {.type = wgpu::SamplerBindingType::Filtering},
          };

          const wgpu::BindGroupEntry bind_group_entry{
              .binding = layout_entry.binding,
              .sampler = sampler.wgpu_handle,
          };
          binding_entries.add(bind_group_entry, layout_entry);
        });
  }

  const auto layout_desc_label = kernel_spec.compute_shader.name + " layout descriptor";

  const wgpu::BindGroupLayoutDescriptor bind_group_layout_desc{
      .label = layout_desc_label.c_str(),
      .entryCount = binding_entries.bind_group_layout_entries.size(),
      .entries = binding_entries.bind_group_layout_entries.data(),
  };

  const wgpu::BindGroupLayout bind_group_layout = m_device.CreateBindGroupLayout(&bind_group_layout_desc);

  const wgpu::PipelineLayoutDescriptor pipeline_layout_desc{
      .bindGroupLayoutCount = 1,
      .bindGroupLayouts = &bind_group_layout,
  };
  const wgpu::PipelineLayout pipeline_layout = m_device.CreatePipelineLayout(&pipeline_layout_desc);

  const std::string compute_pipeline_label = kernel_spec.compute_shader.name + " compute pipeline";
  const wgpu::ComputePipelineDescriptor compute_pipeline_desc{
      .label = compute_pipeline_label.c_str(),
      .layout = pipeline_layout,
      .compute = {.module = make_wgsl_shader_module(kernel_spec.compute_shader.name, wgsl_shader_code, m_device),
                  .entryPoint = kernel_spec.compute_shader.entryPoint.c_str()}};

  const wgpu::BindGroupDescriptor bind_group_desc{
      .layout = bind_group_layout,
      .entryCount = binding_entries.bind_group_entries.size(),
      .entries = binding_entries.bind_group_entries.data(),
  };

  const WorkgroupSize wg_size = {.x = reflected_wg_size[0], .y = reflected_wg_size[1], .z = reflected_wg_size[2]};

  return Kernel{.name = kernel_spec.compute_shader.name,
                .pipeline = m_device.CreateComputePipeline(&compute_pipeline_desc),
                .bind_group = m_device.CreateBindGroup(&bind_group_desc),
                .shader_source = wgsl_shader_code,
                .workgroup_size = wg_size};
}
void ComputeContext::dispatch_kernel(const Kernel &kernel, const DispatchGrid &dispatch_grid) const {
  const wgpu::ComputePassDescriptor pass_desc{
      .label = kernel.name.c_str(),
  };
  const wgpu::CommandEncoder command_encoder = m_device.CreateCommandEncoder();
  const wgpu::ComputePassEncoder compute_pass = command_encoder.BeginComputePass(&pass_desc);
  compute_pass.SetPipeline(kernel.pipeline);
  compute_pass.SetBindGroup(0, kernel.bind_group);
  compute_pass.DispatchWorkgroups(dispatch_grid.x, dispatch_grid.y, dispatch_grid.z);
  compute_pass.End();

  const wgpu::CommandBuffer command_buffer = command_encoder.Finish();
  m_device.GetQueue().Submit(1, &command_buffer);
}

Sampler ComputeContext::new_linear_sampler() const {
  const wgpu::SamplerDescriptor sampler_desc{.magFilter = wgpu::FilterMode::Linear,
                                             .minFilter = wgpu::FilterMode::Linear,
                                             .mipmapFilter = wgpu::MipmapFilterMode::Linear,
                                             .maxAnisotropy = 1};

  return {Sampler::FilterMode::Linear, m_device.CreateSampler(&sampler_desc)};
}

} // namespace MR::GPU
