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
#include "image/registration/syn.h"
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

const char* transformation_choices[] = { "rigid", "affine", "syn", "rigid_affine", "rigid_syn", "affine_syn", "rigid_affine_syn", NULL };


void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
      + "Register two images together using a rigid, affine or a symmetric diffeomorphic (SyN) transformation model."

      + "By default this application will perform an affine, followed by SyN registration. Use the "

      + "FOD registration (with apodised point spread reorientation) will be performed by default if the number of volumes "
        "in the 4th dimension equals the number of coefficients in an antipodally symmetric spherical harmonic series (e.g. 6, 15, 28 etc). "
        "The -no_reorientation option can be used to force reorientation off if required."

      + "SyN estimates both the warp and it's inverse. These are each split into two warps to achieve a symmetric transformation (i.e "
        "both the moving and template image are warped towards a 'middle ground'. See Avants (2008) Med Image Anal. 12(1): 26–41.) "
        "By default this application will save all four warps (so that subsequent registrations can be initialised with the output warps) "
        "Warps are saved in a single 5D file, with the 5th dimension defining the warp type. (These can be visualised by switching volume "
        "groups in MRview)."

      + "By default the affine transformation will be saved in the warp image header (use mrinfo to view). To save the affine transform "
        "separately as a text file, use the -affine option.";


  ARGUMENTS
    + Argument ("moving", "moving image").type_image_in ()
    + Argument ("template", "template image").type_image_in ();


  OPTIONS
  + Option ("registration", "the registration type. Valid choices are: "
                             "rigid, affine, syn, rigid_affine, rigid_syn, affine_syn, rigid_affine_syn (Default: affine_syn)")
    + Argument ("type").type_choice (transformation_choices)

  + Option ("transformed", "the transformed moving image after registration to the template")
    + Argument ("image").type_image_out ()

  + Option ("tmask", "a mask to define the template image region to use for optimisation.")
    + Argument ("filename").type_image_in ()

  + Option ("mmask", "a mask to define the moving image region to use for optimisation.")
    + Argument ("filename").type_image_in ()

  + Image::Registration::rigid_options

  + Image::Registration::affine_options

  + Image::Registration::syn_options

  + Image::Registration::initialisation_options

  + Image::Registration::fod_options;
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
  const Image::Header template_header (argument[1]);

  Image::check_dimensions (moving_header, template_header);
  Ptr<Image::BufferScratch<value_type> > moving_buffer_ptr;
  Ptr<Image::BufferScratch<value_type> > template_buffer_ptr;

  Options opt = get_options ("no_reorientation");
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
        load_image(argument[0], num_SH, moving_buffer_ptr);
        load_image(argument[1], num_SH, template_buffer_ptr);
    } else {
      load_image (argument[0], moving_header.dim(3), moving_buffer_ptr);
      load_image (argument[1], template_header.dim(3), template_buffer_ptr);
    }
  } else {
    do_reorientation = false;
    load_image (argument[0], 1, moving_buffer_ptr);
    load_image (argument[1], 1, template_buffer_ptr);
  }

  Image::BufferScratch<value_type>::voxel_type moving_voxel (*moving_buffer_ptr);
  Image::BufferScratch<value_type>::voxel_type template_voxel (*template_buffer_ptr);

  opt = get_options ("transformed");
  Ptr<Image::Buffer<value_type> > transformed_buffer_ptr;
  if (opt.size())
    transformed_buffer_ptr = new Image::Buffer<value_type> (opt[0][0], template_header);

  opt = get_options ("registration");
  bool do_rigid  = false;
  bool do_affine = false;
  bool do_syn = false;
  int registration_type = 5;
  if (opt.size())
    registration_type = opt[0][0];
  switch (registration_type) {
    case 0:
      do_rigid = true;
      break;
    case 1:
      do_affine = true;
      break;
    case 2:
      do_syn = true;
      break;
    case 3:
      do_rigid = true;
      do_affine = true;
      break;
    case 4:
      do_rigid = true;
      do_syn = true;
      break;
    case 5:
      do_affine = true;
      do_syn = true;
      break;
    case 6:
      do_rigid = true;
      do_affine = true;
      do_syn = true;
      break;
    default:
      break;
  }


  opt = get_options ("rigid_out");
  bool output_rigid = false;
  std::string rigid_filename;
  if (opt.size()) {
    if (!do_rigid)
      throw Exception ("rigid transformation output requested when no rigid registration is requested");
    output_rigid = true;
    rigid_filename = std::string (opt[0][0]);
  }

  opt = get_options ("affine_out");
  bool output_affine = false;
  std::string affine_filename;
  if (opt.size()) {
   if (!do_affine)
     throw Exception ("affine transformation output requested when no affine registration is requested");
   output_affine = true;
   affine_filename = std::string (opt[0][0]);
  }

  opt = get_options ("warp_out");
  Ptr<Image::Buffer<value_type> > warp_buffer;
  if (opt.size()) {
    if (!do_syn)
      throw Exception ("SyN warp output requested when no SyN registration is requested");
    Image::Header warp_header (template_header);
    warp_header.set_ndim (5);
    warp_header.dim(3) = 3;
    warp_header.dim(4) = 4;
    warp_header.stride(0) = 2;
    warp_header.stride(1) = 3;
    warp_header.stride(2) = 4;
    warp_header.stride(3) = 1;
    warp_header.stride(4) = 5;
    warp_buffer = new Image::Buffer<value_type> (std::string (opt[0][0]), warp_header);
  }

  opt = get_options ("rigid_scale");
  std::vector<value_type> rigid_scale_factors;
  if (opt.size ()) {
    if (!do_rigid)
      throw Exception ("the rigid multi-resolution scale factors were input when no rigid registration is requested");
    rigid_scale_factors = parse_floats (opt[0][0]);
  }

  opt = get_options ("affine_scale");
  std::vector<value_type> affine_scale_factors;
  if (opt.size ()) {
    if (!do_affine)
      throw Exception ("the affine multi-resolution scale factors were input when no rigid registration is requested");
    affine_scale_factors = parse_floats (opt[0][0]);
  }

  opt = get_options ("syn_scale");
  std::vector<value_type> syn_scale_factors;
  if (opt.size ()) {
    if (!do_syn)
      throw Exception ("the syn multi-resolution scale factors were input when no rigid registration is requested");
    syn_scale_factors = parse_floats (opt[0][0]);
  }

  opt = get_options ("tmask");
  Ptr<Image::BufferPreload<bool> > tmask_image;
  if (opt.size ())
    tmask_image = new Image::BufferPreload<bool> (opt[0][0]);

  opt = get_options ("mmask");
  Ptr<Image::BufferPreload<bool> > mmask_image;
  if (opt.size ())
    mmask_image = new Image::BufferPreload<bool> (opt[0][0]);

  opt = get_options ("rigid_niter");
  std::vector<int> rigid_niter;;
  if (opt.size ()) {
    rigid_niter = parse_ints (opt[0][0]);
    if (!do_rigid)
      throw Exception ("the number of rigid iterations have been input when no rigid registration is requested");
  }

  opt = get_options ("affine_niter");
  std::vector<int> affine_niter;
  if (opt.size ()) {
    affine_niter = parse_ints (opt[0][0]);
    if (!do_affine)
      throw Exception ("the number of affine iterations have been input when no affine registration is requested");
  }

  opt = get_options ("syn_niter");
  std::vector<int> syn_niter;
  if (opt.size ()) {
    if (!do_syn)
      throw Exception ("the number of syn iterations have been input when no SyN registration is requested");
    syn_niter = parse_ints (opt[0][0]);
  }

  opt = get_options ("smooth_update");
  value_type smooth_update = NAN;
  if (opt.size()) {
    smooth_update = opt[0][0];
    if (!do_syn)
      throw Exception ("the warp update field smoothing parameter was input with no SyN registration is requested");
  }

  opt = get_options ("smooth_warp");
  value_type smooth_warp = NAN;
  if (opt.size()) {
    smooth_warp = opt[0][0];
    if (!do_syn)
      throw Exception ("the warp field smoothing parameter was input with no SyN registration is requested");
  }

  Image::Registration::Transform::Rigid<double> rigid;
  opt = get_options ("rigid_init");
  bool init_rigid_set = false;
  if (opt.size()) {
    throw Exception ("initialise with rigid not yet implemented");
    init_rigid_set = true;
    Math::Matrix<value_type> init_rigid;
    init_rigid.load (opt[0][0]);
    //TODO // set rigid
  }

  Image::Registration::Transform::Affine<double> affine;
  opt = get_options ("affine_init");
  bool init_affine_set = false;
  if (opt.size()) {
    throw Exception ("initialise with affine not yet implemented");
    if (init_rigid_set)
      throw Exception ("you cannot initialise registrations with both a rigid and affine transformation");
    if (do_rigid)
      throw Exception ("you cannot initialise a rigid registration with an affine transformation");
    init_affine_set = true;
    Math::Matrix<value_type> init_affine;
    init_affine.load (opt[0][0]);
    //TODO // set affine
  }

  opt = get_options ("syn_init");
  Ptr<Image::Buffer<value_type> > init_warp_buffer;
  if (opt.size()) {
    if (init_rigid_set || init_affine_set)
      throw Exception ("you cannot initialise registrations with both a warp and a linear transformation "
                       "(the linear transformation will already be included in the warp)");
    throw Exception ("initialise with affine not yet implemented");
    init_warp_buffer = new Image::Buffer<value_type> (opt[0][0]);
  }

  opt = get_options ("centre");
  Image::Registration::Transform::Init::InitType init_centre = Image::Registration::Transform::Init::mass;
  if (opt.size()) {
    switch ((int)opt[0][0]){
      case 0:
        init_centre = Image::Registration::Transform::Init::mass;
        break;
      case 1:
        init_centre = Image::Registration::Transform::Init::geometric;
        break;
      case 2:
        init_centre = Image::Registration::Transform::Init::none;
        break;
      default:
        break;
    }
  }


  opt = get_options ("directions");
  Math::Matrix<value_type> directions;
  DWI::Directions::electrostatic_repulsion_60 (directions);
  if (opt.size()) {
    if (!do_reorientation)
      throw Exception ("apodised PSF directions specified when no reorientation is to be performed");
    directions.load(opt[0][0]);
  }

  if (do_rigid) {

    CONSOLE ("running rigid registration");
    Image::Registration::Linear rigid_registration;
    Image::Registration::Metric::MeanSquared metric;

    if (rigid_scale_factors.size())
      rigid_registration.set_scale_factor (rigid_scale_factors);
    if (rigid_niter.size())
      rigid_registration.set_max_iter (rigid_niter);
    if (init_rigid_set)
      rigid_registration.set_init_type (Image::Registration::Transform::Init::none);
    else
      rigid_registration.set_init_type (init_centre);

    rigid_registration.run_masked (metric, rigid, moving_voxel, template_voxel, mmask_image, tmask_image);

    if (output_rigid)
      rigid.get_transform().save (rigid_filename);
  }

  if (do_affine) {

    CONSOLE ("running affine registration");
    Image::Registration::Linear affine_registration;
    Image::Registration::Metric::MeanSquared metric;

    if (affine_scale_factors.size())
      affine_registration.set_scale_factor (affine_scale_factors);
    if (affine_niter.size())
      affine_registration.set_max_iter (affine_niter);
    if (do_rigid) {
      affine.set_centre (rigid.get_centre());
      affine.set_translation (rigid.get_translation());
      affine.set_matrix (rigid.get_matrix());
    }
    if (do_rigid || init_affine_set)
      affine_registration.set_init_type (Image::Registration::Transform::Init::none);
    else
      affine_registration.set_init_type (init_centre);

    affine_registration.run_masked (metric, affine, moving_voxel, template_voxel, mmask_image, tmask_image);
    if (output_affine)
      affine.get_transform().save (affine_filename);

  }

  if (do_syn) {

    CONSOLE ("running SyN registration");

    if (smooth_warp) {

    }

    if (smooth_update) {

    }

    if (warp_buffer) {
      //write out warp
    }

  }

  if (transformed_buffer_ptr) {
    Image::Buffer<float>::voxel_type transformed_voxel (*transformed_buffer_ptr);

    if (do_syn) {

//      if (do_reorientation)
//        Image::Registration::fod_reorient (transformed_buffer, affine.get_transform());
    } else if (do_affine) {
      Image::Filter::reslice<Image::Interp::Cubic> (moving_voxel, transformed_voxel, affine.get_transform(), Image::Adapter::AutoOverSample, 0.0);
//      if (do_reorientation)
//        Image::Filter::fod_reorient (transformed_buffer, affine.get_transform());
    } else {
      Image::Filter::reslice<Image::Interp::Cubic> (moving_voxel, transformed_voxel, rigid.get_transform(), Image::Adapter::AutoOverSample, 0.0);
//      if (do_reorientation)
//        Image::Filter::fod_reorient (transformed_buffer, affine.get_transform());
    }
  }
}
