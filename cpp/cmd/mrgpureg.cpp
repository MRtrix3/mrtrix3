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

#include "adapter/reslice.h"
#include "algo/threaded_copy.h"
#include "app.h"
#include "cmdline_option.h"
#include "command.h" // IWYU pragma: keep
#include "datatype.h"
#include "exception.h"
#include "file/matrix.h"
#include "filter/reslice.h"
#include "filter/warp.h"
#include "gpu/gpu.h"
#include "gpu/registration/eigenhelpers.h"
#include "gpu/registration/globalregistration.h"
#include "gpu/registration/imageoperations.h"
#include "gpu/registration/nonlinearregistration.h"
#include "gpu/registration/registrationtypes.h"
#include "header.h"
#include "image.h"
#include "image_helpers.h"
#include "interp/cubic.h"
#include "magic_enum/magic_enum.hpp"
#include "mrtrix.h"
#include "types.h"

#include <Eigen/Core>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace MR;
using namespace App;

namespace {

template <typename Enum> std::vector<std::string> lowercase_enum_names() {
  static constexpr auto names_view = magic_enum::enum_names<Enum>();
  std::vector<std::string> result;
  result.reserve(names_view.size());
  for (const auto &s : names_view) {
    result.push_back(MR::lowercase(std::string(s)));
  }
  return result;
}
template <typename Enum> std::string enum_name_lowercase(Enum e) {
  auto name = magic_enum::enum_name(e);
  return MR::lowercase(std::string(name));
}

template <typename Enum> Enum from_name(std::string_view name) {
  auto e = magic_enum::enum_cast<Enum>(name, magic_enum::case_insensitive);
  if (!e.has_value()) {
    std::string error = "Unsupported value '" + std::string(name) + "'. Supported values are: ";
    const auto names = lowercase_enum_names<Enum>();
    for (const auto &n : names) {
      error += n + ", ";
    }
    throw Exception(error);
  }
  return e.value();
}

constexpr float default_max_search_angle = 45.0F;
constexpr GlobalRegistrationType default_global_registration_type = GlobalRegistrationType::Affine;
constexpr MetricType default_metric_type = MetricType::NMI;
constexpr uint32_t default_ncc_window_radius = 2U;
constexpr uint32_t default_max_iterations = 500;
const std::vector<std::string> supported_global_metric_types = lowercase_enum_names<MetricType>();
const std::vector<std::string> supported_nonlinear_metric_types = {"ssd", "ncc"};
const std::vector<std::string> supported_registration_modes = {
    "rigid", "affine", "nonlinear", "rigid_affine", "rigid_affine_nonlinear"};
const std::vector<std::string> supported_init_translations = lowercase_enum_names<InitTranslationChoice>();
const std::vector<std::string> supported_init_rotations = lowercase_enum_names<InitRotationChoice>();

struct RegistrationStages {
  bool run_rigid = false;
  bool run_affine = false;
  bool run_nonlinear = false;

  bool has_global_registration() const { return run_rigid || run_affine; }
};

RegistrationStages parse_registration_stages(std::string_view name) {
  const auto mode = MR::lowercase(std::string(name));
  if (mode == "rigid") {
    return RegistrationStages{.run_rigid = true};
  }
  if (mode == "affine") {
    return RegistrationStages{.run_affine = true};
  }
  if (mode == "nonlinear") {
    return RegistrationStages{.run_nonlinear = true};
  }
  if (mode == "rigid_affine") {
    return RegistrationStages{.run_rigid = true, .run_affine = true};
  }
  if (mode == "rigid_affine_nonlinear") {
    return RegistrationStages{.run_rigid = true, .run_affine = true, .run_nonlinear = true};
  }

  std::string error = "Unsupported value '" + std::string(name) + "'. Supported values are: ";
  for (const auto &supported_mode : supported_registration_modes) {
    error += supported_mode + ", ";
  }
  throw Exception(error);
}

GlobalMetric make_global_metric(MetricType metric_type, uint32_t ncc_window_radius) {
  switch (metric_type) {
  case MetricType::NMI:
    return NMIMetric{};
  case MetricType::SSD:
    return SSDMetric{};
  case MetricType::NCC:
    return NCCMetric{.window_radius = ncc_window_radius};
  default:
    throw Exception("Unsupported metric type");
  }
}

NonLinearMetric make_nonlinear_metric(MetricType metric_type, uint32_t ncc_window_radius) {
  switch (metric_type) {
  case MetricType::SSD:
    return SSDMetric{};
  case MetricType::NCC:
    if (ncc_window_radius == 0U) {
      throw Exception("Non-linear registration with NCC metric requires ncc_radius >= 1.");
    }
    return NCCMetric{.window_radius = ncc_window_radius};
  default:
    throw Exception("Unsupported nonlinear metric type");
  }
}

} // namespace

// clang-format off
 // NOLINTBEGIN(readability-implicit-bool-conversion)
 void usage() {
    AUTHOR = "Daljit Singh",
    SYNOPSIS = "GPU driven non-linear registration computes warps to map from both image1->image2 and image2->image1."
              " Based on Symmetric Log-Domain Diffeomorphic Registration: A Demons-based Approach by Vercauteren et al (2009)."
              " Warps are output as deformation fields, where warp1 can be used to transform image1->image2 and warp2 to transform image2->image1."
              " The algorithm support both LNCC (local normalized cross correlation) and SSD (sum of squared differences) metrics for non-linear registration.";

    REFERENCES
        + "Vercauteren, T., Pennec, X., Perchant, A., & Ayache, N. (2009). "
          "Symmetric log-domain diffeomorphic registration: A demons-based approach. "
          "International conference on medical image computing and computer-assisted intervention, pp. 754-761. ";
                
     ARGUMENTS
         + Argument ("image1 image2", "input image 1 ('moving') and input image 2 ('template')").type_image_in()
         + Argument ("contrast1 contrast2",
                    "optional list of additional input images used as additional contrasts."
                    " Can be used multiple times."
                    " contrastX and imageX must share the same coordinate system.").type_image_in().optional().allow_multiple();

     OPTIONS
         + Option ("transformed", "image1 transformed to image2 space after registration."
                                " Note that -transformed needs to be repeated for each contrast.")
             .allow_multiple()
             + Argument("image").type_image_out().optional()

         + Option ("nl_warp",
                   "the non-linear warp output defined as two deformation fields,"
                   " where warp1 can be used to transform image1->image2"
                   " and warp2 to transform image2->image1."
                   " The deformation fields also encapsulate any linear transformation"
                   " estimated prior to non-linear registration.")
             + Argument("warp1").type_image_out()
             + Argument("warp2").type_image_out()

         + Option ("matrix", "write the transformation matrix used for reslicing image1 into image2 space.")
             + Argument("filename").type_file_out()

         + Option ("type", "type of transform (rigid, affine, nonlinear, rigid_affine, rigid_affine_nonlinear)")
             + Argument("name").type_choice(supported_registration_modes)

         + Option ("global_metric", "similarity metric to use for rigid/affine registrations (nmi, ssd, ncc)")
             + Argument("name").type_choice(supported_global_metric_types)

        + Option ("nl_metric", "similarity metric to use for nonlinear registration (ssd, ncc)")
             + Argument("name").type_choice(supported_nonlinear_metric_types)

         // TODO: Should we mention that using a large window radius (> 3) is not recommended
         // as it's computationally expensive and usually does not improve results?
         + Option("ncc_radius",
              "window radius (in voxels) for the NCC metric; set to 0 for global NCC (default: " +
                std::to_string(default_ncc_window_radius) + ").")
           + Argument("radius").type_integer(0, 15)

         + Option("mask1", "a mask to define the region of image1 to use for optimisation.")
             + Argument("filename").type_image_in()

         + Option("mask2", "a mask to define the region of image2 to use for optimisation.")
             + Argument("filename").type_image_in()

         + Option("max_iter", "maximum number of iterations (default: " + std::to_string(default_max_iterations) + ")")
             + Argument("number").type_integer(10, 1000)

         + Option("nl_fluid_sigma",
                  "Gaussian sigma (in voxel units) for fluid-like smoothing of the nonlinear update field"
                  " (default: " + std::to_string(default_nonlinear_fluid_blur_sigma_voxels) + ")."
                  " Set to 0 to disable fluid smoothing.")
             + Argument("sigma").type_float(0.0)

         + Option("nl_diffusion_sigma",
                  "Gaussian sigma (in voxel units) for diffusion-like smoothing of the nonlinear velocity field"
                  " (default: " + std::to_string(default_nonlinear_diffusion_blur_sigma_voxels) + ")."
                  " Set to 0 to disable diffusion smoothing.")
             + Argument("sigma").type_float(0.0)

         + Option("nl_ncc_update_target_voxels",
                  "For nonlinear registration, target maximum LNCC update magnitude in voxel units used"
                  " when renormalising the LNCC update field before the update is applied"
                  " (default: " + std::to_string(default_nonlinear_ncc_update_target_voxels) + ")."
                  " Larger values imply more aggressive updates, which can increase the risk of overshooting.")
             + Argument("rate").type_float(0.0)

         + Option("nl_bch_terms",
                  "Number of Baker-Campbell-Hausdorff terms to retain when composing each nonlinear update."
                  " (default: " + std::to_string(default_nonlinear_svf_bch_terms) + ").")
             + Argument("terms").type_integer(1, 2)

         + Option("init_translation",
                  "initialise the translation and centre of rotation;"
                  " Valid choices are:"
                  " mass (aligns the centers of mass of both images, default);"
                  " geometric (aligns geometric image centres);"
                  " none.")
             + Argument("type").type_choice(supported_init_translations)

         + Option("init_rotation",
                  "Method to use to initialise the rotation."
                  " Valid choices are:"
                  " search (search for the best rotation using the selected metric);"
                  " moments (rotation based on directions of intensity variance with respect to centre of mass);"
                  " none (default).")
             + Argument("type").type_choice(supported_init_rotations)

            + Option("init_rotation_max_angle",
                   "Maximum rotation angle (degrees) to sample when init_rotation=search (default: " + std::to_string(default_max_search_angle) +
                   " Use a larger value only when images may be grossly misaligned.")
               + Argument("degrees").type_float(0.0, 180.0)

         + Option("init_matrix",
                  "initialise either the registration with the supplied transformation matrix "
                  "(as a 4x4 matrix in scanner coordinates). "
                  "Note that this overrides init_translation and init_rotation initialisation")
             + Argument("filename").type_file_in()
         + Option ("mc_weights", "relative weight of images used for multi-contrast registration. Default: 1.0 (equal weighting)")
             + Argument ("weights").type_sequence_float ();
 }
 // NOLINTEND(readability-implicit-bool-conversion)

// clang-format on

struct HeaderPair {
  Header header1;
  Header header2;
};

struct ImagePair {
  Image<float> image1;
  Image<float> image2;
};

void run() {
  auto gpu_context_request = GPU::ComputeContext::request_async();
  std::vector<HeaderPair> header_pairs;
  const size_t arg_size = argument.size();
  if (arg_size % 2 != 0 || arg_size < 2) {
    const auto error = MR::join(argument, " ");
    throw Exception("Unexpected number of input images, arguments: " + error);
  }

  for (size_t i = 0; i < arg_size; i += 2) {
    header_pairs.push_back({Header::open(argument[i]), Header::open(argument[i + 1])});
  }

  for (const auto &[header1, header2] : header_pairs) {
    if (header1.ndim() != header2.ndim()) {
      throw Exception("Input images " + header1.name() + " and " + header2.name() +
                      " have different number of dimensions: " + std::to_string(header1.ndim()) + " and " +
                      std::to_string(header2.ndim()));
    }
    check_3D_nonunity(header1);
    check_3D_nonunity(header2);
  }

  const std::string registration_mode_name =
      get_option_value<std::string>("type", enum_name_lowercase(default_global_registration_type));
  const RegistrationStages registration_stages = parse_registration_stages(registration_mode_name);
  const bool has_nonlinear_registration = registration_stages.run_nonlinear;
  const bool has_global_registration = registration_stages.has_global_registration();

  const auto global_metric_options = get_options("global_metric");
  const auto nl_metric_options = get_options("nl_metric");
  const auto nl_fluid_sigma_options = get_options("nl_fluid_sigma");
  const auto nl_diffusion_sigma_options = get_options("nl_diffusion_sigma");
  const auto nl_ncc_update_target_voxels_options = get_options("nl_ncc_update_target_voxels");
  const auto nl_bch_terms_options = get_options("nl_bch_terms");

  if (!has_nonlinear_registration && !nl_metric_options.empty()) {
    throw Exception("nl_metric is only valid when using nonlinear registration.");
  }
  if (!has_nonlinear_registration && !nl_fluid_sigma_options.empty()) {
    throw Exception("nl_fluid_sigma is only valid when using nonlinear registration.");
  }
  if (!has_nonlinear_registration && !nl_diffusion_sigma_options.empty()) {
    throw Exception("nl_diffusion_sigma is only valid when using nonlinear registration.");
  }
  if (!has_nonlinear_registration && !nl_ncc_update_target_voxels_options.empty()) {
    throw Exception("nl_ncc_update_target_voxels is only valid when using nonlinear registration.");
  }
  if (!has_nonlinear_registration && !nl_bch_terms_options.empty()) {
    throw Exception("nl_bch_terms is only valid when using nonlinear registration.");
  }
  const auto nl_warp_option = get_options("nl_warp");
  if (!has_nonlinear_registration && !nl_warp_option.empty()) {
    throw Exception("nl_warp output is only valid when using nonlinear registration.");
  }

  MetricType metric_type = default_metric_type;
  if (has_global_registration) {
    metric_type =
        from_name<MetricType>(get_option_value<std::string>("global_metric", enum_name_lowercase(default_metric_type)));
  }

  std::optional<MetricType> nonlinear_metric_type;
  if (has_nonlinear_registration) {
    const std::string default_nonlinear_metric = "ncc";
    nonlinear_metric_type = from_name<MetricType>(get_option_value<std::string>("nl_metric", default_nonlinear_metric));
    if (*nonlinear_metric_type != MetricType::NCC && !nl_ncc_update_target_voxels_options.empty()) {
      throw Exception(
          "nl_ncc_update_target_voxels is only valid when using nonlinear registration with nl_metric ncc.");
    }
  }

  const uint32_t ncc_window_radius = get_option_value<uint32_t>("ncc_radius", default_ncc_window_radius);
  const float nonlinear_fluid_sigma =
      get_option_value<float>("nl_fluid_sigma", default_nonlinear_fluid_blur_sigma_voxels);
  const float nonlinear_diffusion_sigma =
      get_option_value<float>("nl_diffusion_sigma", default_nonlinear_diffusion_blur_sigma_voxels);
  const float nonlinear_ncc_update_target_voxels =
      get_option_value<float>("nl_ncc_update_target_voxels", default_nonlinear_ncc_update_target_voxels);
  const uint32_t nonlinear_svf_bch_terms = get_option_value<uint32_t>("nl_bch_terms", default_nonlinear_svf_bch_terms);

  std::optional<Image<float>> mask1;
  std::optional<Image<float>> mask2;
  const auto mask1_option = get_options("mask1");
  const auto mask2_option = get_options("mask2");
  if (!mask1_option.empty()) {
    mask1 = Image<float>::open(mask1_option[0][0]);
  }
  if (!mask2_option.empty()) {
    mask2 = Image<float>::open(mask2_option[0][0]);
  }
  if (mask1) {
    if (mask1->ndim() != 3) {
      throw Exception("mask1 must be a 3D image.");
    }
    check_dimensions(*mask1, header_pairs.front().header1, 0, 3);
  }
  if (mask2) {
    if (mask2->ndim() != 3) {
      throw Exception("mask2 must be a 3D image.");
    }
    check_dimensions(*mask2, header_pairs.front().header2, 0, 3);
  }

  const uint32_t max_iterations = get_option_value<uint32_t>("max_iter", default_max_iterations);

  const auto init_matrix_option = get_options("init_matrix");

  std::optional<InitialGuess> initial_guess;
  std::optional<MR::transform_type> initial_affine;
  if (!init_matrix_option.empty()) {
    // TODO: compute centre from images. Also check what's the correct thing to do in this case.
    Eigen::Vector3d centre;
    const MR::transform_type init_transform = File::Matrix::load_transform(init_matrix_option[0][0], centre);
    if (has_global_registration) {
      initial_guess = init_transform;
    }
    initial_affine = init_transform;
  } else if (has_global_registration) {
    const InitTranslationChoice init_translation =
        from_name<InitTranslationChoice>(get_option_value<std::string>("init_translation", "mass"));
    const InitRotationChoice init_rotation =
        from_name<InitRotationChoice>(get_option_value<std::string>("init_rotation", "none"));
    const float init_rotation_max_angle = get_option_value<float>("init_rotation_max_angle", default_max_search_angle);
    initial_guess = InitialisationOptions{
        .translation_choice = init_translation,
        .rotation_choice = init_rotation,
        .cost_metric = make_global_metric(metric_type, ncc_window_radius),
        .max_search_angle_degrees = init_rotation_max_angle,
    };
  }

  // TODO: we only support 3D images for now. We'll need to extend this to
  // support 4D images later.
  for (const auto &[header1, header2] : header_pairs) {
    check_dimensions(header1, header_pairs.front().header1, 0, 3);
    check_dimensions(header2, header_pairs.front().header2, 0, 3);

    if (header1.ndim() != 3 || header2.ndim() != 3) {
      throw Exception("Input images with dimensionality other than 3 are not supported.");
    }
  }

  const auto weight_options = get_options("mc_weights");
  std::vector<default_type> mc_weights;
  if (!weight_options.empty()) {
    mc_weights = parse_floats(weight_options[0][0]);
    if (mc_weights.size() == 1) {
      mc_weights.resize(header_pairs.size(), mc_weights[0]);
    } else if (mc_weights.size() != header_pairs.size()) {
      throw Exception("number of mc_weights does not match number of contrasts");
    }
    const bool weights_positive = std::all_of(mc_weights.begin(), mc_weights.end(), [](auto w) { return w >= 0.0; });
    if (!weights_positive) {
      throw Exception("mc_weights must be non-negative");
    }
  }

  std::vector<ChannelConfig> channels;
  size_t index = 0;
  for (const auto &[header1, header2] : header_pairs) {
    const ChannelConfig channel{
        .image1 = Image<float>::open(header1.name()).with_direct_io(),
        .image2 = Image<float>::open(header2.name()).with_direct_io(),
        .image1Mask = mask1,
        .image2Mask = mask2,
        .weight = static_cast<float>(mc_weights.empty() ? 1.0F : mc_weights[index]),
    };

    channels.push_back(channel);
    ++index;
  }

  const std::string matrix_filename = get_option_value<std::string>("matrix", "");
  if (has_nonlinear_registration && !matrix_filename.empty()) {
    throw Exception("Matrix outputs are not yet supported when using nonlinear registration.");
  }

  const auto transformed_option = get_options("transformed");
  std::vector<std::filesystem::path> transformed_filenames;
  if (!transformed_option.empty()) {
    if (transformed_option.size() > header_pairs.size()) {
      throw Exception("Number of -transformed images exceeds number of contrasts");
    }
    if (transformed_option.size() < header_pairs.size()) {
      WARN("Number of -transformed images is less than number of contrasts.");
    }
    for (size_t i = 0; i < transformed_option.size(); ++i) {
      const std::filesystem::path output_path(transformed_option[i][0]);
      transformed_filenames.push_back(output_path);
      const auto input1_path = std::filesystem::path(header_pairs[i].header1.name());
      INFO(input1_path.filename().string() + ", transformed to space of image2, will be saved to " +
           output_path.string());
    }
  }

  std::string warp1_filename;
  std::string warp2_filename;
  if (!nl_warp_option.empty()) {
    warp1_filename = std::string(nl_warp_option[0][0]);
    warp2_filename = std::string(nl_warp_option[0][1]);
  }

  std::optional<GlobalMetric> global_metric;
  if (has_global_registration) {
    global_metric = make_global_metric(metric_type, ncc_window_radius);
  }

  std::optional<NonLinearMetric> nonlinear_metric;
  if (has_nonlinear_registration) {
    nonlinear_metric = make_nonlinear_metric(*nonlinear_metric_type, ncc_window_radius);
  }

  auto gpu_compute_context = gpu_context_request.get();
  std::optional<RegistrationResult> registration_result;

  if (registration_stages.run_rigid) {
    if (!initial_guess || !global_metric) {
      throw Exception("Rigid registration could not be initialised.");
    }
    CONSOLE("Running rigid registration stage.");
    const GlobalRegistrationConfig rigid_config{
        .channels = channels,
        .transformation_type = GlobalRegistrationType::Rigid,
        .initial_guess = *initial_guess,
        .metric = *global_metric,
        .max_iterations = max_iterations,
    };
    registration_result = GPU::run_registration(rigid_config, gpu_compute_context);
  }

  if (registration_stages.run_affine) {
    if (!global_metric || (!registration_result && !initial_guess)) {
      throw Exception("Affine registration could not be initialised.");
    }
    const InitialGuess affine_initial_guess =
        registration_result ? InitialGuess{registration_result->transformation} : *initial_guess;
    CONSOLE("Running affine registration stage.");
    const GlobalRegistrationConfig affine_config{
        .channels = channels,
        .transformation_type = GlobalRegistrationType::Affine,
        .initial_guess = affine_initial_guess,
        .metric = *global_metric,
        .max_iterations = max_iterations,
    };
    registration_result = GPU::run_registration(affine_config, gpu_compute_context);
  }

  if (registration_result) {
    initial_affine = registration_result->transformation;
  }

  if (has_nonlinear_registration) {
    if (!nonlinear_metric) {
      throw Exception("Non-linear registration could not be initialised.");
    }
    const NonLinearRegistrationConfig nonlinear_config{
        .channels = channels,
        .metric = *nonlinear_metric,
        .max_iterations = max_iterations,
        .fluid_blur_sigma_voxels = nonlinear_fluid_sigma,
        .diffusion_blur_sigma_voxels = nonlinear_diffusion_sigma,
        .ncc_update_target_voxels = nonlinear_ncc_update_target_voxels,
        .svf_bch_terms = nonlinear_svf_bch_terms,
        .initial_affine = initial_affine,
    };
    const NonLinearRegistrationResult nonlinear_result =
        GPU::run_nonlinear_registration(nonlinear_config, gpu_compute_context);

    std::optional<Image<float>> warp1_image;
    if (!warp1_filename.empty() || !transformed_filenames.empty()) {
      if (!nonlinear_result.warp1) {
        throw Exception("Non-linear registration did not return warp1.");
      }
      warp1_image = nonlinear_result.warp1.value();
    }

    if (!warp1_filename.empty()) {
      if (!nonlinear_result.warp2) {
        throw Exception("Non-linear registration did not return warp2.");
      }
      Image<float> warp2_image = nonlinear_result.warp2.value();
      Image<float> output_warp1 = Image<float>::create(warp1_filename, warp1_image.value()).with_direct_io();
      threaded_copy(warp1_image.value(), output_warp1);

      Image<float> output_warp2 = Image<float>::create(warp2_filename, warp2_image).with_direct_io();
      threaded_copy(warp2_image, output_warp2);
    }

    if (!transformed_filenames.empty()) {
      Header transformed_warp_header(warp1_image.value());
      transformed_warp_header.datatype() = DataType::from<default_type>();
      Image<default_type> transformed_warp =
          Image<default_type>::scratch(transformed_warp_header, "nonlinear transformed warp").with_direct_io();
      threaded_copy(warp1_image.value(), transformed_warp);

      const size_t transforms_to_write = std::min(transformed_filenames.size(), header_pairs.size());
      for (size_t idx = 0; idx < transforms_to_write; ++idx) {
        const auto &[header1, header2] = header_pairs[idx];

        Image<float> input_image = Image<float>::open(header1.name());
        Header output_header(header2);
        output_header.datatype() = DataType::from<float>();
        auto output_image = Image<float>::create(transformed_filenames[idx].string(), output_header).with_direct_io();

        Filter::warp<Interp::Cubic>(input_image, output_image, transformed_warp, 0.0F);
      }
    }

    return;
  }

  if (!registration_result) {
    throw Exception("Registration type is not supported.");
  }

  if (!matrix_filename.empty()) {
    const Eigen::Vector3d centre = image_centre_scanner_space(header_pairs.front().header1);
    File::Matrix::save_transform(registration_result->transformation, centre, matrix_filename);
  }

  if (!transformed_filenames.empty()) {
    const size_t transforms_to_write = std::min(transformed_filenames.size(), header_pairs.size());
    for (size_t idx = 0; idx < transforms_to_write; ++idx) {
      const auto &[header1, header2] = header_pairs[idx];

      Image<float> input_image = Image<float>::open(header1.name());
      Header output_header(header2);
      output_header.datatype() = DataType::from<float>();
      auto output_image = Image<float>::create(transformed_filenames[idx].string(), output_header).with_direct_io();

      Filter::reslice<Interp::Cubic>(
          input_image, output_image, registration_result->transformation, Adapter::AutoOverSample, 0.0F);
    }
  }
}
