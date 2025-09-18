#include "app.h"
#include "cmdline_option.h"
#include "command.h"
#include "file/matrix.h"
#include "image.h"
#include "image_helpers.h"
#include "gpu.h"
#include "transform.h"
#include "types.h"
#include "utils.h"

#include <Eigen/Core>
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <vector>

using namespace MR;
using namespace App;

namespace {

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

// A cross-platform function to get the path of the executable
std::filesystem::path getExecutablePath() {
#if defined(_WIN32)
  wchar_t buffer[MAX_PATH];
  DWORD len = GetModuleFileNameW(NULL, buffer, MAX_PATH);
  if (len == 0 || len == MAX_PATH)
    throw std::runtime_error("GetModuleFileNameW failed");
  return std::filesystem::path(buffer);
#elif defined(__APPLE__)
  uint32_t size = 0;
  if (_NSGetExecutablePath(nullptr, &size) != -1)
    throw std::runtime_error("Unexpected success getting buffer size");
  std::string buffer;
  buffer.resize(size);
  if (_NSGetExecutablePath(&buffer[0], &size) != 0)
    throw std::runtime_error("Buffer still too small");
  return std::filesystem::canonical(std::filesystem::path(buffer));
#elif defined(__linux__)
  std::string link = "/proc/self/exe";
  std::error_code ec;
  auto p = std::filesystem::read_symlink(link, ec);
  if (ec)
    throw std::runtime_error("read_symlink failed: " + ec.message());
  return std::filesystem::canonical(p);
#else
#error Unsupported platform
#endif
}
}

void usage() {
  AUTHOR = "Daljit Singh",
      SYNOPSIS = "Transforms an image given the input image and the backward transformation matrix";

  ARGUMENTS
      + Argument ("input", "input image").type_image_in()
      + Argument ("output", "the output image.").type_image_out()
      + Argument ("transform").type_file_in();

}


void run()
{
  const std::filesystem::path inputPath { argument.at(0) };
  const std::filesystem::path outputPath { argument.at(1) };
  const std::filesystem::path transformPath { argument.at(2) };

  const transform_type transform = File::Matrix::load_transform(transformPath);
  const GPU::ComputeContext context;

  const auto inputImage = Image<float>::open(inputPath).with_direct_io();
  const GPU::Texture inputTexture = context.newTextureFromHostImage(inputImage);

  const transform_type transformationWorldCoords = Transform(inputImage).scanner2voxel *
                                                   transform *
                                                   Transform(inputImage).voxel2scanner;

  auto to4x4Matrix = [](const transform_type& t) {
    const auto mat3x4 = t.matrix().cast<float>();
    Eigen::Matrix4f matrix;
    matrix.block<3, 4>(0, 0) = mat3x4;
    matrix.block<1, 4>(3, 0) = Eigen::RowVector4f(0, 0, 0, 1);
    return matrix;
  };

  const Eigen::Matrix4f transformationMat = to4x4Matrix(transformationWorldCoords);
  constexpr MR::GPU::WorkgroupSize workgroupSize = {8, 8, 4};

  const MR::GPU::Buffer<float> transformBuffer = context.newBufferFromHostMemory<float>(transformationMat);

  const MR::GPU::TextureSpec outputTextureSpec {
      .width = inputTexture.spec.width,
      .height = inputTexture.spec.height,
      .depth = inputTexture.spec.depth,
      .format = GPU::TextureFormat::R32Float,
      .usage = { .storageBinding = true }
  };
  const GPU::Texture outputTexture = context.newEmptyTexture(outputTextureSpec);

  const auto currentPath = getExecutablePath().parent_path();
  const GPU::KernelSpec transformKernelSpec {
      .computeShader = {
        .shaderSource = GPU::ShaderFile { currentPath / "shaders/transform_image.wgsl" },
        .workgroupSize = workgroupSize,
      },
      .readOnlyBuffers = { transformBuffer},
      .readOnlyTextures = { inputTexture },
      .writeOnlyTextures = { outputTexture },
      .samplers = { context.newLinearSampler() }
  };

  const GPU::Kernel transformKernel = context.newKernel(transformKernelSpec);

  const auto width  = inputTexture.spec.width;
  const auto height = inputTexture.spec.height;
  const auto depth  = inputTexture.spec.depth;

  const GPU::DispatchGrid dispatchGrid {
      .wgCountX = Utils::nextMultipleOf(width / workgroupSize.x, workgroupSize.x),
      .wgCountY = Utils::nextMultipleOf(height / workgroupSize.y, workgroupSize.y),
      .wgCountZ = Utils::nextMultipleOf(depth / workgroupSize.z, workgroupSize.z)
  };

  context.dispatchKernel(transformKernel, dispatchGrid);

  std::vector<float> gpuData(MR::voxel_count(inputImage), 0.0F);
  context.downloadTexture(outputTexture, gpuData);

  Image<float> outputImage = Image<float>::scratch(inputImage);
  float *data = static_cast<float*>(outputImage.address());

  std::copy_n(gpuData.data(), gpuData.size(), data);

  MR::save(outputImage, outputPath);
}
