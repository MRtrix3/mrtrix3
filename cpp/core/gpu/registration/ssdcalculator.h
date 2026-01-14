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
#include <optional>

namespace MR::GPU {
class SSDCalculator {
public:
  struct Config {
    TransformationType transformation_type = TransformationType::Affine;
    Texture fixed;
    Texture moving;
    std::optional<Texture> fixed_mask;
    std::optional<Texture> moving_mask;
    VoxelScannerMatrices voxel_scanner_matrices{};
    CalculatorOutput output = CalculatorOutput::CostAndGradients;
    const ComputeContext *context = nullptr;
  };
  explicit SSDCalculator(const Config &config);

  void update(const GlobalTransform &transformation);
  IterationResult get_result() const;

private:
  CalculatorOutput m_output = CalculatorOutput::CostAndGradients;
  const ComputeContext *m_compute_context = nullptr;
  Buffer<std::byte> m_uniforms_buffer;
  Buffer<float> m_partials_buffer;
  Buffer<uint32_t> m_num_contributing_voxels_buffer;

  Kernel m_kernel;

  Texture m_fixed;
  Texture m_moving;
  Texture m_fixed_mask;
  Texture m_moving_mask;
  bool m_use_fixed_mask = false;
  bool m_use_moving_mask = false;

  DispatchGrid m_dispatch_grid;
  VoxelScannerMatrices m_voxel_scanner_matrices;
  uint32_t m_degrees_of_freedom = 0;
};
} // namespace MR::GPU
