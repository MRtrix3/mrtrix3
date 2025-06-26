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

#include <string>

#include "command.h"
#include "filter/demodulate.h"
#include "header.h"
#include "image.h"
#include "stride.h"

#include <Eigen/Dense>
#include <Eigen/Eigenvalues>

#include "denoise/denoise.h"
#include "denoise/estimate.h"
#include "denoise/estimator/estimator.h"
#include "denoise/estimator/unity.h"
#include "denoise/exports.h"
#include "denoise/iterative.h"
#include "denoise/kernel/cuboid.h"
#include "denoise/kernel/data.h"
#include "denoise/kernel/kernel.h"
#include "denoise/kernel/sphere_radius.h"
#include "denoise/kernel/sphere_ratio.h"
#include "denoise/mask.h"
#include "denoise/precondition.h"
#include "denoise/recon.h"
#include "denoise/subsample.h"
#include "dwi/gradient.h"

using namespace MR;
using namespace App;
using namespace MR::Denoise;

// clang-format off
void usage() {

  SYNOPSIS = "Improved dMRI denoising using Marchenko-Pastur PCA";

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)"
           " and Daan Christiaens (daan.christiaens@kcl.ac.uk)"
           " and Jelle Veraart (jelle.veraart@nyumc.org)"
           " and J-Donald Tournier (jdtournier@gmail.com)";

  DESCRIPTION
  + "This command performs DWI data denoising,"
    " additionally with data-driven noise map estimation if not provided explicitly."
    " The output denoised DWI data is formed based on filtering of eigenvectors of PCA decompositions:"
    " for a set of patches each of which consists of a set of proximal voxels,"
    " the PCA decomposition is applied,"
    " and the DWI signal for those voxels is reconstructed"
    " where the contribution of each eigenvector is modulated based on its classification as noise."
    " In many use cases,"
    " a threshold that classifies eigenvalues as belonging to signal of interest vs. random thermal noise"
    " is based on the prior knowledge that the eigenspectrum of random covariance matrices"
    " is described by the universal Marchenko-Pastur (MP) distribution."

  + "This command includes many capabilities absent from the original dwidenoise command. "
    "These include:"
    " - Multiple sliding window kernel shapes,"
      " including a spherical kernel that dilates at image edges to preserve aspect ratio;"
    " - A greater number of mechanisms for noise level estimation,"
      " including taking a pre-estimated noise map as input;"
    " - Preconditioning, including (per-shell) demeaning,"
      " phase demodulation (linear or nonlinear),"
      " and variance-stabilising transform to compensate for within-patch heteroscedasticity;"
    " - Overcomplete local PCA;"
    " - Subsampling (performing fewer PCAs than there are input voxels);"
    " - Optimal shrinkage of eigenvalues."

  + Denoise::first_step_description

  + Denoise::non_gaussian_noise_description

  + demodulation_description

  + Kernel::shape_description

  + Kernel::default_size_description

  + Kernel::cuboid_size_description

  + Denoise::filter_description

  + Denoise::aggregation_description;

  EXAMPLES
  + Example("To approximately replicate the behaviour of the original dwidenoise command",
            "dwidenoise2 DWI.mif out.mif -shape cuboid -subsample 1 -demodulate none -demean none -filter truncate -aggregator exclusive",
            "While this is neither guaranteed to match exactly the output of the original dwidenoise command"
            " nor is it a recommended use case,"
            " it may nevertheless be informative in demonstrating those advanced features of dwidenoise2 active by default"
            " that must be explicitly disabled in order to approximate that behaviour.")

  + Example("Denoising multi-echo fMRI data",
            "mrcat original_TE1.mif original_TE2.mif original_TE3.mif original_all.mif -axis 4;"
            " dwidenoise2 original_all.mif denoised_all.mif;"
            " for_each 1 2 3 : mrconvert denoised_all.mif denoised_TEIN.mif -coord 4 IN -axes 0,1,2,3",
            "By concatenating echoes along a new fifth image axis,"
            " the program will by default calculate the mean intensity per echo "
            " and regress this from the data prior to PCA; "
            " this should reduce the signal rank and increase PCA precision.");

  REFERENCES
  + "Veraart, J.; Novikov, D.S.; Christiaens, D.; Ades-aron, B.; Sijbers, J. & Fieremans, E. " // Internal
    "Denoising of diffusion MRI using random matrix theory. "
    "NeuroImage, 2016, 142, 394-406, doi: 10.1016/j.neuroimage.2016.08.016"

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

  + "* If using anything other than -aggregation exclusive: "
    "Manjon, J.V.; Coupe, P.; Concha, L.; Buades, A.; D. Collins, D.L.; Robles, M. "
    "Diffusion Weighted Image Denoising Using Overcomplete Local PCA. "
    "PLoS ONE, 2013, 8(9), e73021"

  + "* If using -estimator med or -filter optthresh: "
    "Gavish, M.; Donoho, D.L."
    "The Optimal Hard Threshold for Singular Values is 4/sqrt(3). "
    "IEEE Transactions on Information Theory, 2014, 60(8), 5040-5053.";

  ARGUMENTS
  + Argument("dwi", "the input diffusion-weighted image.").type_image_in()
  + Argument("out", "the output denoised DWI image.").type_image_out();

  OPTIONS
  + OptionGroup("Options for modifying PCA computations")
  + datatype_option
  + Estimator::estimator_denoise_options
  + Kernel::options
  + subsample_option
  + precondition_options(true)

  + Option("iterative",
           "EXPERIMENTAL; perform iterative refinement of noise level estimation"
           " prior to final denoising step")

  + OptionGroup("Options that affect reconstruction of the output image series")
  + Option("filter",
           "Modulate how component contributions are filtered "
           "based on the cumulative eigenvalues relative to the noise level; "
           "options are: " + join(filters, ",") + "; "
           "default: optshrink (Optimal Shrinkage based on minimisation of the Frobenius norm)")
    + Argument("choice").type_choice(filters)
  + Option("aggregator",
           "Select how the outcomes of multiple PCA outcomes centred at different voxels "
           "contribute to the reconstructed DWI signal in each voxel; "
           "options are: " + join(aggregators, ",") + "; default: Gaussian")
    + Argument("choice").type_choice(aggregators)
  // TODO For specifically the Gaussian aggregator,
  //   should ideally be possible to select the FWHM of the aggregator

  + OptionGroup("Options for exporting additional data regarding PCA behaviour")
  + Option("noise_out",
           "The output noise map,"
           " i.e., the estimated noise level 'sigma' in the data. "
           "Note that on complex input data,"
           " this will be the total noise level across real and imaginary channels,"
           " so a scale factor sqrt(2) applies.")
    + Argument("image").type_image_out()
  + Option("lamplus",
           "The estimated upper bound of the noise portion of the eigenspectrum (\"lambda-plus\")")
    + Argument("image").type_image_out()
  + Option("rank_pcanonzero",
           "The (non-zero) rank of the PCA decomposition for each patch absent any denoising")
    + Argument("image").type_image_out()
  + Option("rank_input",
           "The estimated rank of the input data for each denoising patch")
    + Argument("image").type_image_out()
  + Option("rank_output",
           "An estimated rank for the output image data, accounting for multi-patch aggregation")
    + Argument("image").type_image_out()
  + Option("variance_removed",
           "the fraction of variance removed in producing the output denoised data "
           "(note that this applies strictly to the PCA and omits effects of preconditioning)")
    + Argument("image").type_image_out()
  + Option("eigenspectra",
           "Output a matrix containing the spectra of eigenvalues across patches")
    + Argument("file").type_file_out()

  + OptionGroup("Options for debugging the operation of sliding window kernels")
  + Option("max_dist",
           "The maximum distance between a voxel and another voxel that was included in the local denoising patch")
    + Argument("image").type_image_out()
  + Option("voxelcount",
           "The number of voxels that contributed to the PCA for processing of each voxel")
    + Argument("image").type_image_out()
  + Option("patchcount",
           "The number of unique patches to which an image voxel contributes")
    + Argument("image").type_image_out()
  + Option("sum_aggregation",
           "The sum of aggregation weights of those patches contributing to each output voxel")
    + Argument("image").type_image_out()
  + Option("sum_optshrink",
           "the sum of eigenvector weights computed for the denoising patch centred at each voxel "
           "as a result of performing optimal shrinkage")
    + Argument("image").type_image_out()

  + DWI::GradImportOptions();

}
// clang-format on

// Necessary to allow normalisation by sum of aggregation weights
//   where the image type is cdouble, but aggregation weights are float
// (operations combining complex & real types not allowed to be of different precision)
std::complex<double> operator/(const std::complex<double> &c, const float n) { return c / double(n); }

const std::vector<Iterative::Iteration> default_iterations({{{8, 8, 8}, 16.0, false},  //
                                                            {{4, 4, 4}, 4.0, false},   //
                                                            {{2, 2, 2}, 1.0, true},    //
                                                            {{2, 2, 2}, 1.0, false}}); //

template <typename T>
void run(Header &dwi,
         const Demodulation &demodulation,
         const demean_type demean,
         Image<float> &user_vst_image,
         const std::vector<Iterative::Iteration> &iterations,
         std::shared_ptr<Estimator::Base> estimator,
         filter_type filter,
         aggregator_type aggregator,
         const std::string &output_name,
         Exports &final_exports) {

  Image<T> input(dwi.get_image<T>());
  Image<bool> mask = generate_mask(input);
  Image<float> vst_image(user_vst_image);

  Precondition<T> preconditioner(input, demodulation, demean, user_vst_image);
  Image<T> input_preconditioned =
      Image<T>::scratch(preconditioner.header(), "Preconditioned version of \"" + dwi.name() + "\"");

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
    vst_image = iteration_exports.noise_out;
    preconditioner.update_vst_image(vst_image);
    estimator->update_vst_image(vst_image);
  }

  auto subsample = Subsample::make(dwi, default_subsample_ratio);

  // Implementation from here differs to that of dwi2noise
  auto kernel = Kernel::make_kernel(input, subsample->get_factors(), iterations.back().kernel_size_multiplier);
  kernel->set_mask(mask);

  // If we're doing an iterative optimisation,
  //   then in the final iteration we assume that the variance-stabilising transform
  //   results in a noise level of 1.0 everywhere
  if (iterations.size() > 1)
    estimator.reset(new Estimator::Unity());

  auto opt = get_options("preconditioned_input");
  if (!opt.empty())
    input_preconditioned = Image<T>::create(opt[0][0], preconditioner.header());
  preconditioner(input, input_preconditioned, false);
  Recon<T> func(input_preconditioned,
                subsample,
                kernel,
                estimator,
                filter,
                aggregator,
                final_exports,
                preconditioner.null_rank());

  Image<T> output = Image<T>::create(output_name, dwi);
  Image<T> output_preconditioned;
  opt = get_options("preconditioned_output");
  if (!opt.empty())
    output_preconditioned = Image<T>::create(opt[0][0], preconditioner.header());
  // If we can make use of the output image for preconditioned denoised data
  //   rather than explicitly allocating an intermediate scratch buffer for that purpose,
  //   then do so
  else if (DataType::from<T>() == dwi.datatype() && dwi.ndim() == 4)
    output_preconditioned = output;
  else
    output_preconditioned = Image<T>::scratch(                              //
        preconditioner.header(),                                            //
        "Scratch buffer for denoised data before undoing preconditioning"); //

  const bool final_iteration_includes_MP =
      iterations.size() == 1 && get_options("noise_in").empty() && get_options("fixed_rank").empty();
  ThreadedLoop(final_iteration_includes_MP                                 //
                   ? "running MP-PCA noise level estimation and denoising" //
                   : "running PCA denoising",                              //
               input_preconditioned,                                       //
               0,                                                          //
               3)                                                          //
      .run(func, input_preconditioned, output_preconditioned);             //

  // Rescale output if aggregation was performed
  if (aggregator != aggregator_type::EXCLUSIVE) {
    for (auto l_voxel = Loop(final_exports.sum_aggregation)      //
         (output_preconditioned, final_exports.sum_aggregation); //
         l_voxel;                                                //
         ++l_voxel) {                                            //
      for (auto l_volume = Loop(3)(output_preconditioned); l_volume; ++l_volume)
        output_preconditioned.value() /= double(final_exports.sum_aggregation.value());
    }
    if (final_exports.rank_output.valid()) {
      for (auto l = Loop(final_exports.sum_aggregation)                //
           (final_exports.rank_output, final_exports.sum_aggregation); //
           l;                                                          //
           ++l)                                                        //
        final_exports.rank_output.value() /= final_exports.sum_aggregation.value();
    }
  }

  // reverse effects of preconditioning
  preconditioner(output_preconditioned, output, true);

  // Modify some optional outputs to better reflect utilisation of preconditioning
  const ssize_t preconditioner_null_rank = preconditioner.null_rank();
  if (preconditioner_null_rank > 0) {
    if (final_exports.rank_input.valid()) {
      for (auto l = Loop(final_exports.rank_input)(final_exports.rank_input); l; ++l) {
        if (final_exports.rank_input.value() > 0)
          final_exports.rank_input.value() =                                                                      //
              std::min<uint16_t>(uint16_t(final_exports.rank_input.value()) + uint16_t(preconditioner_null_rank), //
                                 uint16_t(dwi.size(3)));                                                          //
      }
    }
    if (final_exports.rank_output.valid()) {
      for (auto l = Loop(final_exports.rank_output)(final_exports.rank_output); l; ++l)
        final_exports.rank_output.value() =                                                             //
            std::min<float>(float(final_exports.rank_output.value()) + float(preconditioner_null_rank), //
                            float(dwi.size(3)));                                                        //
    }
  }
  if (vst_image.valid() && final_exports.noise_out.valid()) {
    Interp::Linear<Image<float>> vst_interp(vst_image);
    const Transform transform(subsample->header());
    for (auto l = Loop(final_exports.noise_out)(final_exports.noise_out); l; ++l) {
      vst_interp.scanner(transform.voxel2scanner * Eigen::Vector3d({double(final_exports.noise_out.index(0)),
                                                                    double(final_exports.noise_out.index(1)),
                                                                    double(final_exports.noise_out.index(2))}));
      final_exports.noise_out.value() *= vst_interp.value();
    }
  }
}

void run() {
  auto dwi = Header::open(argument[0]);
  if (dwi.ndim() < 4)
    throw Exception("input image must be at least 4-dimensional");
  ssize_t intensities_per_voxel = dwi.size(3);
  for (ssize_t axis = 4; axis != dwi.ndim(); ++axis)
    intensities_per_voxel *= dwi.size(axis);
  if (intensities_per_voxel == 1)
    throw Exception("input image must be non-singleton across non-spatial dimensions");

  const Demodulation demodulation = select_demodulation(dwi);
  const demean_type demean = select_demean(dwi);

  Image<float> user_vst_image;
  auto opt = get_options("vst");
  if (!opt.empty()) {
    if (demean == demean_type::NONE) {
      WARN("Application of variance-stabilising transform in the absence of demeaning may be erroneous");
    }
    user_vst_image = Image<float>::open(opt[0][0]);
    if (user_vst_image.ndim() != 3)
      throw Exception("Variance-stabilising transform noise level image must be 3D");
  }

  aggregator_type aggregator = aggregator_type::GAUSSIAN;
  opt = get_options("aggregator");
  if (!opt.empty())
    aggregator = aggregator_type(int(opt[0][0]));

  auto final_subsample = Subsample::make(dwi, aggregator == aggregator_type::EXCLUSIVE ? 1 : default_subsample_ratio);
  assert(final_subsample);
  if (aggregator == aggregator_type::EXCLUSIVE &&
      *std::max_element(final_subsample->get_factors().begin(), final_subsample->get_factors().end()) > 1) {
    WARN("Utilising subsampling in conjunction with -aggregator exclusive "
         "will result in an output image with holes, "
         "as not all input voxels will have their own patch");
  }

  auto estimator = Estimator::make_estimator(user_vst_image, true);

  filter_type filter = get_options("fixed_rank").empty() ? filter_type::OPTSHRINK : filter_type::TRUNCATE;
  opt = get_options("filter");
  if (!opt.empty())
    filter = filter_type(int(opt[0][0]));

  Exports final_exports(dwi, final_subsample->header());
  opt = get_options("noise_out");
  if (!opt.empty())
    final_exports.set_noise_out(opt[0][0]);
  opt = get_options("lamplus");
  if (!opt.empty())
    final_exports.set_lamplus(opt[0][0]);
  opt = get_options("rank_pcanonzero");
  if (!opt.empty())
    final_exports.set_rank_pcanonzero(opt[0][0]);
  opt = get_options("rank_input");
  if (!opt.empty())
    final_exports.set_rank_input(opt[0][0]);
  opt = get_options("rank_output");
  if (!opt.empty()) {
    if (aggregator == aggregator_type::EXCLUSIVE && filter == filter_type::TRUNCATE) {
      WARN("When using -aggregator exclusive and -filter truncate, "
           "the output of -rank_output will be identical to the output of -rank_input, "
           "as there is no aggregation of multiple patches per output voxel "
           "and no optimal shrinkage to reduce output rank relative to estimated input rank");
    }
    final_exports.set_rank_output(opt[0][0]);
  }
  opt = get_options("sum_optshrink");
  if (!opt.empty()) {
    if (filter == filter_type::TRUNCATE) {
      WARN("Note that with a truncation filter, "
           "output image from -sumweights option will be equivalent to rank_input");
    }
    final_exports.set_sum_optshrink(opt[0][0]);
  }
  opt = get_options("max_dist");
  if (!opt.empty())
    final_exports.set_max_dist(opt[0][0]);
  opt = get_options("voxelcount");
  if (!opt.empty())
    final_exports.set_voxelcount(opt[0][0]);
  opt = get_options("patchcount");
  if (!opt.empty())
    final_exports.set_patchcount(opt[0][0]);
  opt = get_options("sum_aggregation");
  if (!opt.empty()) {
    if (aggregator == aggregator_type::EXCLUSIVE) {
      WARN("Output from -sum_aggregation will just contain 1 for every voxel processed: "
           "no patch aggregation takes place when output series comes exclusively from central patch");
    }
    final_exports.set_sum_aggregation(opt[0][0]);
  } else if (aggregator != aggregator_type::EXCLUSIVE) {
    final_exports.set_sum_aggregation();
  }
  opt = get_options("variance_removed");
  if (!opt.empty())
    final_exports.set_variance_removed(opt[0][0]);
  opt = get_options("eigenspectra");
  if (!opt.empty())
    final_exports.set_eigenspectra_path(opt[0][0]);

  std::vector<Iterative::Iteration> iterations;
  if (get_options("iterative").empty()) {
    Iterative::Iteration config;
    config.subsample_ratios = final_subsample->get_factors();
    config.kernel_size_multiplier = 1.0;
    config.smooth_noiseout = false;
    iterations.push_back(config);
  } else {
    if (!get_options("subsample").empty())
      throw Exception("Implementation does not support use of both -iterative and -subsample");
    if (!get_options("noise_in").empty())
      throw Exception("Options -iterative and -noise_in are mutually exclusive");
    if (!get_options("fixed_rank").empty())
      throw Exception("Options -iterative and -fixed_rank are mutually exclusive");
    if (demean == demean_type::NONE) {
      WARN("Use of both -iterative and -demean none should be treated with scepticism,"
           " as correction of heteroscedasticity will introduce false tissue contrast");
    }
    iterations = default_iterations;
  }

  int prec = get_option_value("datatype", 0); // default: single precision
  if (dwi.datatype().is_complex())
    prec += 2; // support complex input data
  switch (prec) {
  case 0:
    assert(demodulation.axes.empty());
    INFO("select real float32 for processing");
    run<float>(         //
        dwi,            //
        demodulation,   //
        demean,         //
        user_vst_image, //
        iterations,     //
        estimator,      //
        filter,         //
        aggregator,     //
        argument[1],    //
        final_exports); //
    break;
  case 1:
    assert(demodulation.axes.empty());
    INFO("select real float64 for processing");
    run<double>(        //
        dwi,            //
        demodulation,   //
        demean,         //
        user_vst_image, //
        iterations,     //
        estimator,      //
        filter,         //
        aggregator,     //
        argument[1],    //
        final_exports); //
    break;
  case 2:
    INFO("select complex float32 for processing");
    run<cfloat>(        //
        dwi,            //
        demodulation,   //
        demean,         //
        user_vst_image, //
        iterations,     //
        estimator,      //
        filter,         //
        aggregator,     //
        argument[1],    //
        final_exports); //
    break;
  case 3:
    INFO("select complex float64 for processing");
    run<cdouble>(       //
        dwi,            //
        demodulation,   //
        demean,         //
        user_vst_image, //
        iterations,     //
        estimator,      //
        filter,         //
        aggregator,     //
        argument[1],    //
        final_exports); //
    break;
  }
}
