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
 #include "app.h"
 #include "cmdline_option.h"
 #include "command.h" // IWYU pragma: keep
 #include "datatype.h"
 #include "exception.h"
 #include "file/matrix.h"
 #include "filter/reslice.h"
 #include "gpu/gpu.h"
 #include "gpu/registration/eigenhelpers.h"
 #include "gpu/registration/globalregistration.h"
 #include "gpu/registration/registrationtypes.h"
 #include "gpu/registration/imageoperations.h"
 #include "gpu/registration/imageoperations.h"
 #include "header.h"
 #include "image.h"
 #include "image_helpers.h"
 #include "interp/cubic.h"
 #include "magic_enum/magic_enum.hpp"
 #include "math/average_space.h"
 #include "mrtrix.h"
 #include "types.h"

 #include <Eigen/Core>
 #include <Eigen/Geometry>

 #include <algorithm>
 #include <cmath>
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
 constexpr TransformationType default_transformation_type = TransformationType::Affine;
 constexpr MetricType default_metric_type = MetricType::NMI;
 constexpr uint32_t default_ncc_window_radius = 0U;
 constexpr uint32_t default_max_iterations = 500;
 const std::vector<std::string> supported_metric_types = lowercase_enum_names<MetricType>();
 const std::vector<std::string> supported_transform_types = lowercase_enum_names<TransformationType>();
 const std::vector<std::string> supported_init_translations = lowercase_enum_names<InitTranslationChoice>();
 const std::vector<std::string> supported_init_rotations = lowercase_enum_names<InitRotationChoice>();

 struct HalfwayTransforms {
   transform_type half;
   transform_type half_inverse;
   Eigen::Matrix4d half_matrix;
   Eigen::Matrix4d half_inverse_matrix;
 };

 HalfwayTransforms compute_halfway_transforms(const transform_type &scanner_transform) {
   const Eigen::Matrix4d matrix = EigenHelpers::to_homogeneous_mat4d(scanner_transform);
   const double det = matrix.block<3, 3>(0, 0).determinant();
   if (!std::isfinite(det) || det <= 0.0) {
     throw Exception("Cannot compute halfway transform: non-invertible or reflected transform.");
   }
   const Eigen::Matrix4d half_matrix = matrix.sqrt();
   const Eigen::Matrix4d half_inverse_matrix = half_matrix.inverse();
   return HalfwayTransforms{
       .half = EigenHelpers::from_homogeneous_mat4d(half_matrix),
       .half_inverse = EigenHelpers::from_homogeneous_mat4d(half_inverse_matrix),
       .half_matrix = half_matrix,
       .half_inverse_matrix = half_inverse_matrix,
   };
 }

 } // namespace

 // clang-format off
 // NOLINTBEGIN(readability-implicit-bool-conversion)
 void usage() {
     AUTHOR = "Daljit Singh",
         SYNOPSIS = "Affine image registration on the GPU.";

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

         + Option ("transformed_midway", "image1 and image2 after registration transformed and regridded to the midway space."
                                        " Note that -transformed_midway needs to be repeated for each contrast.")
             .allow_multiple()
             + Argument("image1_transformed").type_image_out()
             + Argument("image2_transformed").type_image_out()

         + Option ("matrix", "write the transformation matrix used for reslicing image1 into image2 space.")
             + Argument("filename").type_file_out()

         + Option ("matrix_1tomidway", "write the transformation matrix used for reslicing image1 into midway space.")
             + Argument("filename").type_file_out()

         + Option ("matrix_2tomidway", "write the transformation matrix used for reslicing image2 into midway space.")
             + Argument("filename").type_file_out()

         + Option ("type", "type of transform (rigid, affine)")
             + Argument("name").type_choice(supported_transform_types)

         + Option ("metric", "similarity metric to use (nmi, ssd, ncc)")
             + Argument("name").type_choice(supported_metric_types)

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

   const TransformationType transform_type = from_name<TransformationType>(
       get_option_value<std::string>("type", enum_name_lowercase(default_transformation_type)));

   const MetricType metric_type =
       from_name<MetricType>(get_option_value<std::string>("metric", enum_name_lowercase(default_metric_type)));

   const uint32_t ncc_window_radius = get_option_value<uint32_t>("ncc_radius", default_ncc_window_radius);

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

   Eigen::Vector3d centre;
   InitialGuess initial_guess;
   if (!init_matrix_option.empty()) {
     // TODO: compute centre from images. Also check what's the correct thing to do in this case.
     initial_guess = File::Matrix::load_transform(init_matrix_option[0][0], centre);
   } else {
     const InitTranslationChoice init_translation =
         from_name<InitTranslationChoice>(get_option_value<std::string>("init_translation", "mass"));
     const InitRotationChoice init_rotation =
         from_name<InitRotationChoice>(get_option_value<std::string>("init_rotation", "none"));
     const float init_rotation_max_angle = get_option_value<float>("init_rotation_max_angle", default_max_search_angle);
     Metric init_metric;
     switch (metric_type) {
     case MetricType::NMI:
       init_metric = NMIMetric{};
       break;
     case MetricType::SSD:
       init_metric = SSDMetric{};
       break;
     case MetricType::NCC:
       init_metric = NCCMetric{.window_radius = ncc_window_radius};
       break;
     default:
       throw Exception("Unsupported metric type");
     }

     initial_guess = InitialisationOptions{
         .translation_choice = init_translation,
         .rotation_choice = init_rotation,
         .cost_metric = init_metric,
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

   Metric metric;
   switch (metric_type) {
   case MetricType::NMI:
     metric = NMIMetric{};
     break;
   case MetricType::SSD:
     metric = SSDMetric{};
     break;
   case MetricType::NCC:
     metric = NCCMetric{.window_radius = ncc_window_radius};
     break;
   default:
     throw Exception("Unsupported metric type");
   }

   const RegistrationConfig registration_config{
       .channels = channels,
       .transformation_type = transform_type,
       .initial_guess = initial_guess,
       .metric = metric,
       .max_iterations = max_iterations,
   };

   auto gpu_compute_context = gpu_context_request.get();
   const RegistrationResult registration_result = GPU::run_registration(registration_config, gpu_compute_context);

   const std::string matrix_filename = get_option_value<std::string>("matrix", "");
   const std::string matrix_1tomid_filename = get_option_value<std::string>("matrix_1tomidway", "");
   const std::string matrix_2tomid_filename = get_option_value<std::string>("matrix_2tomidway", "");

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

   const auto transformed_midway_option = get_options("transformed_midway");
   std::vector<std::filesystem::path> transformed_midway1_filenames;
   std::vector<std::filesystem::path> transformed_midway2_filenames;
   if (!transformed_midway_option.empty()) {
     if (transformed_midway_option.size() > header_pairs.size()) {
       throw Exception("Number of -transformed_midway images exceeds number of contrasts");
     }
     if (transformed_midway_option.size() < header_pairs.size()) {
       WARN("Number of -transformed_midway images is less than number of contrasts.");
     }
     for (size_t i = 0; i < transformed_midway_option.size(); ++i) {
       if (transformed_midway_option[i].args.size() != 2U) {
         throw Exception("Each -transformed_midway option requires two output images.");
       }
       const std::filesystem::path output1_path(transformed_midway_option[i][0]);
       const std::filesystem::path output2_path(transformed_midway_option[i][1]);
       transformed_midway1_filenames.push_back(output1_path);
       transformed_midway2_filenames.push_back(output2_path);
       const auto input1_path = std::filesystem::path(header_pairs[i].header1.name());
       const auto input2_path = std::filesystem::path(header_pairs[i].header2.name());
       INFO(input1_path.filename().string() + ", transformed to midway space, will be saved to " +
            output1_path.string());
       INFO(input2_path.filename().string() + ", transformed to midway space, will be saved to " +
            output2_path.string());
     }
   }

   const bool needs_halfway_transforms =
       !transformed_midway1_filenames.empty() || !matrix_1tomid_filename.empty() || !matrix_2tomid_filename.empty();
   std::optional<HalfwayTransforms> halfway_transforms;
   if (needs_halfway_transforms) {
     halfway_transforms = compute_halfway_transforms(registration_result.transformation);
   }

   if (!matrix_filename.empty() || !matrix_1tomid_filename.empty() || !matrix_2tomid_filename.empty()) {
     const Eigen::Vector3d centre = image_centre_scanner_space(header_pairs.front().header1);
     if (!matrix_filename.empty()) {
       File::Matrix::save_transform(registration_result.transformation, centre, matrix_filename);
     }
     if (!matrix_1tomid_filename.empty()) {
       File::Matrix::save_transform(halfway_transforms->half, centre, matrix_1tomid_filename);
     }
     if (!matrix_2tomid_filename.empty()) {
       File::Matrix::save_transform(halfway_transforms->half_inverse, centre, matrix_2tomid_filename);
     }
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
           input_image, output_image, registration_result.transformation, Adapter::AutoOverSample, 0.0F);
     }
   }

   if (!transformed_midway1_filenames.empty()) {
     // Compute midpioint transforms in scanner space and then build a midway output header that can hold both images
     using ProjectiveTransform = Eigen::Transform<default_type, 3, Eigen::Projective>;
     const ProjectiveTransform half_projective(halfway_transforms->half_matrix);
     const ProjectiveTransform half_inverse_projective(halfway_transforms->half_inverse_matrix);

     const size_t transforms_to_write = std::min(transformed_midway1_filenames.size(), header_pairs.size());
     for (size_t idx = 0; idx < transforms_to_write; ++idx) {
       const auto &[header1, header2] = header_pairs[idx];
       Header output_header = compute_minimum_average_header(header1, header2, half_inverse_projective, half_projective);
       output_header.datatype() = DataType::from<float>();

       Image<float> input_image1 = Image<float>::open(header1.name());
       auto output_image1 = Image<float>::create(transformed_midway1_filenames[idx].string(), output_header).with_direct_io();
       Filter::reslice<Interp::Cubic>(
           input_image1, output_image1, halfway_transforms->half, Adapter::AutoOverSample, 0.0F);

       Image<float> input_image2 = Image<float>::open(header2.name());
       auto output_image2 = Image<float>::create(transformed_midway2_filenames[idx].string(), output_header).with_direct_io();
       Filter::reslice<Interp::Cubic>(
           input_image2, output_image2, halfway_transforms->half_inverse, Adapter::AutoOverSample, 0.0F);
     }
   }
 }
