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
#include "dwi/gradient.h"
#include "image/registration/transform/reorient.h"


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
typedef Image::BufferPreload<value_type> InputBufferType;
typedef Image::Buffer<value_type> OutputBufferType;



void run ()
{
  Math::Matrix<float> linear_transform;

  Options opt = get_options ("linear");
  if (opt.size()) {
    linear_transform.load (opt[0][0]);
    if (linear_transform.rows() != 4 || linear_transform.columns() != 4)
      throw Exception ("transform matrix supplied in file \"" + opt[0][0] + "\" is not 4x4");
  }

  Image::Header input_header (argument[0]);
  Image::Header output_header (input_header);


  output_header.datatype() = DataType::from_command_line (output_header.datatype());

  bool inverse = get_options ("inverse").size();
  bool replace = get_options ("replace").size();

  if (inverse) {
    if (!linear_transform.is_set())
      throw Exception ("no transform provided for option '-inverse' (specify using '-transform' option)");
    Math::Matrix<float> I = Math::LU::inv (linear_transform);
    linear_transform.swap (I);
  }

  opt = get_options ("flip");
  if (opt.size()) {
    std::vector<int> axes = opt[0][0];
    Math::Matrix<float> flip (4,4);
    flip.identity();
    for (size_t i = 0; i < axes.size(); ++i) {
      if (axes[i] < 0 || axes[i] > 2)
        throw Exception ("axes supplied to -flip are out of bounds (" + std::string (opt[0][0]) + ")");
      flip(axes[i],3) += flip(axes[i],axes[i]) * input_header.vox(axes[i]) * (input_header.dim(axes[i])-1);
      flip(axes[i], axes[i]) *= -1.0;
    }

    if (!linear_transform.is_set()) {
      linear_transform.allocate (4,4);
      linear_transform.identity();
    }
    
    Math::Matrix<float> tmp;
    if (!replace) {
      Math::Matrix<float> irot = Math::LU::inv (input_header.transform());
      Math::mult (tmp, flip, irot);
      Math::mult (flip, input_header.transform(), tmp);
    }
    Math::mult (tmp, linear_transform, flip);
    linear_transform = tmp;
  }

  if (replace)
    if (!linear_transform.is_set())
      throw Exception ("no transform provided for option '-replace' (specify using '-transform' option)");




  if (linear_transform.is_set() && input_header.ndim() == 4) {
    try {
      Math::Matrix<float> grad = DWI::get_DW_scheme<float> (input_header);
      if (input_header.dim (3) == (int) grad.rows()) {
        INFO ("DW gradients will be reoriented");

        Math::Matrix<float> rot = linear_transform.sub(0,3,0,3);
        if (replace) {
          Math::Matrix<float> irot = Math::LU::inv (input_header.transform().sub (0,3,0,3));
          Math::mult (rot, linear_transform.sub(0,3,0,3), irot);
        }

        Math::Vector<float> g (3);
        for (size_t n = 0; n < grad.rows(); ++n) {
          Math::mult (g, rot, grad.row(n).sub(0,3));
          grad.row(n).sub(0,3) = g;
        }

        output_header.DW_scheme() = grad;
      }
    }
    catch (Exception& e) {
      e.display (2);
    }
  }


  opt = get_options ("noreorientation");
  bool do_reorientation = false;
  Math::Matrix<value_type> directions_cartesian;
  if (!opt.size() && linear_transform.is_set() && input_header.ndim() == 4 &&
      input_header.dim(3) >= 6 &&
      input_header.dim(3) == (int) Math::SH::NforL (Math::SH::LforN (input_header.dim(3)))) {
    do_reorientation = true;
    CONSOLE ("SH series detected, performing apodised PSF reorientation");

    Math::Matrix<value_type> directions_az_el;
    opt = get_options ("directions");
    if (opt.size())
      directions_az_el.load(opt[0][0]);
    else
      DWI::Directions::electrostatic_repulsion_60 (directions_az_el);
    Math::SH::S2C (directions_az_el, directions_cartesian);

  }


  opt = get_options ("template"); // need to reslice
  if (opt.size()) {
    INFO ("image will be regridded");

    std::string name = opt[0][0];
    Image::ConstHeader template_header (name);
    for (size_t i = 0; i < 3; ++i) {
       output_header.dim(i) = template_header.dim(i);
       output_header.vox(i) = template_header.vox(i);
    }
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

    Image::Stride::List stride = Image::Stride::get (input_header);

    InputBufferType input_buffer (input_header, stride);
    if (replace) {
      Image::Info& info_in (input_buffer);
      info_in.transform().swap (linear_transform);
      linear_transform.clear();
    }
    InputBufferType::voxel_type in (input_buffer);

    if (do_reorientation) {
      stride = Image::Stride::contiguous_along_axis (3, input_header);
      Image::Stride::set (output_header, stride);
    }

    OutputBufferType output_buffer (argument[1], output_header);
    OutputBufferType::voxel_type output_vox (output_buffer);

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

    if (do_reorientation)
      Image::Registration::Transform::reorient ("reorienting...", output_vox, output_vox, linear_transform, directions_cartesian);

  } 
  else {
    // straight copy:
    INFO ("image will not be regridded");

    if (linear_transform.is_set()) {
      output_header.comments().push_back ("transform modified");
      if (replace)
        output_header.transform().swap (linear_transform);
      else {
        Math::Matrix<float> M (output_header.transform());
        Math::mult (output_header.transform(), Math::LU::inv (linear_transform), M);
      }
    }

    Image::Buffer<value_type> input_buffer (input_header);
    Image::Buffer<value_type>::voxel_type in (input_buffer);

    OutputBufferType data_out (argument[1], output_header);
    OutputBufferType::voxel_type out (data_out);

    Image::copy_with_progress (in, out);

    if (do_reorientation) {
      Math::Matrix<float> transform (linear_transform);
      if (replace)
        Math::mult (transform, linear_transform, Math::LU::inv (output_header.transform()));
      Image::Registration::Transform::reorient ("reorienting...", out, out, transform, directions_cartesian);
    }
  }
}

