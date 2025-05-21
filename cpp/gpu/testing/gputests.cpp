#include <gtest/gtest.h>

#include "exception.h"
#include "gpu.h"
#include "gputests_common.h"
#include "span.h"

#include <cstdint>
#include <cstddef>
#include <exception>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>


using namespace MR;
using namespace MR::GPU;


TEST_F(GPUTest, MakeEmptyBuffer) {
    const size_t bufferElementCount = 1024;
    const Buffer<uint32_t> buffer = context.newEmptyBuffer<uint32_t>(bufferElementCount);

    std::vector<uint32_t> downloadedData(bufferElementCount, 1); // Initialize with non-zero

    context.downloadBuffer<uint32_t>(buffer, downloadedData);

    for (auto val : downloadedData) {
        EXPECT_EQ(val, 0);
    }
}

TEST_F(GPUTest, BufferFromHostMemory) {
    std::vector<int32_t> hostData = {1, 2, 3, 4, 5};

    const Buffer<int32_t> buffer = context.newBufferFromHostMemory<int32_t>(tcb::span<const int32_t>(hostData));

    std::vector<int32_t> downloadedData(hostData.size(), 0);
    context.downloadBuffer<int32_t>(buffer, downloadedData);

    EXPECT_EQ(downloadedData, hostData);
}

TEST_F(GPUTest, BufferFromHostMemoryVoidPtr) {
    std::vector<float> hostData = {1.0F, 2.5F, -3.0F};
    const Buffer<float> buffer = context.newBufferFromHostMemory<float>(hostData.data(), hostData.size() * sizeof(float));

    std::vector<float> downloadedData(hostData.size());
    context.downloadBuffer<float>(buffer, downloadedData);
    EXPECT_EQ(downloadedData, hostData);
}


TEST_F(GPUTest, BufferFromHostMemoryMultipleRegions) {
    std::vector<uint32_t> region1 = {1, 2, 3};
    std::vector<uint32_t> region2 = {4, 5};
    std::vector<uint32_t> region3 = {6, 7, 8, 9};

    const std::vector<tcb::span<const uint32_t>> regions = {region1, region2, region3};
    const Buffer<uint32_t> buffer = context.newBufferFromHostMemory<uint32_t>(regions);

    const std::vector<uint32_t> expectedData = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::vector<uint32_t> downloadedData(expectedData.size());
    context.downloadBuffer<uint32_t>(buffer, downloadedData);

    EXPECT_EQ(downloadedData, expectedData);
}


TEST_F(GPUTest, WriteToBuffer) {
    std::vector<float> newData = {0.1F, 0.2F, 0.3F, 0.4F};

    const Buffer<float> buffer = context.newEmptyBuffer<float>(newData.size());
    std::vector<float> downloadedData(newData.size(), 0.0F);

    context.writeToBuffer<float>(buffer, newData);
    context.downloadBuffer<float>(buffer, downloadedData);

    for (size_t i = 0; i < newData.size(); i++) {
        EXPECT_FLOAT_EQ(downloadedData[i], newData[i]);
    }
}

TEST_F(GPUTest, WriteToBufferWithOffset) {
    const size_t bufferSize = 10;
    std::vector<float> initialData(bufferSize);
    std::iota(initialData.begin(), initialData.end(), 0.0F); // 0, 1, ..., 9

    const Buffer<float> buffer = context.newBufferFromHostMemory<float>(initialData);

    std::vector<float> newData = {100.0F, 101.0F, 102.0F};
    const uint32_t offsetElements = 3;
    const uint32_t offsetBytes = offsetElements * sizeof(float);

    context.writeToBuffer<float>(buffer, newData, offsetBytes);

    std::vector<float> downloadedData(bufferSize);
    context.downloadBuffer<float>(buffer, downloadedData);

    std::vector<float> expectedData = {0.0F, 1.0F, 2.0F, 100.0F, 101.0F, 102.0F, 6.0F, 7.0F, 8.0F, 9.0F};
    for (size_t i = 0; i < bufferSize; ++i) {
        EXPECT_FLOAT_EQ(downloadedData[i], expectedData[i]);
    }
}


TEST_F(GPUTest, EmptyTexture) {
    const MR::GPU::TextureSpec textureSpec = {
        .width = 4, .height = 4, .depth = 1, .format = TextureFormat::R32Float,
    };

    const auto texture = context.newEmptyTexture(textureSpec);

    const uint32_t bytesPerPixel = 4; // R32Float
    const size_t downloadedSizeBytes = textureSpec.width * textureSpec.height * textureSpec.depth * bytesPerPixel;
    std::vector<float> downloadedData(downloadedSizeBytes / sizeof(float), 1.0f); // Init with non-zero

    context.downloadTexture(texture, downloadedData);

    for (uint32_t z = 0; z < textureSpec.depth; ++z) {
        for (uint32_t y = 0; y < textureSpec.height; ++y) {
            for (uint32_t x = 0; x < textureSpec.width; ++x) {
                const size_t idx = (z * textureSpec.height + y) * textureSpec.width + x;
                EXPECT_FLOAT_EQ(downloadedData[idx], 0.0F);
            }
        }
    }
}


TEST_F(GPUTest, KernelWithInlineShader) {
    const std::string shaderCode = R"wgsl(
        @group(0) @binding(0) var<storage, read_write> data: array<f32>;

        @compute @workgroup_size(64)
        fn main(@builtin(global_invocation_id) id: vec3<u32>) {
            let idx = id.x;
            if (idx < arrayLength(&data)) {
                data[idx] = data[idx] * 3.0;
            }
        }
    )wgsl";

    const std::vector<float> hostData = {1.0F, 2.0F, 3.0F, 4.0F};
    const std::vector<float> expectedData = {3.0F, 6.0F, 9.0F, 12.0F};
    Buffer<float> buffer = context.newBufferFromHostMemory<float>(hostData);

    const KernelSpec kernelSpec {
        .computeShader = {
            .shaderSource = InlineShaderText{ shaderCode },
        },
        .readWriteBuffers = { buffer }
    };

    const Kernel kernel = context.newKernel(kernelSpec);
    const DispatchGrid dispatchGrid = { static_cast<uint32_t>((hostData.size() + 63)), 1, 1 };
    context.dispatchKernel(kernel, dispatchGrid);

    std::vector<float> resultData(hostData.size());
    context.downloadBuffer<float>(buffer, resultData);
    EXPECT_EQ(resultData, expectedData);
}


TEST_F(GPUTest, KernelWithPlaceholders) {
    const std::string shaderCode = R"wgsl(
        @group(0) @binding(0) var<storage, read_write> data: array<f32>;

        @compute @workgroup_size(64)
        fn main(@builtin(global_invocation_id) id: vec3<u32>) {
            let idx = id.x;
            if (idx < arrayLength(&data)) {
                data[idx] = data[idx] + {{value_to_add}};
            }
        }
    )wgsl";

    const std::vector<float> hostData = {10.0F, 20.0F};
    const float valueToAdd = 5.5F;
    const std::vector<float> expectedData = {15.5F, 25.5F};
    const Buffer<float> buffer = context.newBufferFromHostMemory<float>(hostData);

    const KernelSpec kernelSpec {
        .computeShader = {
            .shaderSource = InlineShaderText{ shaderCode },
            .placeholders = {{"value_to_add", std::to_string(valueToAdd)}}
        },
        .readWriteBuffers = { buffer }
    };

    const Kernel kernel = context.newKernel(kernelSpec);
    constexpr DispatchGrid dispatchGrid = { 1, 1, 1 };
    context.dispatchKernel(kernel, dispatchGrid);

    std::vector<float> resultData(hostData.size());
    context.downloadBuffer<float>(buffer, resultData);
    EXPECT_EQ(resultData, expectedData);
}

TEST_F(GPUTest, KernelWithMacros) {
    const std::string shaderCode = R"wgsl(
        @group(0) @binding(0) var<storage, read_write> data: array<f32>;

        @compute @workgroup_size(64)
        fn main_macro(@builtin(global_invocation_id) id: vec3<u32>) {
            let idx = id.x;
            if (idx < arrayLength(&data)) {
                #ifdef MULTIPLY_MODE
                data[idx] = data[idx] * 2.0;
                #else
                data[idx] = data[idx] + 1.0;
                #endif
            }
        }
    )wgsl";

    std::vector<float> hostData = {5.0F, 10.0F};
    Buffer<float> buffer = context.newBufferFromHostMemory<float>(hostData);

    // Test with MULTIPLY_MODE defined
    const KernelSpec specMultiply {
        .computeShader = {
            .shaderSource = InlineShaderText{ shaderCode },
            .entryPoint = "main_macro",
            .macros = {"MULTIPLY_MODE"}
        },
        .readWriteBuffers = { buffer }
    };
    const Kernel kernelMultiply = context.newKernel(specMultiply);
    const DispatchGrid dispatchGrid = { 1, 1, 1 };
    context.dispatchKernel(kernelMultiply, dispatchGrid);

    std::vector<float> resultDataMultiply(hostData.size());
    context.downloadBuffer<float>(buffer, resultDataMultiply);
    const std::vector<float> expectedDataMultiply = {10.0F, 20.0F};
    EXPECT_EQ(resultDataMultiply, expectedDataMultiply);

    // Test without MULTIPLY_MODE (ADD_MODE)
    context.writeToBuffer<float>(buffer, hostData); // Reset buffer
    const KernelSpec specAdd {
        .computeShader = {
            .shaderSource = InlineShaderText{ shaderCode },
            .entryPoint = "main_macro"
            // .macros is empty
        },
        .readWriteBuffers = { buffer }
    };
    const Kernel kernelAdd = context.newKernel(specAdd);
    context.dispatchKernel(kernelAdd, dispatchGrid);

    std::vector<float> resultDataAdd(hostData.size());
    context.downloadBuffer<float>(buffer, resultDataAdd);
    const std::vector<float> expectedDataAdd = {6.0F, 11.0F};
    EXPECT_EQ(resultDataAdd, expectedDataAdd);
}

int main(int argc, char **argv) {
    try {
        ::testing::InitGoogleTest(&argc, argv);
        return RUN_ALL_TESTS();
    }
    catch (const MR::Exception &e) {
        e.display();
        return 1;
    }
    catch (const std::exception &e) {
        std::cerr << "Uncaught exception: " << e.what() << "\n";
        return 1;
    }
    catch (...) {
        std::cerr << "Uncaught exception of unknown type" << "\n";
        return 1;
    }
}
