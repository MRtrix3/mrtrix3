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

#include <filesystem>
#include <optional>

#include "adapter/extract.h"
#include "command.h"
#include "enum.h"
#include "file/config.h"
#include "file/nifti_utils.h"
#include "image.h"
#include "registration/warp/compose.h"
#include "registration/warp/convert.h"
#include "registration/warp/helpers.h"
#include "registration/warp/validate.h"

using namespace MR;
using namespace App;

enum class ConversionType {
  Deformation2Displacement,
  Displacement2Deformation,
  WarpFull2Deformation,
  WarpFull2Displacement
};

// clang-format off
// The -template / -midway_space / -from options are meaningful only for the two
//   warpfull-input operations, so they are declared solely on those sub-interfaces.
OptionList warpfull_options() {
  return OptionList()
  + Option ("template",
            "define a template image"
            " (the warpfull grid lies in the midway space between image 1 & 2)."
            " For example, to generate the deformation field that maps image1 to image2,"
            " supply image2 as the template image")
    + Argument ("image").type_image_in()

  + Option ("midway_space",
            "output only the non-linear warp mapping an input image to the midway space"
            " defined by the warpfull grid."
            " If a linear transform exists in the warpfull file header"
            " then it will be composed and included in the output.")

  + Option ("from",
            "define the direction of the desired output field."
            " Use -from 1 to obtain the image1->image2 field and -from 2 for image2->image1."
            " Can be combined with -midway_space to produce a field that only maps to midway space.")
    + Argument ("image").type_integer(1, 2);
}

void usage() {

  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Convert between different representations of a non-linear warp";

  DESCRIPTION
  + "A deformation field is defined as an image"
    " where each voxel defines the corresponding position in the other image"
    " (in scanner space coordinates)."
    " A displacement field stores the displacements (in mm) to the other image"
    " from each voxel's position (in scanner space)."
    " The warpfull file is the 5D format output from mrregister -nl_warp_full,"
    " which contains linear transforms, warps and their inverses"
    " that map each image to a midway space."
    " The operation to be performed is nominated as the first argument;"
    " the subsequent arguments and options available depend on the nominated operation.";
    //TODO add link to warp format documentation

  SUBCOMMANDS_SELECTOR = "operation";

  SUBCOMMANDS
  + Subcommand ("deformation2displacement")
      .set_synopsis ("Convert a deformation field to a displacement field")
      .set_arguments (ArgumentList()
        + Argument ("in", "the input deformation field image.").type_image_in()
        + Argument ("out", "the output displacement field image.").type_image_out())

  + Subcommand ("displacement2deformation")
      .set_synopsis ("Convert a displacement field to a deformation field")
      .set_arguments (ArgumentList()
        + Argument ("in", "the input displacement field image.").type_image_in()
        + Argument ("out", "the output deformation field image.").type_image_out())

  + Subcommand ("warpfull2deformation")
      .set_synopsis ("Convert a 5D warpfull series to a deformation field")
      .set_arguments (ArgumentList()
        + Argument ("in", "the input warpfull image.").type_image_in()
        + Argument ("out", "the output deformation field image.").type_image_out())
      .set_options (warpfull_options())

  + Subcommand ("warpfull2displacement")
      .set_synopsis ("Convert a 5D warpfull series to a displacement field")
      .set_arguments (ArgumentList()
        + Argument ("in", "the input warpfull image.").type_image_in()
        + Argument ("out", "the output displacement field image.").type_image_out())
      .set_options (warpfull_options());

}
// clang-format on

void run() {
  const ConversionType type = MR::Enum::from_name<ConversionType>(App::get_subcommand());

  Header H_in = Header::open(argument[0]);
  auto format = Registration::Warp::validate_header(H_in);

  switch (type) {
  case ConversionType::Deformation2Displacement: {
    if (format != Registration::Warp::WarpFormat::Simple)
      throw Exception("Input to deformation2displacement operation"
                      " must be a 4D deformation field image,"
                      " not a 5D \"full\" warp format series");

    auto deformation = H_in.get_image<default_type>(DirectIO{3});
    Registration::Warp::debug_validate_image(deformation);

    Header H_out(H_in);
    H_out.datatype() = DataType::from_command_line(DataType::Float32);
    Image<default_type> displacement = Image<default_type>::create(argument[1], H_out, DirectIO{3});

    Registration::Warp::deformation2displacement(deformation, displacement);
    break;
  }
  case ConversionType::Displacement2Deformation: {
    if (format != Registration::Warp::WarpFormat::Simple)
      throw Exception("Input to displacement2deformation operation"
                      " must be a 4D displacement field image,"
                      " not a 5D \"full\" warp format series");
    auto displacement = H_in.get_image<default_type>(DirectIO{3});
    Registration::Warp::debug_validate_image(displacement);

    Header H_out(displacement);
    H_out.datatype() = DataType::from_command_line(DataType::Float32);
    Image<default_type> deformation = Image<default_type>::create(argument[1], H_out, DirectIO{3});
    Registration::Warp::displacement2deformation(displacement, deformation);
    break;
  }
  case ConversionType::WarpFull2Deformation:
  case ConversionType::WarpFull2Displacement: {
    const bool midway_space = !get_options("midway_space").empty();
    auto template_filepath = get_optional<std::filesystem::path>("template");
    const int from = get_option_value("from", 1);

    if (!Path::is_mrtrix_image(argument[0]) &&                  //
        !(Path::has_suffix(argument[0], {".nii", ".nii.gz"}) && //
          File::Config::get_bool("NIfTIAutoLoadJSON", false) && //
          std::filesystem::exists(File::NIfTI::get_json_path(argument[0])))) {
      WARN("warp_full image is not in original .mif/.mih file format or in NIfTI file format with associated JSON.  "
           "Converting to other file formats may remove linear transformations stored in the image header.");
    }
    if (format != Registration::Warp::WarpFormat::Full)
      throw Exception("Input to operation converting from a \"full\" warp format series"
                      " must be a 5D image that conforms to that format"
                      " rather than a simple 4D displacement / deformation field image");
    auto warp = H_in.get_image<default_type>(DirectIO{3});
    Registration::Warp::debug_validate_image(warp);

    Image<default_type> warp_output;
    if (midway_space) {
      warp_output = Registration::Warp::compute_midway_deformation(warp, from);
    } else {
      if (!template_filepath.has_value())
        throw Exception("-template option required with warpfull2deformation or warpfull2displacement conversion type");
      auto template_header = Header::open(template_filepath.value());
      warp_output = Registration::Warp::compute_full_deformation(warp, template_header, from);
    }

    if (type == ConversionType::WarpFull2Displacement)
      Registration::Warp::deformation2displacement(warp_output, warp_output);

    Header H_out(warp_output);
    H_out.datatype() = DataType::from_command_line(DataType::Float32);
    Image<default_type> output = Image<default_type>::create(argument[1], H_out);
    threaded_copy_with_progress_message("converting warp", warp_output, output);
    break;
  }
  default:
    throw Exception("Unsupported warp conversion type");
  }
}
