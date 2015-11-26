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
// #define REGISTRATION_GRADIENT_DESCENT_DEBUG

#include "command.h"
#include "image.h"
#include "filter/reslice.h"
#include "interp/cubic.h"
#include "transform.h"
#include "registration/linear.h"
#include "registration/syn.h"
#include "registration/metric/mean_squared.h"
#include "registration/metric/cross_correlation.h"
#include "registration/metric/mean_squared_4D.h"
#include "registration/transform/affine.h"
#include "registration/transform/rigid.h"
#include "dwi/directions/predefined.h"
#include "math/SH.h"
#include "image/average_space.h"


using namespace MR;
using namespace App;

const char* transformation_choices[] = { "rigid", "affine", "syn", "rigid_affine", "rigid_syn", "affine_syn", "rigid_affine_syn", NULL };


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

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
    + Argument ("image1", "input image 1 ('moving')").type_image_in ()
    + Argument ("image2", "input image 2 ('template')").type_image_in ();


  OPTIONS
  + Option ("type", "the registration type. Valid choices are: "
                             "rigid, affine, syn, rigid_affine, rigid_syn, affine_syn, rigid_affine_syn (Default: affine_syn)")
    + Argument ("choice").type_choice (transformation_choices)

  + Option ("transformed", "image1 after registration transformed to the space of image2")
    + Argument ("image").type_image_out ()

  + Option ("transformed_midway", "image1 and image2 after registration transformed "
        "to the midway space")
    + Argument ("image1_transformed").type_image_out ()
    + Argument ("image2_transformed").type_image_out ()

  + Option ("mask1", "a mask to define the region of image1 to use for optimisation.")
    + Argument ("filename").type_image_in ()

  + Option ("mask2", "a mask to define the region of image2 to use for optimisation.")
    + Argument ("filename").type_image_in ()

  + Registration::rigid_options

  + Registration::affine_options

  + Registration::syn_options

  + Registration::initialisation_options

  + Registration::fod_options

  + DataType::options();
}

typedef float value_type;

void load_image (std::string filename, size_t num_vols, Image<value_type>& image) {
  auto temp_image = Image<value_type>::open (filename);
  auto header = Header::open (filename);
  header.datatype() = DataType::from_command_line (DataType::Float32);
  if (num_vols > 1) {
    header.size(3) = num_vols;
    header.stride(0) = 2;
    header.stride(1) = 3;
    header.stride(2) = 4;
    header.stride(3) = 1;
  }
  image = Image<value_type>::scratch (header);
  if (num_vols > 1) {
    for (auto i = Loop ()(image); i; ++i) {
      assign_pos_of (image).to (temp_image);
        image.value() = temp_image.value();
    }
  } else {
    threaded_copy (temp_image, image);
  }
}


void run ()
{
  #ifdef REGISTRATION_GRADIENT_DESCENT_DEBUG
    std::remove("/tmp/gddebug/log.txt");
  #endif
  const auto im1_header = Header::open (argument[0]);
  const auto im2_header = Header::open (argument[1]);

  if (im1_header.ndim() != im2_header.ndim())
    throw Exception ("input images to not have the same number of dimensions");

  Image<value_type> im1_image;
  Image<value_type> im2_image;

  auto opt = get_options ("noreorientation");
  bool do_reorientation = true;
  if (opt.size())
    do_reorientation = false;

  if (im2_header.ndim() > 4) {
    throw Exception ("image dimensions larger than 4 are not supported");
  }
  else if (im2_header.ndim() == 4) {
    if (im1_header.size(3) != im2_header.size(3))
      throw Exception ("input images do not have the same number of volumes in the 4th dimension");

    value_type val = (std::sqrt (float (1 + 8 * im2_header.size(3))) - 3.0) / 4.0;
    if (!(val - (int)val) && do_reorientation && im2_header.size(3) > 1) {
        CONSOLE ("SH series detected, performing FOD registration");
        // Only load as many SH coefficients as required
        int lmax = 4;
        if (Math::SH::LforN(im2_header.size(3)) < 4)
          lmax = Math::SH::LforN(im2_header.size(3));
        auto opt = get_options ("lmax");
        if (opt.size()) {
          lmax = opt[0][0];
          if (lmax % 2)
            throw Exception ("the input lmax must be even");
        }
        int num_SH = Math::SH::NforL (lmax);
        if (num_SH > im2_header.size(3))
            throw Exception ("not enough SH coefficients within input image for desired lmax");
        load_image (argument[0], num_SH, im1_image);
        load_image (argument[1], num_SH, im2_image);
    }
    else {
      do_reorientation = false;
      load_image (argument[0], im1_header.size(3), im1_image);
      load_image (argument[1], im2_header.size(3), im2_image);
    }
  }
  else {
    do_reorientation = false;
    load_image (argument[0], 1, im1_image);
    load_image (argument[1], 1, im2_image);
  }

  // Will currently output whatever lmax was used during registration
  opt = get_options ("transformed");
  Image<value_type> im1_transformed;
  if (opt.size()){
    im1_transformed = Image<value_type>::create (opt[0][0], im2_image);
    im1_transformed.original_header().datatype() = DataType::from_command_line (DataType::Float32);
  }
  opt = get_options ("transformed_midway");
  Image<value_type> image1_midway;
  Image<value_type> image2_midway;
  if (opt.size()){
    std::vector<Header> headers;
    headers.push_back(im1_image.original_header());
    headers.push_back(im2_image.original_header());
    std::vector<Eigen::Transform<double, 3, Eigen::AffineCompact>> void_trafo;
    auto padding = Eigen::Matrix<double, 4, 1>(1.0, 1.0, 1.0, 1.0);
    value_type resolution = 1.0;
    auto mid_way_image = compute_minimum_average_header<double,Eigen::Transform<double, 3, Eigen::AffineCompact>>(headers, resolution, padding, void_trafo);
    image1_midway = Image<value_type>::create (opt[0][0], mid_way_image);
    image1_midway.original_header().datatype() = DataType::from_command_line (DataType::Float32);
    image2_midway = Image<value_type>::create (opt[0][1], mid_way_image);
    image2_midway.original_header().datatype() = DataType::from_command_line (DataType::Float32);
  }

  opt = get_options ("type");
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


  opt = get_options ("rigid");
  bool output_rigid = false;
  std::string rigid_filename;
  if (opt.size()) {
    if (!do_rigid)
      throw Exception ("rigid transformation output requested when no rigid registration is requested");
    output_rigid = true;
    rigid_filename = std::string (opt[0][0]);
  }

  opt = get_options ("affine");
  bool output_affine = false;
  std::string affine_filename;
  if (opt.size()) {
   if (!do_affine)
     throw Exception ("affine transformation output requested when no affine registration is requested");
   output_affine = true;
   affine_filename = std::string (opt[0][0]);
  }

  opt = get_options ("affine_1tomidway");
  bool output_affine_1tomid = false;
  std::string affine_1tomid_filename;
  if (opt.size()) {
   if (!do_affine)
     throw Exception ("midway affine transformation output requested when no affine registration is requested");
   output_affine_1tomid = true;
   affine_1tomid_filename = std::string (opt[0][0]);
  }

  opt = get_options ("affine_2tomidway");
  bool output_affine_2tomid = false;
  std::string affine_2tomid_filename;
  if (opt.size()) {
   if (!do_affine)
     throw Exception ("midway affine transformation output requested when no affine registration is requested");
   output_affine_2tomid = true;
   affine_2tomid_filename = std::string (opt[0][0]);
  }

  opt = get_options ("warp_out");
  std::unique_ptr<Image<value_type> > warp_buffer;
  if (opt.size()) {
    if (!do_syn)
      throw Exception ("SyN warp output requested when no SyN registration is requested");
    Header warp_header (im2_header);
    warp_header.set_ndim (5); //TODO decide on format
    warp_header.size(3) = 3;
    warp_header.size(4) = 4;
    warp_header.stride(0) = 2;
    warp_header.stride(1) = 3;
    warp_header.stride(2) = 4;
    warp_header.stride(3) = 1;
    warp_header.stride(4) = 5;
    warp_buffer.reset (new Image<value_type> (Image<value_type>::create(std::string (opt[0][0]), warp_header)));
  }

  opt = get_options ("rigid_scale");
  std::vector<default_type> rigid_scale_factors;
  if (opt.size ()) {
    if (!do_rigid)
      throw Exception ("the rigid multi-resolution scale factors were input when no rigid registration is requested");
    rigid_scale_factors = parse_floats (opt[0][0]);
  }

  opt = get_options ("affine_scale");
  std::vector<default_type> affine_scale_factors;
  if (opt.size ()) {
    if (!do_affine)
      throw Exception ("the affine multi-resolution scale factors were input when no affine registration is requested");
    affine_scale_factors = parse_floats (opt[0][0]);
  }

  opt = get_options ("affine_repetitions");
  std::vector<int> affine_repetition_factors;
  if (opt.size ()) {
    if (!do_affine)
      throw Exception ("the affine repetition factors were input when no affine registration is requested");
    affine_repetition_factors = parse_ints (opt[0][0]);
  }


  opt = get_options ("rigid_smooth_factor");
  std::vector<default_type> rigid_smooth_factor(1,1.0);
  if (opt.size ()) {
    if (!do_rigid)
      throw Exception ("the rigid smooth factor was input when no rigid registration is requested");
    rigid_smooth_factor = parse_floats (opt[0][0]);
  }

  opt = get_options ("affine_smooth_factor");
  std::vector<default_type> affine_smooth_factor(1,1.0);
  if (opt.size ()) {
    if (!do_affine)
      throw Exception ("the affine smooth factor was input when no affine registration is requested");
    affine_smooth_factor = parse_floats (opt[0][0]);
  }

  opt = get_options ("affine_loop_density");
  std::vector<default_type> affine_loop_density;
  if (opt.size ()) {
    if (!do_affine)
      throw Exception ("the affine sparsity factor was input when no affine registration is requested");
    affine_loop_density = parse_floats (opt[0][0]);
  }

  bool rigid_cc = get_options ("rigid_cc").size() == 1;
  bool affine_cc = get_options ("affine_cc").size() == 1;
  bool affine_robust_estimator = get_options ("affine_robust").size() == 1;

  opt = get_options ("syn_scale");
  std::vector<default_type> syn_scale_factors;
  if (opt.size ()) {
    if (!do_syn)
      throw Exception ("the syn multi-resolution scale factors were input when no syn registration is requested");
    syn_scale_factors = parse_floats (opt[0][0]);
  }


  opt = get_options ("mask2");
  Image<bool> mask2_image;
  if (opt.size ())
    mask2_image = Image<bool>::open(opt[0][0]);

  opt = get_options ("mask1");
  Image<bool> mask1_image;
  if (opt.size ())
    mask1_image = Image<bool>::open(opt[0][0]);

  opt = get_options ("rigid_niter");
  std::vector<int> rigid_niter;
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

  Registration::Transform::Rigid rigid;
  opt = get_options ("rigid_init");
  bool init_rigid_set = false;
  if (opt.size()) {
    throw Exception ("initialise with rigid not yet implemented");
    init_rigid_set = true;
    Eigen::Transform<double, 3, Eigen::AffineCompact> init_rigid = load_transform (opt[0][0]);
    //TODO // set initial rigid....need to rejig wrt centre. Would be easier to save Versor coefficients and centre of rotation
    CONSOLE(str(init_rigid.matrix()));
  }

  Registration::Transform::Affine affine;
  opt = get_options ("affine_init");
  bool init_affine_set = false;
  if (opt.size()) {
    if(do_syn)
      throw Exception ("initialise with affine not yet implemented");
    if (init_rigid_set)
      throw Exception ("you cannot initialise registrations with both a rigid and affine transformation");
    if (do_rigid)
      throw Exception ("you cannot initialise a rigid registration with an affine transformation");
    init_affine_set = true;
    Eigen::Transform<default_type, 3, Eigen::AffineCompact> init_affine = load_transform(opt[0][0]);
    //TODO // set affine....need to rejig wrt centre
    // CONSOLE(str(init_affine.matrix()));
    affine.set_transform(init_affine);
  }

  opt = get_options ("syn_init");
  Image<value_type> init_warp_buffer;
  if (opt.size()) {
    if (init_rigid_set || init_affine_set)
      throw Exception ("you cannot initialise registrations with both a warp and a linear transformation "
                       "(the linear transformation will already be included in the warp)");
    throw Exception ("initialise with affine not yet implemented");
    init_warp_buffer = Image<value_type>::open (opt[0][0]);
  }

  opt = get_options ("centre");
  Registration::Transform::Init::InitType init_centre = Registration::Transform::Init::mass;
  if (opt.size()) {
    switch ((int)opt[0][0]){
      case 0:
        init_centre = Registration::Transform::Init::mass;
        break;
      case 1:
        init_centre = Registration::Transform::Init::geometric;
        break;
      case 2:
        init_centre = Registration::Transform::Init::moments;
        break;
      case 3:
        init_centre = Registration::Transform::Init::linear_from_file;
        break;
      case 4:
        init_centre = Registration::Transform::Init::none;
        break;
      default:
        break;
    }
  }
  if (init_rigid_set and
      (init_centre != Registration::Transform::Init::linear_from_file and
        init_centre != Registration::Transform::Init::none))
    WARN("rigid_init option will be overwritten by centre option. Use -centre linear to use transformation file.");
  if (init_affine_set and
    (init_centre != Registration::Transform::Init::linear_from_file and
        init_centre != Registration::Transform::Init::none))
    WARN("affine_init option will be overwritten by centre option. Use -centre linear to use transformation file.");

  Eigen::MatrixXd directions_az_el;
  opt = get_options ("directions");
  if (opt.size())
    directions_az_el = load_matrix (opt[0][0]);
  else
    directions_az_el = DWI::Directions::electrostatic_repulsion_60();
  Eigen::MatrixXd directions_cartesian = Math::SH::spherical2cartesian (directions_az_el);


  if (do_rigid) {
    CONSOLE ("running rigid registration");
    Registration::Linear rigid_registration;

    if (rigid_scale_factors.size())
    rigid_registration.set_scale_factor (rigid_scale_factors);
    rigid_registration.set_smoothing_factor (rigid_smooth_factor);
    if (rigid_niter.size())
      rigid_registration.set_max_iter (rigid_niter);
    if (init_rigid_set)
      rigid_registration.set_init_type (Registration::Transform::Init::none);
    else
      rigid_registration.set_init_type (init_centre);


    if (im2_image.ndim() == 4) {
      if (rigid_cc)
        throw Exception ("rigid cross correlation not implemted for data with more than 3 dimensions");
      Registration::Metric::MeanSquared4D metric;
      rigid_registration.run_masked (metric, rigid, im1_image, im2_image, mask1_image, mask2_image);
    } else {
      if (rigid_cc){
        std::vector<size_t> extent(3,3);
        rigid_registration.set_extent(extent);
        Registration::Metric::CrossCorrelation metric;
        rigid_registration.run_masked (metric, rigid, im1_image, im2_image, mask1_image, mask2_image);
      }
      else {
        Registration::Metric::MeanSquared metric;
        rigid_registration.run_masked (metric, rigid, im1_image, im2_image, mask1_image, mask2_image);
      }
    }

    if (output_rigid)
      save_transform (rigid.get_transform(), rigid_filename);
  }

  if (do_affine) {
    CONSOLE ("running affine registration");
    Registration::Linear affine_registration;

    if (affine_scale_factors.size())
      affine_registration.set_scale_factor (affine_scale_factors);
    if (affine_repetition_factors.size())
      affine_registration.set_gradient_descent_repetitions(affine_repetition_factors);
    affine_registration.set_smoothing_factor (affine_smooth_factor);
    if (affine_niter.size())
      affine_registration.set_max_iter (affine_niter);
    if (affine_loop_density.size())
      affine_registration.set_loop_density (affine_loop_density);
    if (affine_robust_estimator)
      affine_registration.use_robust_estimate (true);
    if (do_rigid) {
      affine.set_centre (rigid.get_centre());
      affine.set_translation (rigid.get_translation());
      affine.set_matrix (rigid.get_matrix());
    }
    if (do_rigid || init_affine_set)
      affine_registration.set_init_type (Registration::Transform::Init::none);
    else
      affine_registration.set_init_type (init_centre);

    if (do_reorientation)
      affine_registration.set_directions (directions_cartesian);


    if (im2_image.ndim() == 4) {
      if (affine_cc)
        throw Exception ("affine cross correlation not implemented for data with more than 3 dimensions");
      Registration::Metric::MeanSquared4D metric;
      affine_registration.run_masked (metric, affine, im1_image, im2_image, mask1_image, mask2_image);
    } else {
      if (affine_cc){
        Registration::Metric::CrossCorrelation metric;
        std::vector<size_t> extent(3,3);
        affine_registration.set_extent(extent);
        affine_registration.run_masked (metric, affine, im1_image, im2_image, mask1_image, mask2_image);
      }
      else {
        Registration::Metric::MeanSquared metric;
        affine_registration.run_masked (metric, affine, im1_image, im2_image, mask1_image, mask2_image);
      }
    }


    if (output_affine)
      save_transform (affine.get_transform(), affine_filename);

    if (output_affine_1tomid)
      save_transform (affine.get_transform_half(), affine_1tomid_filename);

    if (output_affine_2tomid)
      save_transform (affine.get_transform_half_inverse(), affine_2tomid_filename);
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

  if (im1_transformed.valid()) {
    if (do_syn) {
    } else if (do_affine) {
      Filter::reslice<Interp::Cubic> (im1_image, im1_transformed, affine.get_transform(), Adapter::AutoOverSample, 0.0);
      if (do_reorientation) {
        std::string msg ("reorienting...");
        Registration::Transform::reorient (msg, im1_transformed, affine.get_transform(), directions_cartesian);
      }
    } else {
      Filter::reslice<Interp::Cubic> (im1_image, im1_transformed, rigid.get_transform(), Adapter::AutoOverSample, 0.0);
      if (do_reorientation) {
        std::string msg ("reorienting...");
        Registration::Transform::reorient (msg, im1_transformed, rigid.get_transform(), directions_cartesian);
      }
    }
  }
  if (image1_midway.valid()) {
    if (do_syn) {
    } else if (do_affine) {
      Filter::reslice<Interp::Cubic> (im1_image, image1_midway, affine.get_transform_half(), Adapter::AutoOverSample, 0.0);
      if (do_reorientation) {
        std::string msg ("reorienting...");
        Registration::Transform::reorient (msg, image1_midway, affine.get_transform_half(), directions_cartesian);
      }
    } else {
      Filter::reslice<Interp::Cubic> (im1_image, image1_midway, rigid.get_transform_half(), Adapter::AutoOverSample, 0.0);
      if (do_reorientation) {
        std::string msg ("reorienting...");
        Registration::Transform::reorient (msg, image1_midway, rigid.get_transform_half(), directions_cartesian);
      }
    }
  }
  if (image2_midway.valid()) {
    if (do_syn) {
    } else if (do_affine) {
      Filter::reslice<Interp::Cubic> (im2_image, image2_midway, affine.get_transform_half_inverse(), Adapter::AutoOverSample, 0.0);
      if (do_reorientation) {
        std::string msg ("reorienting...");
        Registration::Transform::reorient (msg, image2_midway, affine.get_transform_half_inverse(), directions_cartesian);
      }
    } else {
      if (do_reorientation) {
        std::string msg ("reorienting...");
        Registration::Transform::reorient (msg, image2_midway, rigid.get_transform_half_inverse(), directions_cartesian);
      }
    }
  }
}
