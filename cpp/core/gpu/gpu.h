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

#pragma once

#include "image.h"
#include "match_variant.h"
#include "shadercache.h"

#include <tcb/span.hpp>
#include <webgpu/webgpu_cpp.h>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <future>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace MR::GPU {

enum class BufferType : uint8_t { StorageBuffer, UniformBuffer };

template <typename T> struct Buffer {
  BufferType buffer_type = BufferType::StorageBuffer;
  wgpu::Buffer wgpu_handle{};

  uint64_t elementsCount() const {
    assert(wgpu_handle.GetSize() % sizeof(T) == 0);
    return wgpu_handle.GetSize() / sizeof(T);
  }
  uint64_t bytesSize() const { return wgpu_handle.GetSize(); }

  using element_t = std::remove_cv_t<std::remove_reference_t<T>>;
  static_assert(std::is_same_v<element_t, float> || std::is_same_v<element_t, int32_t> ||
                    std::is_same_v<element_t, uint32_t> || std::is_same_v<element_t, std::byte>,
                "Buffer<T> supports float, int32_t, uint32_t, std::byte.");
};

using BufferVariant = std::variant<Buffer<float>, Buffer<int32_t>, Buffer<uint32_t>, Buffer<std::byte>>;

struct TextureUsage {
  bool storage_binding = false;
  bool render_target = false;
};

enum class TextureFormat : uint8_t { R32Float, RGBA32Float };

struct TextureSpec {
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t depth = 1;
  TextureFormat format = TextureFormat::R32Float;
  TextureUsage usage;
};

struct Texture {
  TextureSpec spec;
  wgpu::Texture wgpu_handle;
};

struct Sampler {
  enum class FilterMode : uint8_t { Nearest, Linear };
  FilterMode filter_mode = FilterMode::Linear;
  wgpu::Sampler wgpu_handle;
};

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
  uint32_t x = 1;
  uint32_t y = 1;
  uint32_t z = 1;

  uint32_t workgroup_count() const { return x * y * z; }

  // Given `workgroup_size`, the returned grid contains the number
  // of workgroups per dimension so that at most one thread is dispatched
  // per logical element (i.e. an injective, element-wise dispatch so that each
  // element is processed by a single thread).
  static DispatchGrid element_wise(const std::array<size_t, 3> &data_dimensions, const WorkgroupSize &workgroup_size) {
    assert(workgroup_size.x > 0 && workgroup_size.y > 0 && workgroup_size.z > 0);
    return {static_cast<uint32_t>((data_dimensions[0] + workgroup_size.x - 1) / workgroup_size.x),
            static_cast<uint32_t>((data_dimensions[1] + workgroup_size.y - 1) / workgroup_size.y),
            static_cast<uint32_t>((data_dimensions[2] + workgroup_size.z - 1) / workgroup_size.z)};
  }

  // Convenience function for 3D textures.
  static DispatchGrid element_wise_texture(const Texture &texture, const WorkgroupSize &workgroup_size) {
    return element_wise({texture.spec.width, texture.spec.height, texture.spec.depth}, workgroup_size);
  }
};

// Absolute/relative (to working dir) path  of a WGSL file.
struct ShaderFile {
  std::filesystem::path file_path;
};

struct InlineShaderText {
  std::string text;
};

using ShaderSource = std::variant<ShaderFile, InlineShaderText>;

using ShaderConstantValue = std::variant<int32_t, uint32_t, float, bool>;

struct ShaderEntry {
  ShaderSource shader_source;

  std::string entryPoint = "main";

  std::string name = MR::match_v(
      shader_source,
      [](const ShaderFile &file) { return file.file_path.stem().string(); },
      [](const InlineShaderText &) { return std::string("inline_shader"); });

  // Convenience property to set the kWorkgroupSizeX, kWorkgroupSizeY, and
  // kWorkgroupSizeZ constants in the shader. These constant must be declared
  // as extern static const in the shader code.
  std::optional<WorkgroupSize> workgroup_size;

  using ShaderConstantMap = std::unordered_map<std::string, ShaderConstantValue>;
  // Link time constants to specialise the shader module.
  // To use a constant in the shader code, declare it as extern static const.
  ShaderConstantMap constants;
  // Generic specialisation arguments for the shader entry point.
  std::vector<std::string> entry_point_args;
};

using ShaderBindingResource = std::variant<BufferVariant, Texture, Sampler>;

using ShaderBindingsMap = std::unordered_map<std::string, ShaderBindingResource>;

struct KernelSpec {
  ShaderEntry compute_shader;
  ShaderBindingsMap bindings_map;
};

struct Kernel {
  std::string name;
  wgpu::ComputePipeline pipeline;
  wgpu::BindGroup bind_group;
  // For debugging purposes, the shader source code is stored here.
  std::string shader_source;
  WorkgroupSize workgroup_size;
};

struct SlangSessionInfo;

struct ComputeContext {
  explicit ComputeContext();
  ComputeContext(const ComputeContext &) = delete;
  ComputeContext &operator=(const ComputeContext &) = delete;
  ComputeContext(ComputeContext &&) noexcept;
  ComputeContext &operator=(ComputeContext &&) noexcept;
  ~ComputeContext();

  [[nodiscard]] static std::future<ComputeContext> request_async();

  // NOTE: For all buffer creation and write operations, it's safe to discard
  // the original data on the host side after the operation is complete as
  // the data is internally copied to a staging buffer by Dawn's implementation.
  template <typename T = float>
  [[nodiscard]] Buffer<T> new_empty_buffer(size_t size, BufferType buffer_type = BufferType::StorageBuffer) const {
    return {buffer_type, inner_new_empty_buffer(size * sizeof(T), buffer_type)};
  }

  template <typename T = float>
  [[nodiscard]] Buffer<T> new_buffer_from_host_memory(std::initializer_list<T> srcMemory,
                                                      BufferType bufferType = BufferType::StorageBuffer) const {
    return new_buffer_from_host_memory(tcb::span<const T>(srcMemory), bufferType);
  }

  template <typename T = float>
  [[nodiscard]] Buffer<T> new_buffer_from_host_memory(tcb::span<const T> src_memory,
                                                      BufferType buffer_type = BufferType::StorageBuffer) const {
    return {buffer_type, inner_new_buffer_from_host_memory(src_memory.data(), src_memory.size_bytes(), buffer_type)};
  }

  template <typename T = float>
  [[nodiscard]] Buffer<T> new_buffer_from_host_memory(const void *src_memory,
                                                      size_t byte_size,
                                                      BufferType buffer_type = BufferType::StorageBuffer) const {
    return {buffer_type, inner_new_buffer_from_host_memory(src_memory, byte_size, buffer_type)};
  }

  template <typename T = float>
  [[nodiscard]] Buffer<T> new_buffer_from_host_memory(const std::vector<tcb::span<const T>> &src_memory_regions,
                                                      BufferType bufferType = BufferType::StorageBuffer) const {
    size_t totalBytes = 0;
    for (const auto &region : src_memory_regions)
      totalBytes += region.size_bytes();

    auto buffer = inner_new_empty_buffer(totalBytes, bufferType);
    uint64_t offset = 0;
    for (const auto &region : src_memory_regions) {
      inner_write_to_buffer(buffer, region.data(), region.size_bytes(), offset);
      offset += region.size_bytes();
    }
    return Buffer<T>{bufferType, std::move(buffer)};
  }

  // This function blocks until the download is complete.
  template <typename T = float> [[nodiscard]] std::vector<T> download_buffer_as_vector(const Buffer<T> &buffer) const {
    std::vector<T> result(buffer.wgpu_handle.GetSize() / sizeof(T));
    download_buffer(buffer, result.data(), result.size() * sizeof(T));
    return result;
  }

  // This function blocks until the download is complete.
  template <typename T = float> void download_buffer(const Buffer<T> &buffer, tcb::span<T> dst_memory_region) const {
    download_buffer(buffer, dst_memory_region.data(), dst_memory_region.size_bytes());
  }

  // This function blocks until the download is complete.
  template <typename T = float> void download_buffer(const Buffer<T> &buffer, void *data, size_t dst_byte_size) const {
    inner_download_buffer(buffer.wgpu_handle, data, dst_byte_size);
  }

  // Writes to the buffer at the specified offset.
  template <typename T = float>
  void write_to_buffer(const Buffer<T> &buffer, tcb::span<const T> src_memory_region, uint64_t offset = 0) const {
    write_to_buffer(buffer, src_memory_region.data(), src_memory_region.size_bytes(), offset * sizeof(T));
  }

  template <typename T = float>
  void write_to_buffer(const Buffer<T> &buffer, const void *data, size_t size, uint64_t bytesOffset = 0) const {
    inner_write_to_buffer(buffer.wgpu_handle, data, size, bytesOffset);
  }

  // Copy bytes from a source buffer to a destination buffer.
  // if byteSize is 0, the whole source buffer is copied.
  struct BufferCopyInfo {
    uint64_t srcOffset = 0;
    uint64_t dstOffset = 0;
    uint64_t byteSize = 0;
  };
  void copy_buffer_to_buffer(const BufferVariant &srcBuffer,
                             const BufferVariant &dstBuffer,
                             const BufferCopyInfo &info) const;

  template <typename T = float> void clear_buffer(const Buffer<T> &buffer) const {
    inner_clear_buffer(buffer.wgpu_handle);
  }

  [[nodiscard]] Texture new_empty_texture(const TextureSpec &textureSpec) const;

  [[nodiscard]] Texture new_texture_from_host_memory(const TextureSpec &texture_desc,
                                                     tcb::span<const float> src_memory_region) const;

  [[nodiscard]] Texture new_texture_from_host_image(const MR::Image<float> &image,
                                                    const TextureUsage &usage = {}) const;

  Buffer<float> new_buffer_from_host_image(const MR::Image<float> &image,
                                           BufferType bufferType = BufferType::StorageBuffer) const;

  // This function blocks until the download is complete.
  void download_texture(const Texture &texture, tcb::span<float> dst_memory_region) const;

  [[nodiscard]] Kernel new_kernel(const KernelSpec &kernel_spec) const;

  void dispatch_kernel(const Kernel &kernel, const DispatchGrid &dispatch_grid) const;

  [[nodiscard]] Sampler new_linear_sampler() const;

private:
  wgpu::Buffer inner_new_empty_buffer(size_t byteSize, BufferType bufferType = BufferType::StorageBuffer) const;
  wgpu::Buffer inner_new_buffer_from_host_memory(const void *srcMemory,
                                                 size_t srcByteSize,
                                                 BufferType bufferType = BufferType::StorageBuffer) const;
  void inner_download_buffer(const wgpu::Buffer &buffer, void *dstMemory, size_t dstByteSize) const;
  void inner_write_to_buffer(const wgpu::Buffer &buffer, const void *data, size_t srcByteSize, uint64_t offset) const;
  void inner_clear_buffer(const wgpu::Buffer &buffer) const;

  wgpu::Instance m_instance;
  wgpu::Adapter m_adapter;
  wgpu::Device m_device;

  struct DeviceInfo {
    uint32_t subgroup_min_size = 0;
  };

  DeviceInfo m_device_info;

  std::unique_ptr<SlangSessionInfo> m_slang_session_info;

  // Cache of Slang-compiled WGSL shaders
  mutable ShaderCache m_shader_cache;
};
} // namespace MR::GPU
