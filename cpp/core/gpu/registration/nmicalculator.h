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

#include "gpu/registration/calculatoroutput.h"
#include "gpu/gpu.h"
#include "gpu/registration/registrationtypes.h"
#include "gpu/registration/voxelscannermatrices.h"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace MR::GPU {

// Match shader order: source (moving) then target (fixed)
struct Intensities {
  float min_moving;
  float max_moving;
  float min_fixed;
  float max_fixed;
};

class NMICalculator {
public:
  struct Config {
    TransformationType transformation_type = TransformationType::Affine;
    Texture fixed;
    Texture moving;
    std::optional<Texture> fixed_mask;
    std::optional<Texture> moving_mask;
    VoxelScannerMatrices voxel_scanner_matrices{};
    uint32_t num_bins = 32;
    CalculatorOutput output = CalculatorOutput::CostAndGradients;
    const ComputeContext *context = nullptr;
  };
  explicit NMICalculator(const Config &config);

  void update(const GlobalTransform &transformation);
  IterationResult get_result() const;

private:
  CalculatorOutput m_output = CalculatorOutput::CostAndGradients;
  const ComputeContext *m_compute_context = nullptr;

  Buffer<uint32_t> m_raw_joint_histogram_buffer;
  Buffer<float> m_smoothed_joint_histogram_buffer;
  Buffer<float> m_joint_histogram_mass_buffer;
  Buffer<std::byte> m_joint_histogram_uniforms_buffer;
  Buffer<std::byte> m_min_max_uniforms_buffer;
  Buffer<uint32_t> m_min_max_intensity_fixed_buffer;
  Buffer<uint32_t> m_min_max_intensity_moving_buffer;
  Buffer<float> m_precomputed_coefficients_buffer;
  Buffer<float> m_mutual_information_buffer;
  Buffer<std::byte> m_gradients_uniforms_buffer;
  Buffer<float> m_gradients_buffer;

  Kernel m_min_max_moving_kernel;
  Kernel m_joint_histogram_kernel;
  Kernel m_joint_histogram_smooth_kernel;
  Kernel m_compute_total_mass_kernel;
  Kernel m_precompute_kernel;
  Kernel m_gradients_kernel;

  Texture m_fixed;
  Texture m_moving;
  Texture m_fixed_mask;
  Texture m_moving_mask;
  bool m_use_fixed_mask = false;
  bool m_use_moving_mask = false;

  VoxelScannerMatrices m_voxel_scanner_matrices;
  Eigen::Vector3f m_centre_scanner_fixed;
  Eigen::Vector3f m_centre_scanner_moving;

  DispatchGrid m_joint_histogram_dispatch_grid;
  DispatchGrid m_gradients_dispatch_grid;

  uint32_t m_num_bins = 32;
  Intensities m_intensities;
  uint32_t m_degrees_of_freedom;
};
} // namespace MR::GPU
