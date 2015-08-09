/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier and David Raffelt

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
#include "filter/warp.h"
#include "algo/loop.h"
#include "algo/copy.h"
#include "dwi/directions/predefined.h"
#include "dwi/gradient.h"
#include "registration/transform/reorient.h"



using namespace MR;
using namespace App;

const char* interp_choices[] = { "nearest", "linear", "cubic", "sinc", NULL };

void usage ()
{

  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com) & David Raffelt (david.raffelt@florey.edu.au)";

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
    "Magnetic Resonance in Medicine, 2012, 67, 844-855"

    + "If FOD modulation is being performed:\n"
    "Raffelt, D.; Tournier, J.-D.; Rose, S.; Ridgway, G.R.; Henderson, R.; Crozier, S.; Salvado, O.; Connelly, A.; "
    "Apparent Fibre Density: a novel measure for the analysis of diffusion-weighted magnetic resonance images. "
    "Neuroimage, 2012, 15;59(4), 3976-94.";

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
        "set the interpolation method to use when reslicing (choices: nearest, linear, cubic, sinc. Default: cubic).")
    + Argument ("method").type_choice (interp_choices)

    + OptionGroup ("Non-linear transformation options")

    + Option ("warp",
        "apply a non-linear deformation field to warp the input image.")
    + Argument ("image").type_image_in ()

    + OptionGroup ("Fibre orientation distribution handling options")

    + Option ("modulate",
        "modulate the FOD during reorientation to preserve the apparent fibre density")

    + Option ("directions", 
        "directions defining the number and orientation of the apodised point spread functions used in FOD reorientation"
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


void run ()
{
  auto input_header = Header::open (argument[0]);
  Header output_header (input_header);
  output_header.datatype() = DataType::from_command_line (output_header.datatype());

  const bool replace = get_options ("replace").size();

  // Linear
  transform_type linear_transform;
  bool linear = false;
  auto opt = get_options ("linear");
  if (opt.size()) {
    linear = true;
    linear_transform = load_transform (opt[0][0]);
  } else {
    linear_transform.setIdentity();
    if (replace)
      throw Exception ("no transform provided for option '-replace'");
  }

  // Inverse
  const bool inverse = get_options ("inverse").size();
  if (inverse) {
    if (!linear)
      throw Exception ("no transform provided for option '-inverse'");
    linear_transform = linear_transform.inverse();
  }


  // Flip
  opt = get_options ("flip");
  if (opt.size()) {
    std::vector<int> axes = opt[0][0];
    transform_type flip;
    flip.setIdentity();
    for (size_t i = 0; i < axes.size(); ++i) {
      if (axes[i] < 0 || axes[i] > 2)
        throw Exception ("axes supplied to -flip are out of bounds (" + std::string (opt[0][0]) + ")");
      flip(axes[i],3) += flip(axes[i],axes[i]) * input_header.voxsize(axes[i]) * (input_header.size(axes[i])-1);
      flip(axes[i], axes[i]) *= -1.0;
    }
    transform_type tmp;
    if (!replace) {
      transform_type irot = input_header.transform().inverse();
      tmp = flip * irot;
      flip = input_header.transform() * tmp;
    }
    tmp = linear_transform * flip;
    linear_transform = tmp;
  }

  Stride::List stride = Stride::get (input_header);



  auto input = input_header.get_image<float>().with_direct_io (stride);

  // Warp
  opt = get_options ("warp");
  std::shared_ptr<Image<float> > warp_ptr;
  if (opt.size())
    warp_ptr = std::make_shared<Image<float> > (Image<float>::open(opt[0][0]));


  // Detect FOD image for reorientation
  opt = get_options ("noreorientation");
  bool fod_reorientation = false;
  Eigen::MatrixXd directions_cartesian;
  if (!opt.size() && (linear || warp_ptr) && input_header.ndim() == 4 &&
      input_header.size(3) >= 6 &&
      input_header.size(3) == (int) Math::SH::NforL (Math::SH::LforN (input_header.size(3)))) {
    CONSOLE ("SH series detected, performing apodised PSF reorientation");
    fod_reorientation = true;

    Eigen::MatrixXd directions_el_az;
    opt = get_options ("directions");
    if (opt.size())
      directions_el_az = load_matrix (opt[0][0]);
    else
      directions_el_az = DWI::Directions::electrostatic_repulsion_60();
    Math::SH::spherical2cartesian (directions_el_az, directions_cartesian);

    // load with SH coeffients contiguous in RAM
    stride = Stride::contiguous_along_axis (3, input_header);
  }

  bool modulate = false;
  if (get_options ("modulate").size()) {
    modulate = true;
    if (!fod_reorientation)
      throw Exception ("modulation can only be performed with FOD reorientation");
  }


  if (linear && input_header.ndim() == 4 && !warp_ptr && !fod_reorientation) {
    try {
      auto grad = DWI::get_DW_scheme (input_header);
      if (input_header.size(3) == (ssize_t) grad.rows()) {
        INFO ("DW gradients will be reoriented");

        // TODO do we want to detect and warn about shears here?
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



  opt = get_options ("template"); // need to reslice
  if (opt.size()) {
    INFO ("image will be regridded");

    if (replace)
      throw Exception ("you cannot use the -replace option with the -template option");

    auto template_header = Header::open (opt[0][0]);
    for (size_t i = 0; i < 3; ++i) {
       output_header.size(i) = template_header.size(i);
       output_header.voxsize(i) = template_header.voxsize(i);
    }
    output_header.transform() = template_header.transform();
    add_line (output_header.keyval()["comments"], std::string ("resliced to template image \"" + template_header.name() + "\""));

    int interp = 2;  // cubic
    opt = get_options ("interp");
    if (opt.size())
      interp = opt[0][0];

    float out_of_bounds_value = 0.0;
    opt = get_options ("nan");
    if (opt.size())
      out_of_bounds_value = NAN;



    // compose warp with affine
    std::shared_ptr<Image<float> > warp_composed_ptr;
    if (warp_ptr && linear) {
      warp_composed_ptr = std::make_shared<Image<float> > (Image<float>::scratch (*warp_ptr));
      Registration::Transform::compose (linear_transform, *warp_ptr, *warp_composed_ptr);
    } else {
      warp_composed_ptr = warp_ptr;
    }

    auto output = Image<float>::create (argument[1], output_header);

      switch (interp) {
      case 0:
        if (!warp_ptr)
          Filter::reslice<Interp::Nearest> (input, output, linear_transform, Adapter::AutoOverSample, out_of_bounds_value);
        else
          Filter::warp<Interp::Nearest> (input, output, *warp_composed_ptr, out_of_bounds_value);
        break;
      case 1:
        if (!warp_ptr)
          Filter::reslice<Interp::Linear> (input, output, linear_transform, Adapter::AutoOverSample, out_of_bounds_value);
        else
          Filter::warp<Interp::Linear> (input, output, *warp_composed_ptr, out_of_bounds_value);
        break;
      case 2:
        if (!warp_ptr)
          Filter::reslice<Interp::Cubic> (input, output, linear_transform, Adapter::AutoOverSample, out_of_bounds_value);
        else
          Filter::warp<Interp::Cubic> (input, output, *warp_composed_ptr, out_of_bounds_value);
        break;
      case 3:
        if (!warp_ptr)
          Filter::reslice<Interp::Sinc> (input, output, linear_transform, Adapter::AutoOverSample, out_of_bounds_value);
        else
          Filter::warp<Interp::Sinc> (input, output, *warp_composed_ptr, out_of_bounds_value);
        break;
      default:
        assert (0);
        break;
    }

    // only reorient if linear or warp input
    if (fod_reorientation && linear && !warp_ptr)
      Registration::Transform::reorient ("reorienting...", output, linear_transform, directions_cartesian.transpose(), modulate);
    else if (fod_reorientation && warp_ptr)
      Registration::Transform::reorient_warp ("reorienting...", output, *warp_composed_ptr, directions_cartesian.transpose(), modulate);

  } else {
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

    if (fod_reorientation) {
      transform_type transform = linear_transform;
      if (replace)
        transform = linear_transform * output_header.transform().inverse();
      Registration::Transform::reorient ("reorienting...", output, transform, directions_cartesian.transpose());
    }
  }
}

