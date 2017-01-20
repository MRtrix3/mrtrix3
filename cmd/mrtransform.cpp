/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see www.mrtrix.org
 *
 */


#include "command.h"
#include "progressbar.h"
#include "image.h"
#include "math/math.h"
#include "math/sphere.h"
#include "interp/nearest.h"
#include "interp/linear.h"
#include "interp/cubic.h"
#include "interp/sinc.h"
#include "filter/reslice.h"
#include "filter/warp.h"
#include "algo/loop.h"
#include "algo/copy.h"
#include "algo/threaded_copy.h"
#include "dwi/directions/predefined.h"
#include "dwi/gradient.h"
#include "registration/transform/reorient.h"
#include "registration/warp/helpers.h"
#include "registration/warp/compose.h"
#include "math/average_space.h"
#include "math/SH.h"



using namespace MR;
using namespace App;

const char* interp_choices[] = { "nearest", "linear", "cubic", "sinc", NULL };

void usage ()
{

  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com) and David Raffelt (david.raffelt@florey.edu.au) and Max Pietsch (maximilian.pietsch@kcl.ac.uk)";

  DESCRIPTION
  + "apply spatial transformations to an image. "

  + "If a linear transform is applied without a template image the command "
    "will modify the image header transform matrix"

  + "FOD reorientation (with apodised point spread functions) will be performed "
    "by default if the number of volumes in the 4th dimension equals the number "
    "of coefficients in an antipodally symmetric spherical harmonic series (e.g. "
    "6, 15, 28 etc). The -no_reorientation option can be used to force "
    "reorientation off if required."

  + "If a DW scheme is contained in the header (or specified separately), and "
    "the number of directions matches the number of volumes in the images, any "
    "transformation applied using the -linear option will be also be applied to the directions.";

  REFERENCES
    + "* If FOD reorientation is being performed:\n"
    "Raffelt, D.; Tournier, J.-D.; Crozier, S.; Connelly, A. & Salvado, O. " // Internal
    "Reorientation of fiber orientation distributions using apodized point spread functions. "
    "Magnetic Resonance in Medicine, 2012, 67, 844-855"

    + "* If FOD modulation is being performed:\n"
    "Raffelt, D.; Tournier, J.-D.; Rose, S.; Ridgway, G.R.; Henderson, R.; Crozier, S.; Salvado, O.; Connelly, A.; " // Internal
    "Apparent Fibre Density: a novel measure for the analysis of diffusion-weighted magnetic resonance images. "
    "NeuroImage, 2012, 15;59(4), 3976-94.";

  ARGUMENTS
  + Argument ("input", "input image to be transformed.").type_image_in ()
  + Argument ("output", "the output image.").type_image_out ();

  OPTIONS
    + OptionGroup ("Affine transformation options")

    + Option ("linear",
        "specify a linear transform to apply, in the form of a 3x4 "
        "or 4x4 ascii file. Note the standard 'reverse' convention "
        "is used, where the transform maps points in the template image "
        "to the moving image. Note that the reverse convention is still "
        "assumed even if no -template image is supplied")
    +   Argument ("transform").type_file_in ()

    + Option ("flip",
        "flip the specified axes, provided as a comma-separated list of indices (0:x, 1:y, 2:z).")
    +   Argument ("axes").type_sequence_int()

    + Option ("inverse",
        "apply the inverse transformation")

    + Option ("half",
        "apply the matrix square root of the transformation. This can be combined with the inverse option.")

    + Option ("replace",
        "replace the linear transform of the original image by that specified, "
        "rather than applying it to the original image. The specified transform "
        "can be either a template image, or a 3x4 or 4x4 ascii file.")
    +   Argument ("file").type_file_in()

    + Option ("identity",
              "set the header transform of the image to the identity matrix")

    + OptionGroup ("Regridding options")

    + Option ("template",
        "reslice the input image to match the specified template image grid.")
    + Argument ("image").type_image_in ()

    + Option ("midway_space",
        "reslice the input image to the midway space. Requires either the -template or -warp option. If "
        "used with -template and -linear option the input image will be resliced onto the grid halfway between the input and template. "
        "If used with the -warp option the input will be warped to the midway space defined by the grid of the input warp "
        "(i.e. half way between image1 and image2)")

    + Option ("interp",
        "set the interpolation method to use when reslicing (choices: nearest, linear, cubic, sinc. Default: cubic).")
    + Argument ("method").type_choice (interp_choices)

    + OptionGroup ("Non-linear transformation options")

    // TODO point users to a documentation page describing the warp field format
      + Option ("warp",
          "apply a non-linear 4D deformation field to warp the input image. Each voxel in the deformation field must define "
          "the scanner space position that will be used to interpolate the input image during warping (i.e. pull-back/reverse warp convention). "
          "If the -template image is also supplied the deformation field will be resliced first to the template image grid. If no -template "
          "option is supplied then the output image will have the same image grid as the deformation field. This option can be used in "
          "combination with the -affine option, in which case the affine will be applied first)")
      + Argument ("image").type_image_in ()

    + Option ("warp_full",
        "warp the input image using a 5D warp file output from mrregister. Any linear transforms in the warp image header "
        "will also be applied. The -warp_full option must be used in combination with either the -template option or the -midway_space option. "
        "If a -template image is supplied then the full warp will be used. By default the image1->image2 transform will be applied, "
        "however the -from 2 option can be used to apply the image2->image1 transform. Use the -midway_space option to warp the input "
        "image to the midway space. The -from option can also be used to define which warp to use when transforming to midway space")
    + Argument ("image").type_image_in ()

    + Option ("from",
        "used to define which space the input image is when using the -warp_mid option. "
        "Use -from 1 to warp from image1 or -from 2 to warp from image2")
    +   Argument ("image").type_integer (1,2)

    + OptionGroup ("Fibre orientation distribution handling options")

    + Option ("modulate",
        "modulate FODs during reorientation to preserve the apparent fibre density across fibre bundle widths before and after the transformation")

    + Option ("directions",
        "directions defining the number and orientation of the apodised point spread functions used in FOD reorientation "
        "(Default: 300 directions)")
    + Argument ("file", "a list of directions [az el] generated using the dirgen command.").type_file_in()

    + Option ("noreorientation",
        "turn off FOD reorientation. Reorientation is on by default if the number "
        "of volumes in the 4th dimension corresponds to the number of coefficients in an "
        "antipodally symmetric spherical harmonic series (i.e. 6, 15, 28, 45, 66 etc")

    + DWI::GradImportOptions()

    + DataType::options ()

    + Option ("nan",
      "Use NaN as the out of bounds value (Default: 0.0)");
}

void apply_warp (Image<float>& input, Image<float>& output, Image<default_type>& warp, const int interp, const float out_of_bounds_value) {
  switch (interp) {
  case 0:
    Filter::warp<Interp::Nearest> (input, output, warp, out_of_bounds_value);
    break;
  case 1:
    Filter::warp<Interp::Linear> (input, output, warp, out_of_bounds_value);
    break;
  case 2:
    Filter::warp<Interp::Cubic> (input, output, warp, out_of_bounds_value);
    break;
  case 3:
    Filter::warp<Interp::Sinc> (input, output, warp, out_of_bounds_value);
    break;
  default:
    assert (0);
    break;
  }
}



void run ()
{
  auto input_header = Header::open (argument[0]);
  Header output_header (input_header);
  output_header.datatype() = DataType::from_command_line (DataType::from<float> ());

  // Linear
  transform_type linear_transform;
  bool linear = false;
  auto opt = get_options ("linear");
  if (opt.size()) {
    linear = true;
    linear_transform = load_transform (opt[0][0]);
  }

  // Replace
  bool replace = false;
  opt = get_options ("replace");
  if (opt.size()) {
    linear = replace = true;
    try {
      auto template_header = Header::open (opt[0][0]);
      linear_transform = template_header.transform();
    } catch (...) {
      try {
        linear_transform = load_transform (opt[0][0]);
      } catch (...) {
        throw Exception ("Unable to extract transform matrix from -replace file \"" + str(opt[0][0]) + "\"");
      }
    }
  }

  if (get_options ("identity").size()) {
    linear = replace = true;
    linear_transform.setIdentity();
  }

  // Template
  opt = get_options ("template");
  Header template_header;
  if (opt.size()) {
    if (replace)
      throw Exception ("you cannot use the -replace option with the -template option");
    if (!linear)
      linear_transform.setIdentity();
    template_header = Header::open (opt[0][0]);
    for (size_t i = 0; i < 3; ++i) {
      output_header.size(i) = template_header.size(i);
      output_header.spacing(i) = template_header.spacing(i);
    }
    output_header.transform() = template_header.transform();
    add_line (output_header.keyval()["comments"], std::string ("regridded to template image \"" + template_header.name() + "\""));
  }

  // Warp 5D warp
  // TODO add reference to warp format documentation
  opt = get_options ("warp_full");
  Image<default_type> warp;
  if (opt.size()) {
    warp = Image<default_type>::open (opt[0][0]).with_direct_io();
    Registration::Warp::check_warp_full (warp);
    if (linear)
      throw Exception ("the -warp_full option cannot be applied in combination with -linear since the "
                       "linear transform is already included in the warp header");
  }

  // Warp from image1 or image2
  int from = 1;
  opt = get_options ("from");
  if (opt.size()) {
    from = opt[0][0];
    if (!warp.valid())
      WARN ("-from option ignored since no 5D warp was input");
  }

  // Warp deformation field
  opt = get_options ("warp");
  if (opt.size()) {
    if (warp.valid())
      throw Exception ("only one warp field can be input with either -warp or -warp_mid");
    warp = Image<default_type>::open (opt[0][0]).with_direct_io (Stride::contiguous_along_axis(3));
    if (warp.ndim() != 4)
      throw Exception ("the input -warp file must be a 4D deformation field");
    if (warp.size(3) != 3)
      throw Exception ("the input -warp file must have 3 volumes in the 4th dimension (x,y,z positions)");
  }

  // Inverse
  const bool inverse = get_options ("inverse").size();
  if (inverse) {
    if (!(linear || warp.valid()))
      throw Exception ("no linear or warp transformation provided for option '-inverse'");
    if (replace)
      throw Exception ("cannot use -inverse option in conjunction with -replace or -identity options");
    if (warp.valid())
      if (warp.ndim() == 4)
        throw Exception ("cannot apply -inverse with the input -warp_df deformation field.");
    linear_transform = linear_transform.inverse();
  }

  // Half
  const bool half = get_options ("half").size();
  if (half) {
    if (!(linear))
      throw Exception ("no linear transformation provided for option '-half'");
    if (replace)
      throw Exception ("cannot use -half option in conjunction with -replace or -identity options");
    Eigen::Matrix<default_type, 4, 4> temp;
    temp.row(3) << 0, 0, 0, 1.0;
    temp.topLeftCorner(3,4) = linear_transform.matrix().topLeftCorner(3,4);
    linear_transform.matrix() = temp.sqrt().topLeftCorner(3,4);
  }

  // Flip
  opt = get_options ("flip");
  if (opt.size()) {
    vector<int> axes = opt[0][0];
    transform_type flip;
    flip.setIdentity();
    for (size_t i = 0; i < axes.size(); ++i) {
      if (axes[i] < 0 || axes[i] > 2)
        throw Exception ("axes supplied to -flip are out of bounds (" + std::string (opt[0][0]) + ")");
      flip(axes[i],3) += flip(axes[i],axes[i]) * input_header.spacing(axes[i]) * (input_header.size(axes[i])-1);
      flip(axes[i], axes[i]) *= -1.0;
    }
    if (!replace)
      flip = input_header.transform() * flip * input_header.transform().inverse();
    // For flipping an axis in the absence of any other linear transform
    if (!linear) {
      linear_transform.setIdentity();
      linear = true;
    }
    linear_transform = linear_transform * flip;
  }

  Stride::List stride = Stride::get (input_header);

  // Detect FOD image for reorientation
  opt = get_options ("noreorientation");
  bool fod_reorientation = false;
  Eigen::MatrixXd directions_cartesian;
  if (!opt.size() && (linear || warp.valid() || template_header.valid()) && input_header.ndim() == 4 &&
      input_header.size(3) >= 6 &&
      input_header.size(3) == (int) Math::SH::NforL (Math::SH::LforN (input_header.size(3)))) {
    CONSOLE ("SH series detected, performing apodised PSF reorientation");
    fod_reorientation = true;

    Eigen::MatrixXd directions_az_el;
    opt = get_options ("directions");
    if (opt.size())
      directions_az_el = load_matrix (opt[0][0]);
    else
      directions_az_el = DWI::Directions::electrostatic_repulsion_300();
    Math::Sphere::spherical2cartesian (directions_az_el, directions_cartesian);

    // load with SH coeffients contiguous in RAM
    stride = Stride::contiguous_along_axis (3, input_header);
  }

  // Modulate FODs
  bool modulate = false;
  if (get_options ("modulate").size()) {
    modulate = true;
    if (!fod_reorientation)
      throw Exception ("modulation can only be performed with FOD reorientation");
  }

  // Rotate/Flip gradient directions if present
  if (linear && input_header.ndim() == 4 && !warp && !fod_reorientation) {
    Eigen::MatrixXd rotation = linear_transform.linear().inverse();
    Eigen::MatrixXd test = rotation.transpose() * rotation;
    test = test.array() / test.diagonal().mean();
    if (!test.isIdentity (0.001)) {
      WARN ("the input linear transform contains shear or anisotropic scaling and "
            "therefore should not be used to reorient directions / diffusion gradients");
    }
    if (replace)
      rotation = linear_transform.linear() * input_header.transform().linear().inverse();
    try {
      auto grad = DWI::get_DW_scheme (input_header);
      if (input_header.size(3) == (ssize_t) grad.rows()) {
        INFO ("DW gradients detected and will be reoriented");
        for (ssize_t n = 0; n < grad.rows(); ++n) {
          Eigen::Vector3 grad_vector = grad.block<1,3>(n,0);
          grad.block<1,3>(n,0) = rotation * grad_vector;
        }
        DWI::set_DW_scheme (output_header, grad);
      }
    }
    catch (Exception& e) {
      e.display (2);
      WARN ("DW gradients not correctly reoriented");
    }
    // Also look for key 'directions', and rotate those if present
    auto hit = input_header.keyval().find ("directions");
    if (hit != input_header.keyval().end()) {
      INFO ("Header entry \"directions\" detected and will be reoriented");
      try {
        const auto lines = split_lines (hit->second);
        if (lines.size() != size_t(input_header.size(3)))
          throw Exception ("Number of lines in header entry \"directions\" (" + str(lines.size()) + ") does not match number of volumes in image (" + str(input_header.size(3)) + ")");
        Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic> result;
        for (size_t l = 0; l != lines.size(); ++l) {
          const auto v = parse_floats (lines[l]);
          if (!result.cols()) {
            if (!(v.size() == 2 || v.size() == 3))
              throw Exception ("Malformed \"directions\" field (expected matrix with 2 or 3 columns; data has " + str(v.size()) + " columns)");
            result.resize (lines.size(), v.size());
          } else {
            if (v.size() != size_t(result.cols()))
              throw Exception ("Inconsistent number of columns in \"directions\" field");
          }
          if (result.cols() == 2) {
            Eigen::Matrix<default_type, 2, 1> azel (v.data());
            Eigen::Vector3 dir;
            Math::Sphere::spherical2cartesian (azel, dir);
            dir = rotation * dir;
            Math::Sphere::cartesian2spherical (dir, azel);
            result.row (l) = azel;
          } else {
            const Eigen::Vector3 dir = rotation * Eigen::Vector3 (v.data());
            result.row (l) = dir;
          }
          std::stringstream s;
          Eigen::IOFormat format (6, Eigen::DontAlignCols, ",", "\n", "", "", "", "");
          s << result.format (format);
          output_header.keyval()["directions"] = s.str();
        }
      } catch (Exception& e) {
        e.display (2);
        WARN ("Header entry \"directions\" not correctly reoriented");
      }
    }
  }

  // Interpolator
  int interp = 2;  // cubic
  opt = get_options ("interp");
  if (opt.size()) {
    interp = opt[0][0];
    if (!warp && !template_header)
      WARN ("interpolator choice ignored since the input image will not be regridded");
  }

  // Out of bounds value
  float out_of_bounds_value = 0.0;
  opt = get_options ("nan");
  if (opt.size()) {
    out_of_bounds_value = NAN;
    if (!warp && !template_header)
      WARN ("Out of bounds value ignored since the input image will not be regridded");
  }

  auto input = input_header.get_image<float>().with_direct_io (stride);

  // Reslice the image onto template
  if (template_header.valid() && !warp) {
    INFO ("image will be regridded");

    if (get_options ("midway_space").size()) {
      INFO("regridding to midway space");
      vector<Header> headers;
      headers.push_back(input_header);
      headers.push_back(template_header);
      vector<Eigen::Transform<default_type, 3, Eigen::Projective>> void_trafo;
      auto padding = Eigen::Matrix<double, 4, 1>(1.0, 1.0, 1.0, 1.0);
      int subsampling = 1;
      auto midway_header = compute_minimum_average_header (headers, subsampling, padding, void_trafo);
      for (size_t i = 0; i < 3; ++i) {
        output_header.size(i) = midway_header.size(i);
        output_header.spacing(i) = midway_header.spacing(i);
      }
      output_header.transform() = midway_header.transform();
    }

    if (interp == 0)
      output_header.datatype() = DataType::from_command_line (input_header.datatype());
    auto output = Image<float>::create (argument[1], output_header).with_direct_io();

    switch (interp) {
      case 0:
        Filter::reslice<Interp::Nearest> (input, output, linear_transform, Adapter::AutoOverSample, out_of_bounds_value);
        break;
      case 1:
        Filter::reslice<Interp::Linear> (input, output, linear_transform, Adapter::AutoOverSample, out_of_bounds_value);
        break;
      case 2:
        Filter::reslice<Interp::Cubic> (input, output, linear_transform, Adapter::AutoOverSample, out_of_bounds_value);
        break;
      case 3:
        Filter::reslice<Interp::Sinc> (input, output, linear_transform, Adapter::AutoOverSample, out_of_bounds_value);
        break;
      default:
        assert (0);
        break;
    }

    if (fod_reorientation)
      Registration::Transform::reorient ("reorienting", output, output, linear_transform, directions_cartesian.transpose(), modulate);

  } else if (warp.valid()) {

    if (replace)
      throw Exception ("you cannot use the -replace option with the -warp or -warp_df option");

    if (!template_header) {
      for (size_t i = 0; i < 3; ++i) {
        output_header.size(i) = warp.size(i);
        output_header.spacing(i) = warp.spacing(i);
      }
      output_header.transform() = warp.transform();
      add_line (output_header.keyval()["comments"], std::string ("resliced using warp image \"" + warp.name() + "\""));
    }

    auto output = Image<float>::create(argument[1], output_header).with_direct_io();

    if (warp.ndim() == 5) {
      Image<default_type> warp_deform;

      // Warp to the midway space defined by the warp grid
      if (get_options ("midway_space").size()) {
        warp_deform = Registration::Warp::compute_midway_deformation (warp, from);
      // Use the full transform to warp from the image image to the template
      } else {
        warp_deform = Registration::Warp::compute_full_deformation (warp, template_header, from);
      }
      apply_warp (input, output, warp_deform, interp, out_of_bounds_value);
      if (fod_reorientation)
        Registration::Transform::reorient_warp ("reorienting", output, warp_deform, directions_cartesian.transpose(), modulate);

    // Compose and apply input linear and 4D deformation field
    } else if (warp.ndim() == 4 && linear) {
      auto warp_composed = Image<default_type>::scratch (warp);
      Registration::Warp::compose_linear_deformation (linear_transform, warp, warp_composed);
      apply_warp (input, output, warp_composed, interp, out_of_bounds_value);
      if (fod_reorientation)
        Registration::Transform::reorient_warp ("reorienting", output, warp_composed, directions_cartesian.transpose(), modulate);

    // Apply 4D deformation field only
    } else {
      apply_warp (input, output, warp, interp, out_of_bounds_value);
      if (fod_reorientation)
        Registration::Transform::reorient_warp ("reorienting", output, warp, directions_cartesian.transpose(), modulate);
    }

  // No reslicing required, so just modify the header and do a straight copy of the data
  } else {

    if (get_options ("midway").size())
      throw Exception ("midway option given but no template image defined");

    INFO ("image will not be regridded");
    Eigen::MatrixXd rotation = linear_transform.linear();
    Eigen::MatrixXd temp = rotation.transpose() * rotation;
    if (!temp.isIdentity (0.001))
      WARN("the input linear transform is not orthonormal and therefore applying this without the -template "
           "option will mean the output header transform will also be not orthonormal");

    add_line (output_header.keyval()["comments"], std::string ("transform modified"));
    if (replace)
      output_header.transform() = linear_transform;
    else
      output_header.transform() = linear_transform.inverse() * output_header.transform();
    auto output = Image<float>::create (argument[1], output_header).with_direct_io();
    copy_with_progress (input, output);

    if (fod_reorientation) {
      transform_type transform = linear_transform;
      if (replace)
        transform = linear_transform * output_header.transform().inverse();
      Registration::Transform::reorient ("reorienting", output, output, transform, directions_cartesian.transpose());
    }
  }
}

