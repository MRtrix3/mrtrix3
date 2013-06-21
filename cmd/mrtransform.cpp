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
#include "image/voxel.h"
#include "image/interp/nearest.h"
#include "image/interp/linear.h"
#include "image/interp/cubic.h"
#include "image/interp/sinc.h"
#include "image/filter/reslice.h"
#include "image/utils.h"
#include "image/loop.h"
#include "image/copy.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

const char* interp_choices[] = { "nearest", "linear", "cubic", "sinc", NULL };

void usage ()
{
  DESCRIPTION
  + "apply spatial transformations or reslice images."
  + "In most cases, this command will only modify the transform matrix, "
  "without reslicing the image. Only the \"reslice\" option will "
  "actually modify the image data.";

  ARGUMENTS
  + Argument ("input", "input image to be transformed.").type_image_in ()
  + Argument ("output", "the output image.").type_image_out ();

  OPTIONS
  + Option ("linear", "specify a 4x4 linear transform to apply, in the form "
                      "of a 4x4 ascii file. Note the standard 'reverse' convention "
                      "is used, where the transform maps points in the template image "
                      "to the moving image.")
  + Argument ("transform").type_file ()

  + Option ("warp",
            "apply a non-linear transform to the input image.")
  + Argument ("image").type_image_in ()

  + Option ("template",
            "reslice the input image to match the specified template image.")
  + Argument ("image").type_image_in ()

  + Option ("inverse",
            "apply the inverse transformation")

  + Option ("replace",
            "replace the linear transform of the original image by that specified, "
            "rather than applying it to the original image.")

  + Option ("interp",
            "set the interpolation method to use when reslicing (default: cubic).")
  + Argument ("method").type_choice (interp_choices)

  + Option ("oversample",
            "set the oversampling factor to use when reslicing (i.e. the "
            "number of samples to take per voxel along each spatial dimension). "
            "This should be supplied as a vector of 3 integers. By default, the "
            "oversampling factor is determined based on the differences between "
            "input and output voxel sizes.")
  + Argument ("factors").type_sequence_int()

  + Option ("nan",
            "Use NaN as the out of bounds value (Default: 0.0)")

  + DataType::options ();
}



typedef float value_type;



void run ()
{
  Math::Matrix<float> linear_transform;

  Options opt = get_options ("transform");
  if (opt.size()) {
    linear_transform.load (opt[0][0]);
    if (linear_transform.rows() != 4 || linear_transform.columns() != 4)
      throw Exception ("transform matrix supplied in file \"" + opt[0][0] + "\" is not 4x4");
  }


  Image::Buffer<value_type> data_in (argument[0]);
  Image::Header header_out (data_in);

  header_out.datatype() = DataType::from_command_line (header_out.datatype());

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

  opt = get_options ("reslice"); // need to reslice
  if (opt.size()) {
    std::string name = opt[0][0];
    Image::ConstHeader template_header (name);

    header_out.dim(0) = template_header.dim (0);
    header_out.dim(1) = template_header.dim (1);
    header_out.dim(2) = template_header.dim (2);

    header_out.vox(0) = template_header.vox (0);
    header_out.vox(1) = template_header.vox (1);
    header_out.vox(2) = template_header.vox (2);

    header_out.transform() = template_header.transform();
    header_out.comments().push_back ("resliced to reference image \"" + template_header.name() + "\"");


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
    Image::Info& info_in (data_in);
    info_in.transform().swap (linear_transform);
    linear_transform.clear();
  }

  Image::Buffer<float>::voxel_type in (data_in);

  Image::Buffer<float> data_out (argument[1], header_out);
  Image::Buffer<float>::voxel_type out (data_out);

  switch (interp) {
    case 0:
      Image::Filter::reslice<Image::Interp::Nearest> (in, out, linear_transform, oversample, out_of_bounds_value);
      break;
    case 1:
      Image::Filter::reslice<Image::Interp::Linear> (in, out, linear_transform, oversample, out_of_bounds_value);
      break;
    case 2:
      Image::Filter::reslice<Image::Interp::Cubic> (in, out, linear_transform, oversample, out_of_bounds_value);
      break;
    case 3:
      FAIL ("FIXME: sinc interpolation needs a lot of work!");
      Image::Filter::reslice<Image::Interp::Sinc> (in, out, linear_transform, oversample, out_of_bounds_value);
      break;
    default:
      assert (0);
      break;
  }

  } else {
    // straight copy:
    if (linear_transform.is_set()) {
      header_out.comments().push_back ("transform modified");
      if (replace)
        header_out.transform().swap (linear_transform);
      else {
        Math::Matrix<float> M (header_out.transform());
        Math::mult (header_out.transform(), linear_transform, M);
      }
    }

    Image::Buffer<float>::voxel_type in (data_in);

    Image::Buffer<float> data_out (argument[1], header_out);
    Image::Buffer<float>::voxel_type out (data_out);

    Image::copy_with_progress (in, out);
  }
}

