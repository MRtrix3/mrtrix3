#include <gtest/gtest.h>

#include "exception.h"
#include "gpu/gpu.h"
#include "span.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

using namespace MR;
using namespace MR::GPU;

class GPUTest : public ::testing::Test {
protected:
  // Static pointer to the single, shared context for all tests in this suite.
  inline static std::unique_ptr<MR::GPU::ComputeContext> shared_context;

  // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
  MR::GPU::ComputeContext &context;

  GPUTest() : context(*shared_context) {}

  static void SetUpTestSuite() {
    ASSERT_NO_THROW(shared_context = std::make_unique<MR::GPU::ComputeContext>(););
    ASSERT_NE(shared_context, nullptr) << "Failed to create shared GPU context.";
  }

  static void TearDownTestSuite() { shared_context.reset(); }
};

TEST_F(GPUTest, MakeEmptyBuffer) {
  const size_t buffer_element_count = 1024;
  const Buffer<uint32_t> buffer = context.new_empty_buffer<uint32_t>(buffer_element_count);

  std::vector<uint32_t> downloaded_data(buffer_element_count, 1); // Initialize with non-zero

  context.download_buffer<uint32_t>(buffer, downloaded_data);

  for (auto val : downloaded_data) {
    EXPECT_EQ(val, 0);
  }
}

TEST_F(GPUTest, BufferFromHostMemory) {
  std::vector<int32_t> host_data = {1, 2, 3, 4, 5};

  const Buffer<int32_t> buffer = context.new_buffer_from_host_memory<int32_t>(tcb::span<const int32_t>(host_data));

  std::vector<int32_t> downloaded_data(host_data.size(), 0);
  context.download_buffer<int32_t>(buffer, downloaded_data);

  EXPECT_EQ(downloaded_data, host_data);
}

TEST_F(GPUTest, BufferFromHostMemoryVoidPtr) {
  std::vector<float> host_data = {1.0F, 2.5F, -3.0F};
  const Buffer<float> buffer =
      context.new_buffer_from_host_memory<float>(host_data.data(), host_data.size() * sizeof(float));

  std::vector<float> downloaded_data(host_data.size());
  context.download_buffer<float>(buffer, downloaded_data);
  EXPECT_EQ(downloaded_data, host_data);
}

TEST_F(GPUTest, BufferFromHostMemoryMultipleRegions) {
  std::vector<uint32_t> region1 = {1, 2, 3};
  std::vector<uint32_t> region2 = {4, 5};
  std::vector<uint32_t> region3 = {6, 7, 8, 9};

  const std::vector<tcb::span<const uint32_t>> regions = {region1, region2, region3};
  const Buffer<uint32_t> buffer = context.new_buffer_from_host_memory<uint32_t>(regions);

  const std::vector<uint32_t> expected_data = {1, 2, 3, 4, 5, 6, 7, 8, 9};
  std::vector<uint32_t> downloaded_data(expected_data.size());
  context.download_buffer<uint32_t>(buffer, downloaded_data);

  EXPECT_EQ(downloaded_data, expected_data);
}

TEST_F(GPUTest, WriteToBuffer) {
  std::vector<float> new_data = {0.1F, 0.2F, 0.3F, 0.4F};

  const Buffer<float> buffer = context.new_empty_buffer<float>(new_data.size());
  std::vector<float> downloaded_data(new_data.size(), 0.0F);

  context.write_to_buffer<float>(buffer, new_data);
  context.download_buffer<float>(buffer, downloaded_data);

  for (size_t i = 0; i < new_data.size(); i++) {
    EXPECT_FLOAT_EQ(downloaded_data[i], new_data[i]);
  }
}

TEST_F(GPUTest, WriteToBufferWithOffset) {
  const size_t buffer_size = 10;
  std::vector<float> initial_data(buffer_size);
  std::iota(initial_data.begin(), initial_data.end(), 0.0F); // 0, 1, ..., 9

  const Buffer<float> buffer = context.new_buffer_from_host_memory<float>(initial_data);

  std::vector<float> new_data = {100.0F, 101.0F, 102.0F};
  const uint64_t offset_size = static_cast<uint64_t>(new_data.size());

  context.write_to_buffer<float>(buffer, new_data, offset_size);

  std::vector<float> downloaded_data(buffer_size);
  context.download_buffer<float>(buffer, downloaded_data);

  std::vector<float> expected_data = {0.0F, 1.0F, 2.0F, 100.0F, 101.0F, 102.0F, 6.0F, 7.0F, 8.0F, 9.0F};
  for (size_t i = 0; i < buffer_size; ++i) {
    EXPECT_FLOAT_EQ(downloaded_data[i], expected_data[i]);
  }
}

TEST_F(GPUTest, EmptyTexture) {
  const MR::GPU::TextureSpec textureSpec = {
      .width = 4,
      .height = 4,
      .depth = 1,
      .format = TextureFormat::R32Float,
  };

  const auto texture = context.new_empty_texture(textureSpec);

  const uint32_t bytes_per_pixel = 4; // R32Float
  const size_t downloaded_size_bytes =
      static_cast<size_t>(textureSpec.width) * textureSpec.height * textureSpec.depth * bytes_per_pixel;
  std::vector<float> downloaded_data(downloaded_size_bytes / sizeof(float), 1.0F); // Init with non-zero

  context.download_texture(texture, downloaded_data);

  for (uint32_t z = 0; z < textureSpec.depth; ++z) {
    for (uint32_t y = 0; y < textureSpec.height; ++y) {
      for (uint32_t x = 0; x < textureSpec.width; ++x) {
        const size_t idx = ((z * textureSpec.height + y) * textureSpec.width) + x;
        EXPECT_FLOAT_EQ(downloaded_data[idx], 0.0F);
      }
    }
  }
}

TEST_F(GPUTest, KernelWithInlineShader) {
  const std::string shaderCode = R"slang(
        [shader("compute")]
        [numthreads(1, 1, 1)]
        void main(
            uint32_t3 id : SV_DispatchThreadID,
            RWStructuredBuffer<float> data
        ){
            let idx = id.x;
            uint element_count, stride;
            data.GetDimensions(element_count, stride);
            if (idx < element_count) {
                data[idx] = data[idx] * 3.0;
            }
        }
    )slang";

  const std::vector<float> host_data = {1.0F, 2.0F, 3.0F, 4.0F};
  const std::vector<float> expected_data = {3.0F, 6.0F, 9.0F, 12.0F};
  Buffer<float> buffer = context.new_buffer_from_host_memory<float>(host_data);
  const KernelSpec kernel_spec{
      .compute_shader =
          {
              .shader_source = InlineShaderText{shaderCode},
          },
      .bindings_map = {{"data", buffer}},
  };
  const Kernel kernel = context.new_kernel(kernel_spec);
  const DispatchGrid dispatch_grid = {static_cast<uint32_t>((host_data.size() + 63)), 1, 1};
  context.dispatch_kernel(kernel, dispatch_grid);

  std::vector<float> result_data(host_data.size());
  context.download_buffer<float>(buffer, result_data);
  EXPECT_EQ(result_data, expected_data);
}

TEST_F(GPUTest, ShaderConstants) {
  const std::string shaderCode = R"slang(
    extern const static uint32_t uConstantValue;
    extern const static int32_t iConstantValue;
    extern const static float fConstantValue;

    [shader("compute")]
    [numthreads(1, 1, 1)]
    void main(
        uint32_t id : SV_DispatchThreadID,
        RWStructuredBuffer<float> floatBuffer,
        RWStructuredBuffer<uint32_t> uintBuffer,
        RWStructuredBuffer<int32_t> intBuffer
    ){
        floatBuffer[0] = fConstantValue;
        uintBuffer[0] = uConstantValue;
        intBuffer[0] = iConstantValue;
    })slang";

  const float f_constant_value = 3.14F;
  const uint32_t u_constant_value = 42;
  const int32_t i_constant_value = -7;

  Buffer<float> float_buffer = context.new_empty_buffer<float>(1);
  Buffer<uint32_t> uint_buffer = context.new_empty_buffer<uint32_t>(1);
  Buffer<int32_t> int_buffer = context.new_empty_buffer<int32_t>(1);

  const KernelSpec kernel_spec{
      .compute_shader = {.shader_source = InlineShaderText{shaderCode},
                         .constants = {{"fConstantValue", f_constant_value},
                                       {"uConstantValue", u_constant_value},
                                       {"iConstantValue", i_constant_value}}},
      .bindings_map = {{"floatBuffer", float_buffer}, {"uintBuffer", uint_buffer}, {"intBuffer", int_buffer}},
  };

  const Kernel kernel = context.new_kernel(kernel_spec);
  const DispatchGrid dispatch_grid = {1, 1, 1};
  context.dispatch_kernel(kernel, dispatch_grid);
  float downloaded_float = 0.0F;
  uint32_t downloaded_uint = 0;
  int32_t downloaded_int = 0;

  context.download_buffer<float>(float_buffer, &downloaded_float, sizeof(float));
  context.download_buffer<uint32_t>(uint_buffer, &downloaded_uint, sizeof(uint32_t));
  context.download_buffer<int32_t>(int_buffer, &downloaded_int, sizeof(int32_t));

  EXPECT_FLOAT_EQ(downloaded_float, f_constant_value);
  EXPECT_EQ(downloaded_uint, u_constant_value);
  EXPECT_EQ(downloaded_int, i_constant_value);
}

TEST_F(GPUTest, ShaderEntryPointArgs) {
  const std::string shaderCode = R"slang(
        interface IOperation {
            float execute(float a, float b);
        }

        struct Add : IOperation {
            float execute(float a, float b) { return a + b; }
        }

        struct Multiply : IOperation {
            float execute(float a, float b) { return a * b; }
        }

        [shader("compute")]
        [numthreads(1, 1, 1)]
        void main<Op : IOperation>(
            uint32_t3 id : SV_DispatchThreadID,
            RWStructuredBuffer<float> data
        ){
            let idx = id.x;
            let op = Op();
            data[idx] = op.execute(data[idx], 2.0);
        }
    )slang";

  // Test Add
  {
    const std::vector<float> host_data = {1.0F, 2.0F, 3.0F};
    const std::vector<float> expected_data = {3.0F, 4.0F, 5.0F}; // + 2.0
    Buffer<float> buffer = context.new_buffer_from_host_memory<float>(host_data);

    const KernelSpec kernel_spec{
        .compute_shader = {.shader_source = InlineShaderText{shaderCode}, .entry_point_args = {"Add"}},
        .bindings_map = {{"data", buffer}},
    };
    const Kernel kernel = context.new_kernel(kernel_spec);
    const DispatchGrid dispatch_grid = {static_cast<uint32_t>(host_data.size()), 1, 1};
    context.dispatch_kernel(kernel, dispatch_grid);

    std::vector<float> result_data(host_data.size());
    context.download_buffer<float>(buffer, result_data);
    EXPECT_EQ(result_data, expected_data);
  }

  // Test Multiply
  {
    const std::vector<float> host_data = {1.0F, 2.0F, 3.0F};
    const std::vector<float> expected_data = {2.0F, 4.0F, 6.0F}; // * 2.0
    Buffer<float> buffer = context.new_buffer_from_host_memory<float>(host_data);

    const KernelSpec kernel_spec{
        .compute_shader = {.shader_source = InlineShaderText{shaderCode}, .entry_point_args = {"Multiply"}},
        .bindings_map = {{"data", buffer}},
    };
    const Kernel kernel = context.new_kernel(kernel_spec);
    const DispatchGrid dispatch_grid = {static_cast<uint32_t>(host_data.size()), 1, 1};
    context.dispatch_kernel(kernel, dispatch_grid);

    std::vector<float> result_data(host_data.size());
    context.download_buffer<float>(buffer, result_data);
    EXPECT_EQ(result_data, expected_data);
  }
}

TEST_F(GPUTest, CopyBufferToBuffer_Full) {
  std::vector<uint32_t> src_data = {1, 2, 3, 4, 5};

  const Buffer<uint32_t> src_buffer = context.new_buffer_from_host_memory<uint32_t>(src_data);
  const Buffer<uint32_t> dst_buffer = context.new_empty_buffer<uint32_t>(src_data.size());

  // defaults to zeros -> byteSize == 0 means copy whole buffer
  const ComputeContext::BufferCopyInfo info{.byteSize = 0};

  context.copy_buffer_to_buffer(src_buffer, dst_buffer, info);

  std::vector<uint32_t> downloaded_data(src_data.size());
  context.download_buffer<uint32_t>(dst_buffer, downloaded_data);

  EXPECT_EQ(downloaded_data, src_data);
}

TEST_F(GPUTest, CopyBufferToBuffer_Partial) {
  std::vector<uint32_t> src(10);
  std::iota(src.begin(), src.end(), 0U);

  const Buffer<uint32_t> src_buffer = context.new_buffer_from_host_memory<uint32_t>(src);
  Buffer<uint32_t> dst_buffer = context.new_buffer_from_host_memory<uint32_t>(src);

  const ComputeContext::BufferCopyInfo info{
      .srcOffset = 2 * sizeof(uint32_t), .dstOffset = 5 * sizeof(uint32_t), .byteSize = 3 * sizeof(uint32_t)};

  context.copy_buffer_to_buffer(src_buffer, dst_buffer, info);

  std::vector<uint32_t> downloaded_data(src.size());
  context.download_buffer<uint32_t>(dst_buffer, downloaded_data);

  const size_t src_start = info.srcOffset / sizeof(uint32_t);
  const size_t dst_start = info.dstOffset / sizeof(uint32_t);
  const size_t count = info.byteSize / sizeof(uint32_t);

  for (size_t i = 0; i < count; ++i) {
    EXPECT_EQ(downloaded_data[dst_start + i], src[src_start + i]);
  }

  for (size_t i = 0; i < downloaded_data.size(); ++i) {
    if (i < dst_start || i >= dst_start + count) {
      EXPECT_EQ(downloaded_data[i], src[i]);
    }
  }
}

TEST_F(GPUTest, CopyBufferToBuffer_SourceOutOfRangeThrows) {
  std::vector<uint32_t> src = {1, 2, 3};
  std::vector<uint32_t> dst = {0, 0, 0};

  const Buffer<uint32_t> src_buffer = context.new_buffer_from_host_memory<uint32_t>(src);
  const Buffer<uint32_t> dst_buffer = context.new_buffer_from_host_memory<uint32_t>(dst);

  // set srcOffset near end and request more bytes than available
  const ComputeContext::BufferCopyInfo info{
      .srcOffset = 2 * sizeof(uint32_t), // points to last element
      .dstOffset = 0,
      .byteSize = 2 * sizeof(uint32_t) // will read last + one beyond -> should throw
  };
  EXPECT_THROW(context.copy_buffer_to_buffer(src_buffer, dst_buffer, info), Exception);
}

TEST_F(GPUTest, CopyBufferToBuffer_DestinationOutOfRangeThrows) {
  std::vector<uint32_t> src = {10, 20, 30, 40};
  std::vector<uint32_t> dst = {0, 0};

  const Buffer<uint32_t> src_buffer = context.new_buffer_from_host_memory<uint32_t>(src);
  const Buffer<uint32_t> dst_buffer = context.new_buffer_from_host_memory<uint32_t>(dst);

  const ComputeContext::BufferCopyInfo info{
      .srcOffset = 0,
      .dstOffset = 1 * sizeof(uint32_t), // only room for one element, but we'll request more
      .byteSize = 2 * sizeof(uint32_t)   // exceeds dst size -> should throw
  };
  EXPECT_THROW(context.copy_buffer_to_buffer(src_buffer, dst_buffer, info), Exception);
}

TEST_F(GPUTest, ClearBuffer) {
  std::vector<float> data = {1.5F, -2.0F, 3.25F, 4.0F};

  const Buffer<float> buffer = context.new_buffer_from_host_memory<float>(data);

  // Ensure buffer contains non-zero values initially
  std::vector<float> before(data.size(), 0.0F);
  context.download_buffer<float>(buffer, before);
  for (auto v : before)
    EXPECT_NE(v, 0.0F);

  context.clear_buffer<float>(buffer);

  std::vector<float> downloaded(data.size(), 1.0F);
  context.download_buffer<float>(buffer, downloaded);
  for (auto val : downloaded) {
    EXPECT_FLOAT_EQ(val, 0.0F);
  }
}

TEST_F(GPUTest, DownloadBufferAsVector) {
  const std::vector<int32_t> host = {10, 20, 30, 40};

  const Buffer<int32_t> buffer = context.new_buffer_from_host_memory<int32_t>(host);

  const std::vector<int32_t> downloaded = context.download_buffer_as_vector<int32_t>(buffer);

  EXPECT_EQ(downloaded, host);
}
