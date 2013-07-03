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

#include "app.h"
#include "progressbar.h"
#include "image/buffer.h"
#include "image/buffer_preload.h"
#include "image/buffer_scratch.h"
#include "image/voxel.h"
#include "image/interp/nearest.h"
#include "image/interp/linear.h"
#include "image/interp/cubic.h"
#include "image/interp/sinc.h"
#include "image/filter/reslice.h"
#include "image/utils.h"
#include "image/loop.h"
#include "image/copy.h"
#include "dwi/directions/predefined.h"
#include "image/registration/transform/reorient.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

const char* interp_choices[] = { "nearest", "linear", "cubic", "sinc", NULL };

void usage ()
{
  DESCRIPTION
  + "apply spatial transformations to an image. "

  + "If a linear transform is applied without a template image the command "
    "will modify the image header transform matrix"

  + "FOD reorientation (with apodised point spread functions) will be performed by default if the number of volumes "
    "in the 4th dimension equals the number of coefficients in an antipodally symmetric spherical harmonic series (e.g. 6, 15, 28 etc). "
    "The -no_reorientation option can be used to force reorientation off if required.";



  ARGUMENTS
  + Argument ("input", "input image to be transformed.").type_image_in ()
  + Argument ("output", "the output image.").type_image_out ();

  OPTIONS
  + Option ("linear", "specify a 4x4 linear transform to apply, in the form "
                      "of a 4x4 ascii file. Note the standard 'reverse' convention "
                      "is used, where the transform maps points in the template image "
                      "to the moving image.")
    + Argument ("transform").type_file ()

  + Option ("warp", "apply a non-linear transform to the input image. If no template image is supplied, "
                    "then the input warp will define the output image dimensions.")
    + Argument ("image").type_image_in ()

  + Option ("template", "reslice the input image to match the specified template image.")
    + Argument ("image").type_image_in ()

  + Option ("inverse", "apply the inverse transformation")

  + Option ("replace", "replace the linear transform of the original image by that specified, "
                       "rather than applying it to the original image.")

  + Option ("interp", "set the interpolation method to use when reslicing (default: cubic).")
    + Argument ("method").type_choice (interp_choices)

  + Option ("oversample", "set the oversampling factor to use when down sampling (i.e. the "
                          "number of samples to take and average per voxel along each spatial dimension). "
                          "This should be supplied as a vector of 3 integers. By default, the "
                          "oversampling factor is determined based on the differences between "
                          "input and output voxel sizes.")
    + Argument ("factors").type_sequence_int()

  + Option ("nan", "Use NaN as the out of bounds value (Default: 0.0)")

  + Option ("directions", "the directions used for FOD reorienation using apodised point spread functions (Default: 60 directions)")
    + Argument ("file", "a list of directions [az el] generated using the gendir command.").type_file()

  + Option ("noreorientation", "turn off FOD reorientation. Reorientation is on by default if the number "
                               "of volumes in the 4th dimension corresponds to the number of coefficients in an "
                               "antipodally symmetric spherical harmonic series (i.e. 6, 15, 28, 45, 66 etc")

  + DataType::options ();
}



typedef float value_type;



void run ()
{
  Math::Matrix<float> linear_transform;

  Options opt = get_options ("linear");
  if (opt.size()) {
    linear_transform.load (opt[0][0]);
    if (linear_transform.rows() != 4 || linear_transform.columns() != 4)
      throw Exception ("transform matrix supplied in file \"" + opt[0][0] + "\" is not 4x4");
  }

  Ptr<Image::BufferPreload<value_type> > input_buffer;
  Image::Header input_header (argument[0]);
  Image::Header output_header (input_header);

  if (input_header.ndim() == 4) {
    Image::Stride::List stride (4);
    stride[0] = 2;
    stride[1] = 3;
    stride[2] = 4;
    stride[3] = 1;
    output_header.stride(0) = 2;
    output_header.stride(1) = 3;
    output_header.stride(2) = 4;
    output_header.stride(3) = 1;
    input_buffer = new Image::BufferPreload<value_type> (argument[0], stride);
  } else {
    input_buffer = new Image::BufferPreload<value_type> (argument[0]);
  }


  output_header.datatype() = DataType::from_command_line (output_header.datatype());

  bool inverse = get_options ("inverse").size();
  bool replace = get_options ("replace").size();

  if (inverse) {
    if (!linear_transform.is_set())
      throw Exception ("no transform provided for option '-inverse' (specify using '-transform' option)");
    Math::Matrix<float> I;
    Math::LU::inv (I, linear_transform);
    linear_transform.swap (I);
  }


  if (replace)
    if (!linear_transform.is_set())
      throw Exception ("no transform provided for option '-replace' (specify using '-transform' option)");

  opt = get_options ("template"); // need to reslice
  if (opt.size()) {
    std::string name = opt[0][0];
    Image::ConstHeader template_header (name);

    output_header.dim(0) = template_header.dim (0);
    output_header.dim(1) = template_header.dim (1);
    output_header.dim(2) = template_header.dim (2);

    output_header.vox(0) = template_header.vox (0);
    output_header.vox(1) = template_header.vox (1);
    output_header.vox(2) = template_header.vox (2);

    output_header.transform() = template_header.transform();
    output_header.comments().push_back ("resliced to reference image \"" + template_header.name() + "\"");


    int interp = 2;
    opt = get_options ("interp");
    if (opt.size())
      interp = opt[0][0];

    std::vector<int> oversample;
    opt = get_options ("oversample");
    if (opt.size()) {
      oversample = opt[0][0];

      if (oversample.size() != 3)
        throw Exception ("option \"oversample\" expects a vector of 3 values");

      if (oversample[0] < 1 || oversample[1] < 1 || oversample[2] < 1)
        throw Exception ("oversample factors must be greater than zero");
    }

    float out_of_bounds_value = 0.0;
    opt = get_options ("nan");
    if (opt.size())
      out_of_bounds_value = NAN;

    if (replace) {
      Image::Info& info_in (*input_buffer);
      info_in.transform().swap (linear_transform);
      linear_transform.clear();
    }

    Options opt = get_options ("noreorientation");
    bool do_reorientation = false;
    if (output_header.ndim() > 3) {
      value_type val = (Math::sqrt (float (1 + 8 * output_header.dim(3))) - 3.0) / 4.0;
      if (!(val - (int)val) && !opt.size()) {
        do_reorientation = true;
        CONSOLE ("SH series detected, performing apodised PSF reorientation");
      }
    }



    Math::Matrix<value_type> directions_cartesian;
    if (do_reorientation) {
      Math::Matrix<value_type> directions_el_az;
      opt = get_options ("directions");
      if (opt.size())
        directions_el_az.load(opt[0][0]);
      else
        DWI::Directions::electrostatic_repulsion_60 (directions_el_az);
      Math::SH::S2C (directions_el_az, directions_cartesian);
    }

    Image::BufferPreload<float>::voxel_type in (*input_buffer);

    Image::Buffer<float> output_buffer (argument[1], output_header);
    Image::Buffer<float>::voxel_type output_vox (output_buffer);

    switch (interp) {
      case 0:
        Image::Filter::reslice<Image::Interp::Nearest> (in, output_vox, linear_transform, oversample, out_of_bounds_value);
        break;
      case 1:
        Image::Filter::reslice<Image::Interp::Linear> (in, output_vox, linear_transform, oversample, out_of_bounds_value);
        break;
      case 2:
        Image::Filter::reslice<Image::Interp::Cubic> (in, output_vox, linear_transform, oversample, out_of_bounds_value);
        break;
      case 3:
        FAIL ("FIXME: sinc interpolation needs a lot of work!");
        Image::Filter::reslice<Image::Interp::Sinc> (in, output_vox, linear_transform, oversample, out_of_bounds_value);
        break;
      default:
        assert (0);
        break;
    }

    if (do_reorientation) {
      std::string msg ("reorienting...");
      Image::Registration::Transform::reorient (msg, output_vox, output_vox, linear_transform, directions_cartesian);
    }


  } else {
    // straight copy:
    if (linear_transform.is_set()) {
      output_header.comments().push_back ("transform modified");
      if (replace)
        output_header.transform().swap (linear_transform);
      else {
        Math::Matrix<float> M (output_header.transform());
        Math::mult (output_header.transform(), linear_transform, M);
      }
    }

    Image::Buffer<float>::voxel_type in (*input_buffer);

    Image::Buffer<float> data_out (argument[1], output_header);
    Image::Buffer<float>::voxel_type out (data_out);

    Image::copy_with_progress (in, out);
  }
}

