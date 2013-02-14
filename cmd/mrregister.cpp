/*
 Copyright 2012 Brain Research Institute, Melbourne, Australia

 Written by David Raffelt, 23/02/2012.

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
#include "image/buffer.h"
#include "image/buffer_scratch.h"
#include "image/buffer_preload.h"
#include "image/voxel.h"
#include "image/filter/reslice.h"
#include "image/interp/cubic.h"
#include "image/transform.h"
#include "image/adapter/reslice.h"
#include "image/registration/linear.h"
#include "image/registration/nonlinear.h"
#include "image/registration/metric/mean_squared.h"
#include "image/registration/transform/affine.h"
#include "image/registration/transform/rigid.h"
#include "dwi/directions/predefined.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "math/SH.h"
#include "math/LU.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

const char* transformation_choices[] = { "rigid", "affine", "syn", NULL };

const char* initialisation_choices[] = { "mass", "centre", "none",NULL };

void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
      + "register two images together using a rigid, affine or a symmetric diffeomorphic (SyN) transformation model."

      + "FOD registration (with apodised point spread reorientation) will be performed by default if the number of volumes "
        "in the 4th dimension equals the number of coefficients in an antipodally symmetric spherical harmonic series (e.g. 6, 15, 28 etc). "
        "The -no_reorientation option can be used to force off reorientation if required."

      + "By default this application will initialise the chosen registration type with transformations of lower degrees of freedom. "
        "For example, an affine registration will be automatically initialised with the results of a rigid registration, "
        "and SyN registration will be initialised with the result of an affine. To register using ONLY the selected transformation "
        "type, use the -only option."

      + "By default the output syn warp will be composed with the linear transformation. To output the warp "
        "as a separate transformation use the -separate option.";

  ARGUMENTS
      + Argument ("moving", "moving image").type_image_in ()
      + Argument ("type", "the registration type. Valid choices are: "
                          "rigid, affine or syn (Default: syn)").type_choice (transformation_choices)
      + Argument ("template", "the target (fixed or target) image").type_image_in ();


  OPTIONS
  + Option ("transformed", "the transformed moving image after registration to the template")
    + Argument ("image").type_image_out ()

  + Option ("rigid", "the output text file containing the rigid transformation as a 4x4 matrix")
    + Argument ("file").type_file ()

  + Option ("affine", "the output text file containing the affine transformation as a 4x4 matrix")
    + Argument ("file").type_file ()

  + Option ("warp", "the output non-linear transformation defined as a deformation field "
                    "(each voxel defines the corresponding image coordinates wrt to template space)")
    + Argument ("forward_warp inverse_warp").type_image_out ().allow_multiple ()

  + Option ("scale", "use a multi-resolution scheme by defining a scale factor for each level "
                     "using comma separated values (Default: 0.5,1)")
    + Argument ("factor").type_sequence_float ()

  + Option ("init", "initialise the centre of rotation and initial translation. Valid choices are: mass "
                    "(which uses the image center of mass), centre (geometric image centre) or none. "
                    "Default: mass (which may not be suited for multi-modality registration).")
    +Argument ("type").type_choice (initialisation_choices)

  + Option ("init_rigid", "initialise the rigid registration with the supplied rigid transformation (as a 4x4 matrix)")
    + Argument ("file").type_file ()

  + Option ("init_affine", "initialise the affine registration with the supplied affine transformation (as a 4x4 matrix)")
    + Argument ("file").type_file ()

  + Option ("init_warp", "initialise the syn registration with the supplied warp")
      + Argument ("forward_warp inverse_warp").type_image_in ().allow_multiple ()

  + Option ("tmask", "a mask to define the target image region to use for optimisation.")
    + Argument ("filename").type_image_in ()

  + Option ("mmask", "a mask to define the moving image region to use for optimisation.")
    + Argument ("filename").type_image_in ()

  + Option ("niter_rigid", "the maximum number of iterations. This can be specified either as a single number "
                           "for all multi-resolution levels, or a single value for each level. (Default: 1000)")
    + Argument ("num").type_sequence_int ()

  + Option ("niter_affine", "the maximum number of iterations. This can be specified either as a single number "
                           "for all multi-resolution levels, or a single value for each level. (Default: 1000)")
    + Argument ("num").type_sequence_int ()

  + Option ("niter_syn", "the maximum number of iterations. This can be specified either as a single number "
                           "for all multi-resolution levels, or a single value for each level. (Default: 1000)")
    + Argument ("num").type_sequence_int ()

  + Option ("smooth_grad", "regularise the gradient field with Gaussian smoothing (standard deviation in mm, Default 3 x voxel_size)")
    + Argument ("stdev").type_float ()

  + Option ("smooth_disp", "regularise the displacement field with Gaussian smoothing (standard deviation in mm, Default 0.5 x voxel_size)")
    + Argument ("stdev").type_float ()

  + Option ("grad_step", "the initial gradient step size for SyN registration (Default: 0.12)") //TODO
    + Argument ("num").type_float ()

  + Option ("directions", "the directions used for FOD reorienation using apodised point spread functions (Default: 60 directions)")
    + Argument ("file", "a list of directions [az el] generated using the gendir command.").type_file()

  + Option ("lmax", "explicitly set the lmax to be used in FOD registration. By default FOD registration will "
                    "first use lmax 2 until convergence, then add lmax 4 SH coefficients and run till convergence")
    + Argument ("num").type_integer ()

  + Option ("no_reorientation", "turn off FOD reorientation. Reorientation is performed by default if the number "
                                "of volumes in the 4th dimension corresponds to the number of coefficients in an "
                                "antipodally symmetric spherical harmonic series (i.e. 6, 15, 28, 45, 66 etc")

  + Option ("only", "only perform the chosen registration type (i.e. do not initialise using transformations of lower degrees of freedom.")

  + Option ("separate", "ensure output warps are not composed with linear transformation.");
}

typedef float value_type;

void load_image (std::string filename, size_t num_vols, Ptr<Image::BufferScratch<value_type> >& image_buffer) {
  Image::Buffer<value_type> buffer (filename);
  Image::Buffer<value_type>::voxel_type vox (buffer);
  Image::Header header (filename);
  Image::Info info (header);
  if (num_vols > 1) {
    info.dim(3) = num_vols;
    info.stride(0) = 2;
    info.stride(1) = 3;
    info.stride(2) = 4;
    info.stride(3) = 1;
  }
  image_buffer = new Image::BufferScratch<value_type> (info);
  Image::BufferScratch<value_type>::voxel_type image_vox (*image_buffer);
  if (num_vols > 1) {
    Image::LoopInOrder loop (vox, 0, 3);
    for (loop.start (vox, image_vox); loop.ok(); loop.next (vox, image_vox)) {
      for (size_t vol = 0; vol < num_vols; ++vol) {
        vox[3] = image_vox[3] = vol;
        image_vox.value() = vox.value();
      }
    }
  } else {
    Image::threaded_copy (vox, image_vox);
  }
}


void run ()
{

  const Image::Header moving_header (argument[0]);
  const Image::Header template_header (argument[2]);

  Image::check_dimensions (moving_header, template_header);
  Ptr<Image::BufferScratch<value_type> > moving_image_ptr;
  Ptr<Image::BufferScratch<value_type> > template_image_ptr;

  Options opt = get_options ("do_reorientation");
  bool do_reorientation = true;
  if (opt.size())
    do_reorientation = false;

  if (template_header.ndim() > 3) {
    value_type val = (Math::sqrt (float (1 + 8 * template_header.dim(3))) - 3.0) / 4.0;
    if (!(val - (int)val) && do_reorientation) {
        CONSOLE ("SH series detected, performing FOD registration");
        // Only load as many SH coefficients as required
        int lmax = 4;
        Options opt = get_options ("lmax");
        if (opt.size()) {
          lmax = opt[0][0];
          if (lmax % 2)
            throw Exception ("the input lmax must be even");
        }
        int num_SH = Math::SH::NforL (lmax);
        if (num_SH > template_header.dim(3))
            throw Exception ("not enough SH coefficients within input image for desired lmax");
        load_image(argument[0], num_SH, moving_image_ptr);
        load_image(argument[2], num_SH, template_image_ptr);
    } else {
      load_image (argument[0], moving_header.dim(3), moving_image_ptr);
      load_image (argument[2], template_header.dim(3), template_image_ptr);
    }
  } else {
    do_reorientation = false;
    load_image (argument[0], 1, moving_image_ptr);
    load_image (argument[2], 1, template_image_ptr);
  }

  opt = get_options ("transformed");
  Ptr<Image::Buffer<value_type> > transformed_buffer;
  if (opt.size())
    transformed_buffer = new Image::Buffer<value_type> (opt[0][0], template_header);

  const int registration_type = argument[1];
  opt = get_options ("only");
  bool only = opt.size() ? true : false;

  opt = get_options ("separate");
  bool separate_transforms = opt.size() ? true : false;

  if (separate_transforms && only)
    throw Exception ("conflicting options detected (-only & -separate)");

  bool do_rigid  = false;
  bool do_affine = false;
  bool do_syn = false;
  switch (registration_type) {
    case 0:
      do_rigid = true;
      break;
    case 1:
      do_affine = true;
      if (!only)
        do_rigid = true;
      break;
    case 2:
      do_syn = true;
      if (!only)
        do_rigid = true;
        do_affine = true;
        break;
    default:
      break;
  }

  opt = get_options ("rigid");
  bool output_rigid = false;
  std::string rigid_filename;
  if (opt.size()) {
    output_rigid = true;
    rigid_filename = std::string(opt[0][0]);
    if (!do_rigid)
      throw Exception ("rigid transformation output requested when no rigid registration is to be performed.");
  }

  opt = get_options ("affine");
   bool output_affine = false;
   std::string affine_filename;
   if (opt.size()) {
     output_affine = true;
     affine_filename = std::string(opt[0][0]);
     if (!do_affine)
       throw Exception ("affine transformation output requested when no affine registration is to be performed.");
   }

  opt = get_options ("warp");
  Image::Header warp_header (argument[2]);
  warp_header.set_ndim (4);
  warp_header.dim(3) = 3;
  warp_header.stride(0) = 2;
  warp_header.stride(1) = 3;
  warp_header.stride(2) = 4;
  warp_header.stride(3) = 1;
  Ptr<Image::Buffer<value_type> > warp_buffer;
  bool output_syn = false;
  if (opt.size()) {
    warp_buffer = new Image::Buffer<value_type> (argument[3], warp_header);
    output_syn = true;
    if (!do_syn)
      throw Exception ("syn transformation output requested when no syn registration is to be performed.");
  }


  opt = get_options ("scale");
  std::vector<value_type> scale_factors(2);
  scale_factors[0] = 0.5;
  scale_factors[1] = 1;
  if (opt.size ()) {
    scale_factors = parse_floats (opt[0][0]);
    for (size_t i = 0; i < scale_factors.size(); ++i)
      if (scale_factors[i] < 0)
        throw Exception ("the multi-resolution scale factor must be positive");
  }

  opt = get_options ("init");
  int init = 0;
  if (opt.size())
    init = opt[0][0];

  opt = get_options ("init_rigid");
  Math::Matrix<value_type> init_rigid;
  if (opt.size())
    init_rigid.load (opt[0][0]);

  opt = get_options ("init_affine");
  Math::Matrix<value_type> init_affine;
  if (opt.size())
    init_affine.load (opt[0][0]);

  opt = get_options ("init_warp");
  Ptr<Image::Buffer<value_type> > init_warp_buffer;
  if (opt.size())
    init_warp_buffer = new Image::Buffer<value_type> (opt[0][0]);

  opt = get_options ("tmask");
  Ptr<Image::BufferPreload<bool> > tmask_image;
  if (opt.size ())
    tmask_image = new Image::BufferPreload<bool> (opt[0][0]);

  opt = get_options ("mmask");
  Ptr<Image::BufferPreload<bool> > mmask_image;
  if (opt.size ())
    mmask_image = new Image::BufferPreload<bool> (opt[0][0]);

  opt = get_options ("niter_rigid");
  std::vector<int> niter_rigid (1, 1000);

  if (opt.size ()) {
    niter_rigid = parse_ints (opt[0][0]);
    for (size_t i = 0; i < niter_rigid.size (); ++i)
      if (niter_rigid[i] < 0)
        throw Exception ("the number of rigid iterations must be positive");
    if (!do_rigid)
      throw Exception ("the number of rigid iterations have been input when no rigid registration is to be performed");
  }

  opt = get_options ("niter_affine");
  std::vector<int> niter_affine (1, 1000);
  if (opt.size ()) {
    niter_affine = parse_ints (opt[0][0]);
    for (size_t i = 0; i < niter_affine.size (); ++i)
      if (niter_affine[i] < 0)
        throw Exception ("the number of affine iterations must be positive");
    if (!do_affine)
      throw Exception ("the number of affine iterations have been input when no affine registration is to be performed");
  }

  opt = get_options ("niter_syn");
  std::vector<int> niter_syn (1, 1000);
  if (opt.size ()) {
    if (!do_syn)
    niter_syn = parse_ints (opt[0][0]);
    for (size_t i = 0; i < niter_syn.size (); ++i)
      if (niter_syn[i] < 0)
        throw Exception ("the number of syn iterations must be positive");
    if (!do_syn)
      throw Exception ("the number of syn iterations have been input when no syn registration is to be performed");
  }

  opt = get_options ("smooth_grad");
  value_type smooth_grad =  template_header.vox(0) + template_header.vox(1) + template_header.vox(2);
  if (opt.size())
    smooth_grad = opt[0][0];

  opt = get_options ("smooth_disp");
  value_type smooth_disp =  (template_header.vox(0) + template_header.vox(1) + template_header.vox(2)) / 3.0;
  if (opt.size())
    smooth_disp = opt[0][0];

  opt = get_options ("directions");
  Math::Matrix<value_type> directions;
  DWI::Directions::electrostatic_repulsion_60 (directions);
  if (opt.size())
    directions.load(opt[0][0]);

  Image::Registration::Linear registration;
  registration.set_scale_factor (scale_factors);

  switch (init) {
    case 0:
      registration.set_init_type (Image::Registration::Transform::Init::mass);
      break;
    case 1:
      registration.set_init_type (Image::Registration::Transform::Init::centre);
      break;
    case 2:
      registration.set_init_type (Image::Registration::Transform::Init::none);
      break;
    default:
      break;
  }

  Image::Registration::Metric::MeanSquared metric;
  Image::Registration::Transform::Rigid<double> rigid;
  Image::Registration::Transform::Affine<double> affine;

  if (registration_type == 0) {

    CONSOLE ("running rigid registration");
    registration.set_max_iter (niter_rigid);
    registration.run_masked (metric, rigid, *moving_image_ptr, *template_image_ptr, mmask_image, tmask_image);
    if (output_rigid)
      rigid.get_transform().save (rigid_filename);

  } else if (registration_type == 1) {

    if (!only) {
      CONSOLE ("running rigid registration");
      Image::Registration::Transform::Rigid<double> rigid;
      registration.set_max_iter (niter_rigid);
      registration.run_masked (metric, rigid, *moving_image_ptr, *template_image_ptr, mmask_image, tmask_image);
      if (output_rigid)
        rigid.get_transform().save (rigid_filename);
      affine.set_centre (rigid.get_centre());
      affine.set_translation (rigid.get_translation());
      affine.set_matrix (rigid.get_matrix());
      registration.set_init_type (Image::Registration::Transform::Init::none);
    }
    CONSOLE ("running affine registration");
    registration.set_max_iter (niter_affine);
    registration.run_masked (metric, affine, *moving_image_ptr, *template_image_ptr, mmask_image, tmask_image);
    if (output_affine)
      affine.get_transform().save (affine_filename);

  } else {
    CONSOLE ("running syn registration");
  }

  if (transformed_buffer) {
    Image::Buffer<value_type>::voxel_type transformed_vox (*transformed_buffer);
    Image::Buffer<value_type> moving_buffer (moving_header);
    Image::Buffer<value_type>::voxel_type moving_vox (moving_buffer);
    if (do_syn) {
      if (do_reorientation) {
        CONSOLE ("REORIENTATION NOT YET IMPLEMENTED");
      } else {
        CONSOLE ("WARPING NOT YET IMPLEMENTED");
      }
    } else if (do_affine) {
      if (do_reorientation) {
        CONSOLE ("REORIENTATION NOT YET IMPLEMENTED");
      } else {
        Image::Filter::reslice<Image::Interp::Cubic> (moving_vox, transformed_vox, affine.get_transform(), Image::Adapter::AutoOverSample, 0.0);
      }
    } else {
      if (do_reorientation) {
        CONSOLE ("REORIENTATION NOT YET IMPLEMENTED");
      } else {
        Image::Filter::reslice<Image::Interp::Cubic> (moving_vox, transformed_vox, rigid.get_transform(), Image::Adapter::AutoOverSample, 0.0);
      }
    }
  }


}
