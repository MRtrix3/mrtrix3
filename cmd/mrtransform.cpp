/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "command.h"
#include "progressbar.h"
#include "image.h"
#include "math/math.h"
#include "interp/nearest.h"
#include "interp/linear.h"
#include "interp/cubic.h"
#include "interp/sinc.h"
#include "filter/reslice.h"
#include "algo/loop.h"
#include "algo/copy.h"
#include "dwi/directions/predefined.h"
#include "dwi/gradient.h"
//#include "registration/transform/reorient.h"



using namespace MR;
using namespace App;

const char* interp_choices[] = { "nearest", "linear", "cubic", "sinc", NULL };

void usage ()
{
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
    + "If FOD reorientation is being performed:\n"
    "Raffelt, D.; Tournier, J.-D.; Crozier, S.; Connelly, A. & Salvado, O. "
    "Reorientation of fiber orientation distributions using apodized point spread functions. "
    "Magnetic Resonance in Medicine, 2012, 67, 844-855";

  ARGUMENTS
  + Argument ("input", "input image to be transformed.").type_image_in ()
  + Argument ("output", "the output image.").type_image_out ();

  OPTIONS
    + OptionGroup ("Affine transformation options")

    + Option ("linear", 
        "specify a 4x4 linear transform to apply, in the form "
        "of a 4x4 ascii file. Note the standard 'reverse' convention "
        "is used, where the transform maps points in the template image "
        "to the moving image. Note that the reverse convention is still "
        "assumed even if no -template image is supplied")
    +   Argument ("transform").type_file_in ()

    + Option ("flip",
        "flip the specified axes, provided as a comma-separated list of indices (0:x, 1:y, 2:z).")
    +   Argument ("axes").type_sequence_int()

    + Option ("inverse", 
        "apply the inverse transformation")

    + Option ("replace", 
        "replace the linear transform of the original image by that specified, "
        "rather than applying it to the original image.")

    + OptionGroup ("Regridding options")

    + Option ("template", 
        "reslice the input image to match the specified template image.")
    + Argument ("image").type_image_in ()

    + Option ("interp", 
        "set the interpolation method to use when reslicing (default: cubic).")
    + Argument ("method").type_choice (interp_choices)

    + Option ("oversample", 
        "set the oversampling factor to use when down-sampling (i.e. the "
        "number of samples to take and average per voxel along each spatial dimension). "
        "This should be supplied as a vector of 3 integers. By default, the "
        "oversampling factor is determined based on the differences between "
        "input and output voxel sizes.")
    + Argument ("factors").type_sequence_int()

    + OptionGroup ("Non-linear transformation options")

    + Option ("warp_df",
        "apply a non-linear deformation field to the input image. If no template image is supplied, "
        "then the input warp will define the output image dimensions.")
    + Argument ("image").type_image_in ()

    + OptionGroup ("Fibre orientation distribution handling options")

    + Option ("directions", 
        "the directions used for FOD reorientation using apodised point spread functions "
        "(Default: 60 directions)")
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



typedef float value_type;


void run ()
{
  auto input_header = Header::open(argument[0]);
  auto input = input_header.get_image<float>();
  Header output_header (input_header);
  output_header.datatype() = DataType::from_command_line (output_header.datatype());

  transform_type linear_transform;

  bool linear = false;
  auto opt = get_options ("linear");
  if (opt.size()) {
    linear = true;
    linear_transform = load_transform (opt[0][0]);
  }

  const bool inverse = get_options ("inverse").size();
  const bool replace = get_options ("replace").size();

  if (inverse) {
    if (!linear)
      throw Exception ("no transform provided for option '-inverse'");
    linear_transform = linear_transform.inverse();
  }

  opt = get_options ("flip");
  if (opt.size()) {
    std::vector<int> axes = opt[0][0];
    transform_type flip;
    flip.setIdentity();
    for (size_t i = 0; i < axes.size(); ++i) {
      if (axes[i] < 0 || axes[i] > 2)
        throw Exception ("axes supplied to -flip are out of bounds (" + std::string (opt[0][0]) + ")");
      flip(axes[i],3) += flip(axes[i],axes[i]) * input.voxsize(axes[i]) * (input.size(axes[i])-1);
      flip(axes[i], axes[i]) *= -1.0;
    }

    if (!linear)
      linear_transform.setIdentity();
    
    transform_type tmp;
    if (!replace) {
      transform_type irot = input.transform().inverse();
      tmp = flip * irot;
      flip = input.transform() * tmp;
    }
    tmp = linear_transform * flip;
    linear_transform = tmp;
  }

  if (replace)
    if (!linear)
      throw Exception ("no transform provided for option '-replace'");




  if (linear && input.ndim() == 4) {
    try {
      auto grad = DWI::get_DW_scheme (input_header);
      if (input_header.size(3) == (ssize_t) grad.rows()) {
        INFO ("DW gradients will be reoriented");

        auto rotation = linear_transform.linear();
        if (replace)
          rotation = linear_transform.linear() * input_header.transform().linear().inverse();

        for (ssize_t n = 0; n < grad.rows(); ++n) {
          Eigen::Vector3 grad_vector = grad.block<1,3>(n,0);
          grad.block<1,3>(n,0) = rotation * grad_vector;
        }

        output_header.set_DW_scheme(grad);
      }
    }
    catch (Exception& e) {
      e.display (2);
    }
  }


//  opt = get_options ("noreorientation");
//  bool do_reorientation = false;
//  Math::Matrix<value_type> directions_cartesian;
//  if (!opt.size() && linear_transform.is_set() && input_header.ndim() == 4 &&
//      input_header.dim(3) >= 6 &&
//      input_header.dim(3) == (int) Math::SH::NforL (Math::SH::LforN (input_header.dim(3)))) {
//    do_reorientation = true;
//    CONSOLE ("SH series detected, performing apodised PSF reorientation");

//    Math::Matrix<value_type> directions_el_az;
//    opt = get_options ("directions");
//    if (opt.size())
//      directions_el_az.load(opt[0][0]);
//    else
//      DWI::Directions::electrostatic_repulsion_60 (directions_el_az);
//    Math::SH::S2C (directions_el_az, directions_cartesian);

//  }


//  opt = get_options ("template"); // need to reslice
//  if (opt.size()) {
//    INFO ("image will be regridded");

//    auto template_header = Header::open (opt[0][0]);
//    for (size_t i = 0; i < 3; ++i) {
//       output_header.size(i) = template_header.size(i);
//       output_header.voxsize(i) = template_header.voxsize(i);
//    }
//    output_header.transform() = template_header.transform();
//    output_header.comments().push_back ("resliced to template image \"" + template_header.name() + "\"");

//    int interp = 2;  // cubic
//    opt = get_options ("interp");
//    if (opt.size())
//      interp = opt[0][0];

//    float out_of_bounds_value = 0.0;
//    opt = get_options ("nan");
//    if (opt.size())
//      out_of_bounds_value = NAN;

//    Image::Stride::List stride = Image::Stride::get (input_header);

//    InputBufferType input_buffer (input_header, stride);
//    if (replace) {
//      Image::Info& info_in (input_buffer);
//      info_in.transform().swap (linear_transform);
//      linear_transform.clear();
//    }
//    InputBufferType::voxel_type in (input_buffer);

//    if (do_reorientation) {
//      stride = Image::Stride::contiguous_along_axis (3, input_header);
//      Image::Stride::set (output_header, stride);
//    }

//    OutputBufferType output_buffer (argument[1], output_header);
//    OutputBufferType::voxel_type output_vox (output_buffer);

//    switch (interp) {
//      case 0:
//        Filter::reslice<Image::Interp::Nearest> (in, output_vox, linear_transform, oversample, out_of_bounds_value);
//        break;
//      case 1:
//        Filter::reslice<Image::Interp::Linear> (in, output_vox, linear_transform, oversample, out_of_bounds_value);
//        break;
//      case 2:
//        Filter::reslice<Image::Interp::Cubic> (in, output_vox, linear_transform, oversample, out_of_bounds_value);
//        break;
//      case 3:
//        FAIL ("FIXME: sinc interpolation needs a lot of work!");
//        Image::Filter::reslice<Image::Interp::Sinc> (in, output_vox, linear_transform, oversample, out_of_bounds_value);
//        break;
//      default:
//        assert (0);
//        break;
//    }

//    if (do_reorientation)
//      Image::Registration::Transform::reorient ("reorienting...", output_vox, output_vox, linear_transform, directions_cartesian);

//  } else {
    // straight copy:
    INFO ("image will not be regridded");
    if (linear) {
      add_line (output_header.keyval()["comments"], std::string ("transform modified"));
      if (replace)
        output_header.transform() = linear_transform;
      else
        output_header.transform() = linear_transform.inverse() * output_header.transform();
    }
    auto output = Image<float>::create (argument[1], output_header);
    copy_with_progress (input, output);

//    if (do_reorientation) {
//      Math::Matrix<float> transform (linear_transform);
//      if (replace)
//        Math::mult (transform, linear_transform, Math::LU::inv (output_header.transform()));
//      Image::Registration::Transform::reorient ("reorienting...", out, out, transform, directions_cartesian);
//    }
//  }
}

