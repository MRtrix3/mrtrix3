/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#include <memory>

#include "algo/threaded_loop.h"
#include "axes.h"
#include "command.h"
#include "denoise/denoise.h"
#include "denoise/estimate.h"
#include "denoise/estimator/estimator.h"
#include "denoise/exports.h"
#include "denoise/iterative.h"
#include "denoise/kernel/kernel.h"
#include "denoise/mask.h"
#include "denoise/precondition.h"
#include "denoise/subsample.h"
#include "dwi/gradient.h"
#include "exception.h"
#include "filter/demodulate.h"
#include "filter/smooth.h"
#include "interp/linear.h"

using namespace MR;
using namespace App;
using namespace MR::Denoise;

// clang-format off
void usage() {

  SYNOPSIS = "Noise level estimation using Marchenko-Pastur PCA";

  DESCRIPTION
  + "DWI data noise map estimation"
    " by interrogating data redundancy in the PCA domain"
    " using the prior knowledge that the eigenspectrum of random covariance matrices"
    " is described by the universal Marchenko-Pastur (MP) distribution."
    " Fitting the MP distribution to the spectrum of patch-wise signal matrices"
    " hence provides an estimator of the noise level 'sigma'."

  + "Unlike the MRtrix3 command dwidenoise,"
    " this command does not generate a denoised version of the input image series;"
    " its primary output is instead a map of the estimated noise level."
    " While this can also be obtained from the dwidenoise command using option -noise_out,"
    " using instead the dwi2noise command gives the ability to obtain a noise map"
    " to which filtering can be applied,"
    " which can then be utilised for the actual image series denoising,"
    " without generating an unwanted intermiedate denoised image series."

  + "Important note:"
    " noise level estimation should only be performed as the first step of an image processing pipeline."
    " The routine is invalid if interpolation or smoothing has been applied to the data prior to denoising."

  + "Note that on complex input data,"
    " the output will be the total noise level across real and imaginary channels,"
    " so a scale factor sqrt(2) applies."

  + demodulation_description

  + Kernel::shape_description

  + Kernel::default_size_description

  + Kernel::cuboid_size_description;

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)"
           " and Daan Christiaens (daan.christiaens@kcl.ac.uk)"
           " and Jelle Veraart (jelle.veraart@nyumc.org)"
           " and J-Donald Tournier (jdtournier@gmail.com)";

  REFERENCES
  + "Veraart, J.; Fieremans, E. & Novikov, D.S. " // Internal
    "Diffusion MRI noise mapping using random matrix theory. "
    "Magn. Res. Med., 2016, 76(5), 1582-1593, doi: 10.1002/mrm.26059"

  + "Cordero-Grande, L.; Christiaens, D.; Hutter, J.; Price, A.N.; Hajnal, J.V. " // Internal
    "Complex diffusion-weighted image estimation via matrix recovery under general noise models. "
    "NeuroImage, 2019, 200, 391-404, doi: 10.1016/j.neuroimage.2019.06.039"

  + "* If using -estimator mrm2023: "
    "Olesen, J.L.; Ianus, A.; Ostergaard, L.; Shemesh, N.; Jespersen, S.N. "
    "Tensor denoising of multidimensional MRI data. "
    "Magnetic Resonance in Medicine, 2023, 89(3), 1160-1172"

  + "* If using -estimator med: "
    "Gavish, M.; Donoho, D.L. "
    "The Optimal Hard Threshold for Singular Values is 4/sqrt(3). "
    "IEEE Transactions on Information Theory, 2014, 60(8), 5040-5053.";

  ARGUMENTS
  + Argument("dwi", "the input diffusion-weighted image").type_image_in()
  + Argument("noise", "the output estimated noise level map").type_image_out();

  OPTIONS
  + OptionGroup("Options for modifying PCA computations")
  + datatype_option
  + Estimator::estimator_option
  + Option("iterative",
           "Derive noise level estimate through iterative multi-resolution process")
  + Kernel::options
  + subsample_option
  + precondition_options(false)

  + DWI::GradImportOptions()
  + DWI::GradExportOptions()

  + OptionGroup("Options for exporting additional data regarding PCA behaviour")
  + Option("rank_pcanonzero",
           "The (non-zero) rank of the PCA decomposition prior to any noise level estimation")
    + Argument("image").type_image_out()
  + Option("lamplus",
           "The estimated upper bound of the noise portion of the eigenspectrum (\"lambda-plus\")")
    + Argument("image").type_image_out()
  + Option("rank_input",
           "The estimated rank of the input data for each denoising patch")
    + Argument("image").type_image_out()
  + Option("eigenspectra",
           "Output a matrix containing the spectra of eigenvalues across patches")
    + Argument("file").type_file_out()

  + OptionGroup("Options for debugging the operation of sliding window kernels")
  + Option("max_dist",
           "The maximum distance between the centre of the patch and a voxel that was included within that patch")
    + Argument("image").type_image_out()
  + Option("voxelcount",
           "The number of voxels that contributed to the PCA for processing of each patch")
    + Argument("image").type_image_out()
  + Option("patchcount",
           "The number of unique patches to which an input image voxel contributes")
    + Argument("image").type_image_out();

}
// clang-format on

const std::vector<Iterative::Iteration> default_iterations({{{8, 8, 8}, 16.0, false}, //
                                                            {{4, 4, 4}, 4.0, false},  //
                                                            {{2, 2, 2}, 1.0, true}}); //

template <typename T>
void run(Header &dwi,
         const Demodulation &demodulation,
         const demean_type demean,
         Image<float> &user_vst_image,
         std::shared_ptr<Estimator::Base> estimator,
         const std::vector<Iterative::Iteration> &iterations,
         Exports &final_exports) {

  Image<T> input(dwi.get_image<T>());
  Image<bool> mask = generate_mask(input);
  Image<float> vst_image(user_vst_image);

  Header H_preconditioned(input);
  Stride::set(H_preconditioned, Stride::contiguous_along_axis(3, input));
  Image<T> input_preconditioned =
      Image<T>::scratch(H_preconditioned, "Preconditioned version of \"" + dwi.name() + "\"");

  // First two components of this, being demodulation and demeaning,
  //   are consistent across iterations;
  //   therefore don't require recalculation of these,
  //   just overwrite the VST image as required
  Precondition<T> preconditioner(input, demodulation, demean, user_vst_image);

  // All but the last iteration
  for (ssize_t iteration = 0; iteration != iterations.size() - 1; ++iteration) {
    std::shared_ptr<Subsample> subsample = std::make_shared<Subsample>(dwi, iterations[iteration].subsample_ratios);
    // For internal iterations, we only save the output noise level estimate
    Exports iteration_exports(dwi, subsample->header());
    iteration_exports.set_noise_out();
    Iterative::estimate(input,
                        input_preconditioned,
                        mask,
                        vst_image,
                        iterations[iteration],
                        iteration,
                        subsample,
                        estimator,
                        preconditioner,
                        iteration_exports);
    // Propagate result to next iteration
    // TODO Suspect I need to pad this by one voxel in each direction
    //   in order for the linear interpolation to work in the next iteration

    vst_image = iteration_exports.noise_out;
    preconditioner.update_vst_image(vst_image);
    estimator->update_vst_image(vst_image);
  }

  // Last iteration
  // Note that this may be modified by the user at the command-line
  auto subsample = Subsample::make(dwi, default_subsample_ratio);
  Iterative::estimate(input,
                      input_preconditioned,
                      mask,
                      vst_image,
                      iterations.back(),
                      iterations.size() - 1,
                      subsample,
                      estimator,
                      preconditioner,
                      final_exports);
  const uint16_t null_rank = uint16_t(preconditioner.null_rank());
  if (null_rank > 0 && final_exports.rank_input.valid()) {
    for (auto l = Loop(final_exports.rank_input)(final_exports.rank_input); l; ++l)
      final_exports.rank_input.value() =
          std::max<uint16_t>(uint16_t(final_exports.rank_input.value()) + null_rank, uint16_t(dwi.size(3)));
  }
}

void run() {
  auto dwi = Header::open(argument[0]);
  if (dwi.ndim() != 4 || dwi.size(3) <= 1)
    throw Exception("input image must be 4-dimensional");
  bool complex = dwi.datatype().is_complex();

  const Demodulation demodulation = select_demodulation(dwi);
  const demean_type demean = select_demean(dwi);

  Image<float> user_vst_image;
  auto opt = get_options("vst");
  if (!opt.empty()) {
    if (demean == demean_type::NONE)
      throw Exception("Cannot currently apply variance-stabilising transform in the absence of demeaning");
    user_vst_image = Image<float>::open(opt[0][0]);
    if (user_vst_image.ndim() != 3)
      throw Exception("Variance-stabilising transform noise level image must be 3D");
  }

  auto estimator = Estimator::make_estimator(user_vst_image, false);

  // Need to set up the subsampling header of the final iteration for configuration of final exports
  auto final_subsample = Subsample::make(dwi, default_subsample_ratio);

  std::vector<Iterative::Iteration> iterations;
  if (get_options("iterative").empty()) {
    Iterative::Iteration config;
    config.subsample_ratios = final_subsample->get_factors();
    config.kernel_size_multiplier = 1.0;
    config.smooth_noiseout = true;
    iterations.push_back(config);
  } else {
    if (!get_options("subsample").empty())
      throw Exception("Implementation does not support use of both -iterative and -subsample");
    iterations = default_iterations;
  }
  Exports final_exports(dwi, final_subsample->header());
  final_exports.set_noise_out(argument[1]);
  opt = get_options("lamplus");
  if (!opt.empty())
    final_exports.set_lamplus(opt[0][0]);
  opt = get_options("rank_pcanonzero");
  if (!opt.empty())
    final_exports.set_rank_pcanonzero(opt[0][0]);
  opt = get_options("rank_input");
  if (!opt.empty())
    final_exports.set_rank_input(opt[0][0]);
  opt = get_options("max_dist");
  if (!opt.empty())
    final_exports.set_max_dist(opt[0][0]);
  opt = get_options("voxelcount");
  if (!opt.empty())
    final_exports.set_voxelcount(opt[0][0]);
  opt = get_options("patchcount");
  if (!opt.empty())
    final_exports.set_patchcount(opt[0][0]);
  opt = get_options("eigenspectra");
  if (!opt.empty())
    final_exports.set_eigenspectra_path(opt[0][0]);

  int prec = get_option_value("datatype", 0); // default: single precision
  if (complex)
    prec += 2; // support complex input data
  switch (prec) {
  case 0:
    assert(demodulation.axes.empty());
    INFO("select real float32 for processing");
    run<float>(dwi, demodulation, demean, user_vst_image, estimator, iterations, final_exports);
    break;
  case 1:
    assert(demodulation.axes.empty());
    INFO("select real float64 for processing");
    run<double>(dwi, demodulation, demean, user_vst_image, estimator, iterations, final_exports);
    break;
  case 2:
    INFO("select complex float32 for processing");
    run<cfloat>(dwi, demodulation, demean, user_vst_image, estimator, iterations, final_exports);
    break;
  case 3:
    INFO("select complex float64 for processing");
    run<cdouble>(dwi, demodulation, demean, user_vst_image, estimator, iterations, final_exports);
    break;
  }
}
