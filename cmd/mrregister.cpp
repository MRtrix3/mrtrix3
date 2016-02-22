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
#include "image.h"
#include "filter/reslice.h"
#include "interp/cubic.h"
#include "transform.h"
#include "registration/linear.h"
#include "registration/nonlinear.h"
#include "registration/metric/demons.h"
#include "registration/metric/mean_squared.h"
#include "registration/metric/difference_robust.h"
#include "registration/metric/difference_robust_4D.h"
#include "registration/metric/cross_correlation.h"
#include "registration/metric/mean_squared_4D.h"
#include "registration/transform/affine.h"
#include "registration/transform/rigid.h"
#include "dwi/directions/predefined.h"
#include "math/SH.h"
#include "image/average_space.h"


using namespace MR;
using namespace App;

const char* transformation_choices[] = { "rigid", "affine", "nonlinear", "rigid_affine", "rigid_nonlinear", "affine_nonlinear", "rigid_affine_nonlinear", NULL };


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au) & Max Pietsch (maximilian.pietsch@kcl.ac.uk)";

  DESCRIPTION
      + "Register two images together using a rigid, affine or a non-linear transformation model."

      + "By default this application will perform an affine, followed by non-linear registration."

      + "FOD registration (with apodised point spread reorientation) will be performed by default if the number of volumes "
        "in the 4th dimension equals the number of coefficients in an antipodally symmetric spherical harmonic series (e.g. 6, 15, 28 etc). "
        "The -no_reorientation option can be used to force reorientation off if required."

      + "Non-linear registration computes warps to map from both image1->image2 and image2->image1. "
        "Similar to Avants (2008) Med Image Anal. 12(1): 26–41, both the image1 and image2 are warped towards a 'middle space'. "
        "Warps are saved in a single 5D file, with the 5th dimension defining the warp type. See here for more details (TODO). "
        "By default the affine transformation will be saved in the warp image header (use mrinfo to view). To save the affine transform "
        "separately as a text file, use the -affine option.";

  REFERENCES
  + "* If FOD registration is being performed:\n"
    "Raffelt, D.; Tournier, J.-D.; Fripp, J; Crozier, S.; Connelly, A. & Salvado, O. " // Internal
    "Symmetric diffeomorphic registration of fibre orientation distributions. "
    "NeuroImage, 2011, 56(3), 1171-1180"

  + "Raffelt, D.; Tournier, J.-D.; Crozier, S.; Connelly, A. & Salvado, O. " // Internal
    "Reorientation of fiber orientation distributions using apodized point spread functions. "
    "Magnetic Resonance in Medicine, 2012, 67, 844-855";


  ARGUMENTS
    + Argument ("image1", "input image 1 ('moving')").type_image_in ()
    + Argument ("image2", "input image 2 ('template')").type_image_in ();

  OPTIONS
  + Option ("type", "the registration type. Valid choices are: "
                     "rigid, affine, nonlinear, rigid_affine, rigid_nonlinear, affine_nonlinear, rigid_affine_nonlinear (Default: affine_nonlinear)")
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

  + Registration::nonlinear_options

  + Registration::fod_options

  + DataType::options();
}

typedef double value_type;



void run ()
{
  #ifdef REGISTRATION_GRADIENT_DESCENT_DEBUG
    std::remove("/tmp/gddebug/log.txt");
  #endif

  Image<value_type> im1_image = Image<value_type>::open (argument[0]).with_direct_io (Stride::contiguous_along_axis (3));
  Image<value_type> im2_image = Image<value_type>::open (argument[1]).with_direct_io (Stride::contiguous_along_axis (3));

  if (im1_image.ndim() != im2_image.ndim())
    throw Exception ("input images do not have the same number of dimensions");

  auto opt = get_options ("type");
  bool do_rigid  = false;
  bool do_affine = false;
  bool do_nonlinear = false;
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
      do_nonlinear = true;
      break;
    case 3:
      do_rigid = true;
      do_affine = true;
      break;
    case 4:
      do_rigid = true;
      do_nonlinear = true;
      break;
    case 5:
      do_affine = true;
      do_nonlinear = true;
      break;
    case 6:
      do_rigid = true;
      do_affine = true;
      do_nonlinear = true;
      break;
    default:
      break;
  }

  bool do_reorientation = true;
  opt = get_options ("noreorientation");
  if (opt.size())
    do_reorientation = false;


  Eigen::MatrixXd directions_cartesian;
  opt = get_options ("directions");
  if (opt.size())
    directions_cartesian = Math::SH::spherical2cartesian (load_matrix (opt[0][0])).transpose();

  int image_lmax = 0;

  if (im1_image.ndim() > 4) {
    throw Exception ("image dimensions larger than 4 are not supported");
  } else if (im1_image.ndim() == 3) {
    do_reorientation = false;
  } else if (im1_image.ndim() == 4) {
    if (im1_image.size(3) != im2_image.size(3))
      throw Exception ("input images do not have the same number of volumes in the 4th dimension");
    image_lmax = Math::SH::LforN (im1_image.size(3));
    value_type val = (std::sqrt (float (1 + 8 * im2_image.size(3))) - 3.0) / 4.0;
    if (!(val - (int)val) && do_reorientation && im2_image.size(3) > 1) {
      CONSOLE ("SH series detected, performing FOD registration");
      do_reorientation = true;
      if (!directions_cartesian.cols())
        directions_cartesian = Math::SH::spherical2cartesian (DWI::Directions::electrostatic_repulsion_60()).transpose();
    } else {
      do_reorientation = false;
      if (directions_cartesian.cols())
        WARN ("-directions option ignored since no FOD reorientation is being performed");
    }
  }


  opt = get_options ("transformed");
  Image<value_type> im1_transformed;
  if (opt.size()){
    Header transformed_header (im2_image);
    transformed_header.datatype() = DataType::from_command_line (DataType::Float32);
    im1_transformed = Image<value_type>::create (opt[0][0], transformed_header);
  }

  std::string im1_midway_transformed_path;
  std::string im2_midway_transformed_path;
  opt = get_options ("transformed_midway");
  if (opt.size()){
    im1_midway_transformed_path = str(opt[0][0]);
    im2_midway_transformed_path = str(opt[0][1]);
  }

  opt = get_options ("mask2");
  Image<value_type> im2_mask;
  if (opt.size ())
    im2_mask = Image<value_type>::open(opt[0][0]);

  opt = get_options ("mask1");
  Image<value_type> im1_mask;
  if (opt.size ())
    im1_mask = Image<value_type>::open(opt[0][0]);



  // ****** RIGID REGISTRATION OPTIONS *******
  Registration::Linear rigid_registration;
  opt = get_options ("rigid");
  bool output_rigid = false;
  std::string rigid_filename;
  if (opt.size()) {
    if (!do_rigid)
      throw Exception ("rigid transformation output requested when no rigid registration is requested");
    output_rigid = true;
    rigid_filename = std::string (opt[0][0]);
  }

  Registration::Transform::Rigid rigid;
  opt = get_options ("rigid_init");
  bool init_rigid_set = false;
  if (opt.size()) {
    init_rigid_set = true;
    transform_type rigid_transform = load_transform (opt[0][0]);
    rigid.set_transform (rigid_transform);
    rigid_registration.set_init_type (Registration::Transform::Init::none);
  }

  opt = get_options ("rigid_centre");
  if (opt.size()) {
    if (init_rigid_set)
      throw Exception ("options -rigid_init and -rigid_centre are mutually exclusive");
    switch ((int)opt[0][0]){
      case 0:
        rigid_registration.set_init_type (Registration::Transform::Init::mass);
        break;
      case 1:
        rigid_registration.set_init_type (Registration::Transform::Init::geometric);
        break;
      case 2:
        rigid_registration.set_init_type (Registration::Transform::Init::moments);
        break;
      case 3:
        rigid_registration.set_init_type (Registration::Transform::Init::mass_unmasked);
        break;
      case 4:
        rigid_registration.set_init_type (Registration::Transform::Init::moments_unmasked);
        break;
      case 5:
        rigid_registration.set_init_type (Registration::Transform::Init::none);
        break;
      default:
        break;
    }
  }

  opt = get_options ("rigid_scale");
  if (opt.size ()) {
    if (!do_rigid)
      throw Exception ("the rigid multi-resolution scale factors were input when no rigid registration is requested");
    rigid_registration.set_scale_factor (parse_floats (opt[0][0]));
  }

  opt = get_options ("rigid_niter");
  if (opt.size ()) {
    if (!do_rigid)
      throw Exception ("the number of rigid iterations have been input when no rigid registration is requested");
    rigid_registration.set_max_iter (parse_ints (opt[0][0]));
  }

  opt = get_options ("rigid_metric");
  Registration::LinearMetricType rigid_metric = Registration::Diff;
  if (opt.size()) {
    switch ((int)opt[0][0]){
      case 0:
        rigid_metric = Registration::Diff;
        break;
      case 1:
        rigid_metric = Registration::NCC;
        break;
      default:
        break;
    }
  }

  opt = get_options ("rigid_global_search");
  if (opt.size())
      rigid_registration.use_global_search(true);

  opt = get_options ("rigid_lmax");
  std::vector<int> rigid_lmax;
  if (opt.size ()) {
    if (!do_rigid)
      throw Exception ("the -rigid_lmax option has been set when no rigid registration is requested");
    if (im1_image.ndim() < 4)
      throw Exception ("-rigid_lmax option is not valid with 3D images");
    rigid_lmax = parse_ints (opt[0][0]);
    rigid_registration.set_lmax (rigid_lmax);
    for (size_t i = 0; i < rigid_lmax.size (); ++i)
       if (rigid_lmax[i] > image_lmax)
          throw Exception ("the requested -rigid_lmax exceeds the lmax of the input images");
  }


  // ****** AFFINE REGISTRATION OPTIONS *******
  Registration::Linear affine_registration;
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

  Registration::Transform::Affine affine;
  opt = get_options ("affine_init");
  bool init_affine_set = false;
  if (opt.size()) {
    if (init_rigid_set)
      throw Exception ("you cannot initialise registrations with both a rigid and affine transformation");
    if (do_rigid)
      throw Exception ("you cannot initialise with -affine_init since a rigid registration is being performed");

    init_affine_set = true;
    transform_type init_affine = load_transform (opt[0][0]);
    affine.set_transform (init_affine);
    affine_registration.set_init_type (Registration::Transform::Init::none);
  }

  opt = get_options ("affine_centre");
  if (opt.size()) {
    if (init_affine_set)
      throw Exception ("options -affine_init and -affine_centre are mutually exclusive");
    switch ((int)opt[0][0]){
      case 0:
        affine_registration.set_init_type (Registration::Transform::Init::mass);
        break;
      case 1:
        affine_registration.set_init_type (Registration::Transform::Init::geometric);
        break;
      case 2:
        affine_registration.set_init_type (Registration::Transform::Init::moments);
        break;
      case 3:
        affine_registration.set_init_type (Registration::Transform::Init::mass_unmasked);
        break;
      case 4:
        affine_registration.set_init_type (Registration::Transform::Init::moments_unmasked);
        break;
      case 5:
        affine_registration.set_init_type (Registration::Transform::Init::none);
        break;
      default:
        break;
    }
  }

  opt = get_options ("affine_scale");
  if (opt.size ()) {
    if (!do_affine)
      throw Exception ("the affine multi-resolution scale factors were input when no affine registration is requested");
    affine_registration.set_scale_factor (parse_floats (opt[0][0]));
  }

  opt = get_options ("affine_repetitions");
  if (opt.size ()) {
    if (!do_affine)
      throw Exception ("the affine repetition factors were input when no affine registration is requested");
    affine_registration.set_gradient_descent_repetitions (parse_ints (opt[0][0]));
  }

  opt = get_options ("affine_loop_density");
  if (opt.size ()) {
    if (!do_affine)
      throw Exception ("the affine sparsity factor was input when no affine registration is requested");
    affine_registration.set_loop_density (parse_floats (opt[0][0]));
  }

  opt = get_options ("affine_metric");
  Registration::LinearMetricType affine_metric = Registration::Diff;
  if (opt.size()) {
    switch ((int)opt[0][0]){
      case 0:
        affine_metric = Registration::Diff;
        break;
      case 1:
        affine_metric = Registration::NCC;
        break;
      default:
        break;
    }
  }

  opt = get_options ("affine_robust_estimator");
  Registration::LinearRobustMetricEstimatorType affine_estimator = Registration::None;
  if (opt.size()) {
    switch ((int)opt[0][0]){
      case 0:
        affine_estimator = Registration::L1;
        break;
      case 1:
        affine_estimator = Registration::L2;
        break;
      case 2:
        affine_estimator = Registration::LP;
        break;
      default:
        break;
    }
  }

  affine_registration.use_robust_estimate (get_options ("affine_robust_median").size() == 1);

  opt = get_options ("affine_global_search");
  if (opt.size())
      affine_registration.use_global_search(true);

  opt = get_options ("affine_niter");
  if (opt.size ()) {
    if (!do_affine)
      throw Exception ("the number of affine iterations have been input when no affine registration is requested");
    affine_registration.set_max_iter (parse_ints (opt[0][0]));
  }


  opt = get_options ("affine_lmax");
  std::vector<int> affine_lmax;
  if (opt.size ()) {
    if (!do_affine)
      throw Exception ("the -affine_lmax option has been set when no affine registration is requested");
    if (im1_image.ndim() < 4)
      throw Exception ("-affine_lmax option is not valid with 3D images");
    affine_lmax = parse_ints (opt[0][0]);
    affine_registration.set_lmax (affine_lmax);
    for (size_t i = 0; i < affine_lmax.size (); ++i)
      if (affine_lmax[i] > image_lmax)
        throw Exception ("the requested -affine_lmax exceeds the lmax of the input images");
  }


  // ****** NON-LINEAR REGISTRATION OPTIONS *******
  Registration::NonLinear nonlinear_registration;
  opt = get_options ("nl_warp");
  std::string warp_filename;
  if (opt.size()) {
    if (!do_nonlinear)
      throw Exception ("Non-linear warp output requested when no non-linear registration is requested");
    warp_filename = std::string (opt[0][0]);
  }


  opt = get_options ("nl_init");
  bool nonlinear_init = false;
  if (opt.size()) {
    nonlinear_init = true;

    if (!do_nonlinear)
      throw Exception ("the non linear initialisation input when no non linear registration is requested");

    Image<default_type> input_warps = Image<default_type>::open (opt[0][0]);
    if (input_warps.ndim() != 5)
      throw Exception ("non-linear initialisation input is not 5D. Input must be from previous non-linear output");

    nonlinear_registration.initialise (input_warps);

    if (do_affine) {
      WARN ("no affine registration will be performed when initialising with non-linear non-linear warps");
      do_affine = false;
    }
    if (do_rigid) {
      WARN ("no rigid registration will be performed when initialising with non-linear non-linear warps");
      do_rigid = false;
    }
    if (init_affine_set)
      WARN ("-affine_init has no effect since the non-linear init warp also contains the linear transform in the image header");
    if (init_rigid_set)
      WARN ("-rigid_init has no effect since the non-linear init warp also contains the linear transform in the image header");
  }


  opt = get_options ("nl_scale");
  if (opt.size ()) {
    if (!do_nonlinear)
      throw Exception ("the non-linear multi-resolution scale factors were input when no non-linear registration is requested");
    std::vector<default_type> scale_factors = parse_floats (opt[0][0]);
    if (nonlinear_init && (scale_factors.size() > 1)) {
      WARN ("-nl_scale option ignored since only the full resolution will be performed when initialising with non-linear warp");
    } else {
      nonlinear_registration.set_scale_factor (scale_factors);
    }
  }

  opt = get_options ("nl_niter");
  if (opt.size ()) {
    if (!do_nonlinear)
      throw Exception ("the number of non-linear iterations have been input when no non-linear registration is requested");
    std::vector<int> iterations_per_level = parse_ints (opt[0][0]);
    if (nonlinear_init && iterations_per_level.size() > 1)
      throw Exception ("when initialising the non-linear registration the max number of iterations can only be defined for a single level");
    else
      nonlinear_registration.set_max_iter (iterations_per_level);
  }

  opt = get_options ("nl_update_smooth");
  if (opt.size()) {
    if (!do_nonlinear)
      throw Exception ("the warp update field smoothing parameter was input when no non-linear registration is requested");
    nonlinear_registration.set_update_smoothing (opt[0][0]);
  }

  opt = get_options ("nl_disp_smooth");
  if (opt.size()) {
    if (!do_nonlinear)
      throw Exception ("the displacement field smoothing parameter was input when no non-linear registration is requested");
    nonlinear_registration.set_disp_smoothing (opt[0][0]);
  }

  opt = get_options ("nl_grad_step");
  if (opt.size()) {
    if (!do_nonlinear)
      throw Exception ("the initial gradient step size was input when no non-linear registration is requested");
    nonlinear_registration.set_init_grad_step (opt[0][0]);
  }

  opt = get_options ("nl_lmax");
  std::vector<int> nl_lmax;
  if (opt.size ()) {
    if (!do_nonlinear)
      throw Exception ("the -nl_lmax option has been set when no non-linear registration is requested");
    if (im1_image.ndim() < 4)
      throw Exception ("-nl_lmax option is not valid with 3D images");
    nl_lmax = parse_ints (opt[0][0]);
    nonlinear_registration.set_lmax (nl_lmax);
    for (size_t i = 0; i < (nl_lmax).size (); ++i)
      if ((nl_lmax)[i] > image_lmax)
        throw Exception ("the requested -nl_lmax exceeds the lmax of the input images");
  }



  // ****** RUN RIGID REGISTRATION *******
  if (do_rigid) {
    CONSOLE ("running rigid registration");

    if (im2_image.ndim() == 4) {
      if (rigid_metric == Registration::NCC)
        throw Exception ("cross correlation metric not implemented for data with more than 3 dimensions");
      Registration::Metric::MeanSquared4D<Image<value_type>, Image<value_type>> metric (im1_image, im2_image);
      rigid_registration.run_masked (metric, rigid, im1_image, im2_image, im1_mask, im2_mask);
    } else {
      if (rigid_metric == Registration::NCC) {
        std::vector<size_t> extent(3,3);
        rigid_registration.set_extent(extent);
        Registration::Metric::CrossCorrelation metric;
        rigid_registration.run_masked (metric, rigid, im1_image, im2_image, im1_mask, im2_mask);
      }
      else {
        Registration::Metric::MeanSquared metric;
        rigid_registration.run_masked (metric, rigid, im1_image, im2_image, im1_mask, im2_mask);
      }
    }

    if (output_rigid)
      save_transform (rigid.get_transform(), rigid_filename);
  }

  // ****** RUN AFFINE REGISTRATION *******
  if (do_affine) {
    CONSOLE ("running affine registration");

    if (do_rigid) {
      affine.set_centre (rigid.get_centre());
      affine.set_translation (rigid.get_translation());
      affine.set_matrix (rigid.get_matrix());
      affine_registration.set_init_type (Registration::Transform::Init::none);
    }

    if (do_reorientation)
      affine_registration.set_directions (directions_cartesian);

    if (im2_image.ndim() == 4) {
      if (affine_metric == Registration::NCC)
        throw Exception ("cross correlation metric not implemented for data with more than 3 dimensions");
      else if (affine_metric == Registration::Diff) {
        if (affine_estimator == Registration::None) {
          Registration::Metric::MeanSquared4D<Image<value_type>, Image<value_type>> metric (im1_image, im2_image);
          affine_registration.run_masked (metric, affine, im1_image, im2_image, im1_mask, im2_mask);
        } else if (affine_estimator == Registration::L1) {
          Registration::Metric::L1 estimator;
          Registration::Metric::DifferenceRobust4D<Image<value_type>, Image<value_type>, Registration::Metric::L1> metric (im1_image, im2_image, estimator);
          affine_registration.run_masked (metric, affine, im1_image, im2_image, im1_mask, im2_mask);
        } else if (affine_estimator == Registration::L2) {
          Registration::Metric::L2 estimator;
          Registration::Metric::DifferenceRobust4D<Image<value_type>, Image<value_type>, Registration::Metric::L2> metric (im1_image, im2_image, estimator);
          affine_registration.run_masked (metric, affine, im1_image, im2_image, im1_mask, im2_mask);
        } else if (affine_estimator == Registration::LP) {
          Registration::Metric::LP estimator;
          Registration::Metric::DifferenceRobust4D<Image<value_type>, Image<value_type>, Registration::Metric::LP> metric (im1_image, im2_image, estimator);
          affine_registration.run_masked (metric, affine, im1_image, im2_image, im1_mask, im2_mask);
        } else throw Exception ("FIXME: estimator selection");
      } else throw Exception ("FIXME: metric selection");
    } else { // 3D
      if (affine_metric == Registration::NCC){
        Registration::Metric::CrossCorrelation metric;
        std::vector<size_t> extent(3,3);
        affine_registration.set_extent (extent);
        affine_registration.run_masked (metric, affine, im1_image, im2_image, im1_mask, im2_mask);
      }
      else if (affine_metric == Registration::Diff) {
        if (affine_estimator == Registration::None) {
          Registration::Metric::MeanSquared metric;
          affine_registration.run_masked (metric, affine, im1_image, im2_image, im1_mask, im2_mask);
        } else if (affine_estimator == Registration::L1) {
          Registration::Metric::L1 estimator;
          Registration::Metric::DifferenceRobust<Registration::Metric::L1> metric(estimator);
          affine_registration.run_masked (metric, affine, im1_image, im2_image, im1_mask, im2_mask);
        } else if (affine_estimator == Registration::L2) {
          Registration::Metric::L2 estimator;
          Registration::Metric::DifferenceRobust<Registration::Metric::L2> metric(estimator);
          affine_registration.run_masked (metric, affine, im1_image, im2_image, im1_mask, im2_mask);
        } else if (affine_estimator == Registration::LP) {
          Registration::Metric::LP estimator;
          Registration::Metric::DifferenceRobust<Registration::Metric::LP> metric(estimator);
          affine_registration.run_masked (metric, affine, im1_image, im2_image, im1_mask, im2_mask);
        } else throw Exception ("FIXME: estimator selection");
      } else throw Exception ("FIXME: metric selection");
    }

    if (output_affine)
      save_transform (affine.get_transform(), affine_filename);

    if (output_affine_1tomid)
      save_transform (affine.get_transform_half(), affine_1tomid_filename);

    if (output_affine_2tomid)
      save_transform (affine.get_transform_half_inverse(), affine_2tomid_filename);
  }


  // ****** RUN NON-LINEAR REGISTRATION *******
  if (do_nonlinear) {
    CONSOLE ("running non-linear registration");

    if (do_reorientation)
      nonlinear_registration.set_aPSF_directions (directions_cartesian);

    if (do_affine) {
      nonlinear_registration.run (affine, im1_image, im2_image, im1_mask, im2_mask);
    } else if (do_rigid) {
      nonlinear_registration.run (rigid, im1_image, im2_image, im1_mask, im2_mask);
    } else {
      Registration::Transform::Affine identity_transform;
      nonlinear_registration.run (identity_transform, im1_image, im2_image, im1_mask, im2_mask);
    }

    if (warp_filename.size()) {
      //TODO add affine parameters to comments too?
      Header output_header = nonlinear_registration.get_output_warps_header();
      auto output_warps = Image<float>::create (warp_filename, output_header);
      nonlinear_registration.get_output_warps (output_warps);
    }
  }



  if (im1_transformed.valid()) {
    INFO ("Outputting tranformed input images...");

    if (do_nonlinear) {
      Header deform_header (im1_transformed);
      deform_header.set_ndim(4);
      deform_header.size(3) = 3;
      Image<default_type> deform_field = Image<default_type>::scratch (deform_header);
      Registration::Transform::compose_halfway_transforms (nonlinear_registration.get_im2_linear().inverse(),
                                                           *(nonlinear_registration.get_im2_disp_field_inv()),
                                                           *(nonlinear_registration.get_im1_disp_field()),
                                                           nonlinear_registration.get_im1_linear(),
                                                           deform_field);
      if (im1_image.ndim() == 3) {
        Filter::warp<Interp::Cubic> (im1_image, im1_transformed, deform_field, 0.0);
      } else { // write to scratch buffer first since FOD reorientation requires direct IO
        auto temp_output = Image<default_type>::scratch (im1_transformed);
        Filter::warp<Interp::Cubic> (im1_image, temp_output, deform_field, 0.0);
        if (do_reorientation)
          Registration::Transform::reorient_warp ("reorienting FODs",
                                                  temp_output,
                                                  deform_field,
                                                  Math::SH::spherical2cartesian (DWI::Directions::electrostatic_repulsion_300()).transpose());
        threaded_copy (temp_output, im1_transformed);
      }

    } else if (do_affine) {
      if (im1_image.ndim() == 3) {
        Filter::reslice<Interp::Cubic> (im1_image, im1_transformed, affine.get_transform(), Adapter::AutoOverSample, 0.0);
      } else { // write to scratch buffer first since FOD reorientation requires direct IO
        auto temp_output = Image<default_type>::scratch (im1_transformed);
        Filter::reslice<Interp::Cubic> (im1_image, temp_output, affine.get_transform(), Adapter::AutoOverSample, 0.0);
        if (do_reorientation)
          Registration::Transform::reorient ("reorienting FODs",
                                             temp_output,
                                             temp_output,
                                             affine.get_transform(),
                                             Math::SH::spherical2cartesian (DWI::Directions::electrostatic_repulsion_300()).transpose());
        threaded_copy (temp_output, im1_transformed);
      }
    } else {
      if (im1_image.ndim() == 3) {
        Filter::reslice<Interp::Cubic> (im1_image, im1_transformed, rigid.get_transform(), Adapter::AutoOverSample, 0.0);
      } else { // write to scratch buffer first since FOD reorientation requires direct IO
        auto temp_output = Image<default_type>::scratch (im1_transformed);
        Filter::reslice<Interp::Cubic> (im1_image, im1_transformed, rigid.get_transform(), Adapter::AutoOverSample, 0.0);
        if (do_reorientation)
          Registration::Transform::reorient ("reorienting FODs",
                                             temp_output,
                                             temp_output,
                                             rigid.get_transform(),
                                             Math::SH::spherical2cartesian (DWI::Directions::electrostatic_repulsion_300()).transpose());
        threaded_copy (temp_output, im1_transformed);
      }
    }
  }


  if (!im1_midway_transformed_path.empty() and !im2_midway_transformed_path.empty()) {
    if (do_nonlinear) {
      Image<default_type> im1_deform_field = Image<default_type>::scratch (*(nonlinear_registration.get_im1_disp_field()));
      Registration::Transform::compose_linear_displacement (affine.get_transform_half(), *(nonlinear_registration.get_im1_disp_field()), im1_deform_field);
      Header midway_header (nonlinear_registration.get_midway_header());
      midway_header.datatype() = DataType::from_command_line (DataType::Float32);
      midway_header.set_ndim(im1_image.ndim());
      if (midway_header.ndim() == 4)
        midway_header.size(3) = im1_image.size(3);
      auto im1_midway = Image<default_type>::create (im1_midway_transformed_path, midway_header);

      if (im1_image.ndim() == 3) {
        Filter::warp<Interp::Cubic> (im1_image, im1_midway, im1_deform_field, 0.0);
      } else {
        //TODO warp input lmax. Use more directions for lmax 8
        auto temp_output = Image<default_type>::scratch (midway_header);
        Filter::warp<Interp::Cubic> (im1_image, temp_output, im1_deform_field, 0.0);
        if (do_reorientation)
          Registration::Transform::reorient_warp ("reorienting FODs", temp_output, im1_deform_field, directions_cartesian);
        threaded_copy (temp_output, im1_midway);
      }

      Image<default_type> im2_deform_field = Image<default_type>::scratch (*(nonlinear_registration.get_im2_disp_field()));
      Registration::Transform::compose_linear_displacement (affine.get_transform_half_inverse(), *(nonlinear_registration.get_im2_disp_field()), im2_deform_field);

      auto im2_midway = Image<default_type>::create (im2_midway_transformed_path, midway_header);

      if (im2_image.ndim() == 3) {
        Filter::warp<Interp::Cubic> (im2_image, im2_midway, im2_deform_field, 0.0);
      } else {
        auto temp_output = Image<default_type>::scratch (midway_header);
        Filter::warp<Interp::Cubic> (im2_image, temp_output, im2_deform_field, 0.0);
        if (do_reorientation)
          Registration::Transform::reorient_warp ("reorienting FODs", temp_output, im2_deform_field, directions_cartesian);
        threaded_copy (temp_output, im2_midway);
      }

    } else if (do_affine){
      affine_registration.write_transformed_images (im1_image, im2_image, affine, im1_midway_transformed_path, im2_midway_transformed_path, do_reorientation);
    } else {
      rigid_registration.write_transformed_images (im1_image, im2_image, rigid, im1_midway_transformed_path, im2_midway_transformed_path, do_reorientation);

    }
  }
}
