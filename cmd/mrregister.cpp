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
#include "image/buffer_preload.h"
#include "image/voxel.h"
#include "image/filter/reslice.h"
#include "image/interp/cubic.h"
#include "image/transform.h"
#include "image/adapter/reslice.h"
#include "registration/linear_registration.h"
#include "registration/metric/mean_squared_metric.h"
#include "registration/transform/affine.h"
#include "registration/transform/rigid.h"
#include "math/hemisphere/predefined_dirs.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "math/SH.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;
using namespace Registration;

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

      + "By default the output transformation will be composed with those of lower degrees of freedom. To output each "
        "transformation separately use the -separate option.";

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
    + Argument ("image").type_image_out ()

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
      + Argument ("image").type_image_in ()

  + Option ("tmask", "a mask to define the target image region to use for optimisation.")
    + Argument ("filename").type_image_in ()

  + Option ("mmask", "a mask to define the moving image region to use for optimisation.")
    + Argument ("filename").type_image_in ()

  + Option ("rigid_niter", "the maximum number of iterations. This can be specified either as a single number "
                           "for all multi-resolution levels, or a single value for each level. (Default: 1000)")
    + Argument ("num").type_sequence_int ()

  + Option ("affine_niter", "the maximum number of iterations. This can be specified either as a single number "
                           "for all multi-resolution levels, or a single value for each level. (Default: 1000)")
    + Argument ("num").type_sequence_int ()

  + Option ("syn_niter", "the maximum number of iterations. This can be specified either as a single number "
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

  + Option ("separate", "output transforms thar are not composed with transformations of lower degrees of freedom.");
}

typedef float value_type;

void run ()
{

  const Image::Header moving_header (argument[0]);
  const int registration_type = argument[1];
  const Image::Header template_header (argument[2]);

  Image::check_dimensions (moving_header, template_header);
  Ptr<Image::BufferScratch<value_type> > moving_buffer_ptr;
  Ptr<Image::BufferScratch<value_type>::voxel_type> moving_vox_ptr;
  Ptr<Image::BufferScratch<value_type> > template_buffer_ptr;
  Ptr<Image::BufferScratch<value_type>::voxel_type> template_vox_ptr;

  bool do_reorientation = false;
  if (template_header.ndim() == 4) {
    if (template_header.dim(3) == 6 ||
        template_header.dim(3) == 15 ||
        template_header.dim(3) == 28 ||
        template_header.dim(3) == 45 ||
        template_header.dim(3) == 66 ||
        template_header.dim(3) == 91 ||
        template_header.dim(3) == 120) {

      // Only load as many SH coefficients as required
      do_reorientation = true;
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

      Image::Buffer<value_type> moving_buffer (argument[0]);
      Image::Buffer<value_type>::voxel_type moving_vox (moving_buffer);
      Image::Buffer<value_type> template_buffer (argument[1]);
      Image::Buffer<value_type>::voxel_type template_vox (template_buffer);

      Image::Info moving_info (argument[0]);
      moving_info.dim (3) = num_SH;
      moving_info.stride(0) = 2;
      moving_info.stride(1) = 3;
      moving_info.stride(2) = 4;
      moving_info.stride(3) = 1;
      moving_buffer_ptr = new Image::BufferScratch<value_type> (moving_info);
      //HERE!!
//      Image::LoopInOrder (moving_vox_ptr)

    }
  } else {
  }




//  Image::BufferPreload<value_type> template_data (argument[2]);
//  Image::BufferPreload<value_type>::voxel_type template_vox (template_data);


  Options opt = get_options ("transformed");
  Ptr<Image::Buffer<value_type> > transformed_buffer;
  Ptr<Image::Buffer<value_type>::voxel_type > transformed_voxel;
  if (opt.size()) {
    transformed_buffer = new Image::Buffer<value_type> (argument[3], template_data);
    transformed_voxel = new Image::Buffer<value_type>::voxel_type (*transformed_buffer);
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
  Ptr<Image::Buffer<value_type>::voxel_type > warp_voxel;
  if (opt.size()) {
    warp_buffer = new Image::Buffer<value_type> (argument[3], warp_header);
    warp_voxel = new Image::Buffer<value_type>::voxel_type (*warp_buffer);
  }

  opt = get_options ("scale");
  std::vector<value_type> scale_factors (2);
  scale_factors[0] = 0.5;
  scale_factors[1] = 1;
  if (opt.size ()) {
    scale_factors = parse_floats (opt[0][0]);
    for (size_t i = 0; i < scale_factors.size(); ++i)
      if (scale_factors[i] < 0)
        throw Exception ("the multi-resolution scale factor must be positive");
  }

  opt = get_options ("init");
  const int init = 0;
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

  // TODO think about interpolation and preload strides
  opt = get_options ("init_warp");
  Ptr<Image::Buffer<value_type> > init_warp_buffer;
  Ptr<Image::Buffer<value_type>::voxel_type > init_warp_voxel;
  if (opt.size()) {
    init_warp_buffer = new Ptr<Image::Buffer<value_type> > (opt[0][0]);
    init_warp_voxel = new Ptr<Image::Buffer<value_type> >::voxel_type (*init_warp_buffer);
  }

  opt = get_options ("tmask");
  Ptr<Image::BufferPreload<bool> > tmask_data;
  Ptr<Image::BufferPreload<bool>::voxel_type> tmask_voxel;
  if (opt.size ()) {
    tmask_data = new Image::BufferPreload<bool> (opt[0][0]);
    tmask_voxel = new Image::BufferPreload<bool>::voxel_type (*tmask_data);
  }

  opt = get_options ("mmask");
  Ptr<Image::BufferPreload<bool> > mmask_data;
  Ptr<Image::BufferPreload<bool>::voxel_type> mmask_voxel;
  if (opt.size ()) {
    mmask_data = new Image::BufferPreload<bool> (opt[0][0]);
    mmask_voxel = new Image::BufferPreload<bool>::voxel_type (*mmask_data);
  }

  opt = get_options ("rigid_niter");
  std::vector<int> rigid_niter (1, 1000);
  if (opt.size ()) {
    rigid_niter = parse_ints (opt[0][0]);
    for (size_t i = 0; i < rigid_niter.size (); ++i)
      if (rigid_niter[i] < 0)
        throw Exception ("the number of rigid iterations must be positive");
  }

  opt = get_options ("affine_niter");
  std::vector<int> affine_niter (1, 1000);
  if (opt.size ()) {
    affine_niter = parse_ints (opt[0][0]);
    for (size_t i = 0; i < affine_niter.size (); ++i)
      if (affine_niter[i] < 0)
        throw Exception ("the number of affine iterations must be positive");
  }

  opt = get_options ("syn_niter");
  std::vector<int> syn_niter (1, 1000);
  if (opt.size ()) {
    syn_niter = parse_ints (opt[0][0]);
    for (size_t i = 0; i < syn_niter.size (); ++i)
      if (syn_niter[i] < 0)
        throw Exception ("the number of syn iterations must be positive");
  }

  opt = get_options ("smooth_grad");
  value_type smooth_grad =  template_vox.vox(0) + template_vox.vox(1) + template_vox.vox(2);
  if (opt.size())
    smooth_grad = opt[0][0];

  opt = get_options ("smooth_disp");
  value_type smooth_disp =  (template_vox.vox(0) + template_vox.vox(1) + template_vox.vox(2)) / 3.0;
  if (opt.size())
    smooth_disp = opt[0][0];

  opt = get_options ("directions");
  Math::Matrix<value_type> directions;
  Math::Hemisphere::Directions directions_60 (directions);
  if (opt.size())
    directions.load(opt[0][0]);



  LinearRegistration registration;
  registration.set_max_iter (niter);
  registration.set_scale_factor (scale_factors);


  switch (init) {
    case 0:
      registration.set_init_type (Transform::Init::mass);
      break;
    case 1:
      registration.set_init_type (Transform::Init::centre);
      break;
    case 2:
      registration.set_init_type (Transform::Init::none);
      break;
    default:
      break;
  }

  Metric::MeanSquared metric;

  int transform_type = 1;
  opt = get_options ("transform");
  if (opt.size())
    transform_type = opt[0][0];

  Math::Matrix <double> final_transform;
  if (transform_type == 0) {

    Transform::Rigid<double> rigid;
    CONSOLE ("running rigid registration");
    registration.run_masked (metric, rigid, moving_vox, template_vox, mmask_voxel, tmask_voxel);
    rigid.get_transform (final_transform);
    final_transform.save (argument[2]);
    Image::Filter::reslice<Image::Interp::Cubic> (moving_vox, output_voxel, final_transform, Image::Adapter::AutoOverSample, 0.0);

  } else if (transform_type == 1) {

    Transform::Affine<double> affine;
    CONSOLE ("running affine registration");
    registration.run_masked (metric, affine, moving_vox, template_vox, mmask_voxel, tmask_voxel);
    affine.get_transform (final_transform);
    final_transform.save (argument[2]);
    Image::Filter::reslice<Image::Interp::Cubic> (moving_vox, output_voxel, final_transform, Image::Adapter::AutoOverSample, 0.0);

  } else {

    Transform::Rigid<double> rigid;
    CONSOLE ("running rigid registration");
    registration.run_masked (metric, rigid, moving_vox, template_vox, mmask_voxel, tmask_voxel);
    rigid.get_transform (final_transform);
    Math::Vector<double> centre;
    rigid.get_centre (centre);
    Transform::Affine<double> affine;
    affine.set_centre (centre);
    affine.set_transform (final_transform);
    registration.set_init_type (Transform::Init::none);
    CONSOLE ("running affine registration");
    registration.run_masked (metric, affine, moving_vox, template_vox, mmask_voxel, tmask_voxel);
    affine.get_transform (final_transform);
    final_transform.save (argument[2]);
    Image::Filter::reslice<Image::Interp::Cubic> (moving_vox, output_voxel, final_transform, Image::Adapter::AutoOverSample, 0.0);
  }

}
