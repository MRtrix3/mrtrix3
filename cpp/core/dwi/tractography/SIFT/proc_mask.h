/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#include <filesystem>
#include <optional>

#include "cmdline_option.h"
#include "image.h"

#include "algo/iterator.h"
#include "interp/linear.h"

#include "dwi/tractography/ACT/act.h"
#include "dwi/tractography/ACT/tissues.h"

namespace MR::DWI::Tractography::SIFT {

extern const App::OptionGroup SIFTModelProcMaskOption;

//! build the SIFT model processing mask from the supplied inputs (no command-line access)
/*! \param in_dwi         the input FOD / DWI image defining the target grid
 *  \param out_mask       the processing mask to populate
 *  \param out_5tt        scratch buffer populated with regridded 5TT data when \a act_5tt_path is provided
 *  \param proc_mask_path optional path to a user-supplied processing mask image
 *  \param act_5tt_path   optional path to an ACT 5TT image from which to derive the mask (used only if
 *                        \a proc_mask_path is absent); when both are absent a homogeneous mask is created */
void initialise_processing_mask(Image<float> &in_dwi,
                                Image<float> &out_mask,
                                Image<float> &out_5tt,
                                const std::optional<std::filesystem::path> &proc_mask_path,
                                const std::optional<std::filesystem::path> &act_5tt_path);

// Private functor for performing ACT image regridding
class ResampleFunctor {

  using transform_type = Eigen::Transform<float, 3, Eigen::AffineCompact>;

public:
  ResampleFunctor(Image<float> &, Image<float> &, Image<float> &);
  ResampleFunctor(const ResampleFunctor &);

  void operator()(const Iterator &);

private:
  Image<float> dwi;
  std::shared_ptr<transform_type> voxel2scanner;
  Interp::Linear<Image<float>> interp_anat;
  Image<float> out;

  // Helper function for doing the regridding
  ACT::Tissues ACT2pve(const Iterator &);
};

} // namespace MR::DWI::Tractography::SIFT
