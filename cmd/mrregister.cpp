/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "command.h"
#include "image.h"
#include "image_helpers.h"
#include "filter/reslice.h"
#include "interp/cubic.h"
#include "transform.h"
#include "registration/multi_contrast.h"
#include "registration/linear.h"
#include "registration/nonlinear.h"
#include "registration/metric/demons.h"
#include "registration/metric/mean_squared.h"
#include "registration/metric/difference_robust.h"
#include "registration/metric/normalised_cross_correlation.h"
#include "registration/transform/affine.h"
#include "registration/transform/rigid.h"
#include "dwi/directions/predefined.h"
#include "math/average_space.h"
#include "math/SH.h"
#include "math/sphere.h"
#include "transform.h"


using namespace MR;
using namespace App;

const char* transformation_choices[] = { "rigid", "affine", "nonlinear", "rigid_affine", "rigid_nonlinear", "affine_nonlinear", "rigid_affine_nonlinear", NULL };

const OptionGroup multiContrastOptions =
  OptionGroup ("Multi-contrast options")
  + Option ("mc_weights", "relative weight of images used for multi-contrast registration. Default: 1.0 (equal weighting)")
    + Argument ("weights").type_sequence_float ();


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au) & Max Pietsch (maximilian.pietsch@kcl.ac.uk)";

  SYNOPSIS = "Register two images together using a symmetric rigid, affine or non-linear transformation model";

  DESCRIPTION
      + "By default this application will perform an affine, followed by non-linear registration."

      + "FOD registration (with apodised point spread reorientation) will be performed by default if the number of volumes "
        "in the 4th dimension equals the number of coefficients in an antipodally symmetric spherical harmonic series (e.g. 6, 15, 28 etc). "
        "The -no_reorientation option can be used to force reorientation off if required."

      // TODO link to 5D warp file format documentation
      + "Non-linear registration computes warps to map from both image1->image2 and image2->image1. "
        "Similar to Avants (2008) Med Image Anal. 12(1): 26–41, registration is performed by matching both the image1 and image2 in a 'midway space'. "
        "Warps can be saved as two deformation fields that map directly between image1->image2 and image2->image1, or if using -nl_warp_full as a single 5D file "
        "that stores all 4 warps image1->mid->image2, and image2->mid->image1. The 5D warp format stores x,y,z deformations in the 4th dimension, and uses the 5th dimension "
        "to index the 4 warps. The affine transforms estimated (to midway space) are also stored as comments in the image header. The 5D warp file can be used to reinitialise "
        "subsequent registrations, in addition to transforming images to midway space (e.g. for intra-subject alignment in a 2-time-point longitudinal analysis).";

  REFERENCES
  + "* If FOD registration is being performed:\n"
    "Raffelt, D.; Tournier, J.-D.; Fripp, J; Crozier, S.; Connelly, A. & Salvado, O. " // Internal
    "Symmetric diffeomorphic registration of fibre orientation distributions. "
    "NeuroImage, 2011, 56(3), 1171-1180"

  + "Raffelt, D.; Tournier, J.-D.; Crozier, S.; Connelly, A. & Salvado, O. " // Internal
    "Reorientation of fiber orientation distributions using apodized point spread functions. "
    "Magnetic Resonance in Medicine, 2012, 67, 844-855";


  ARGUMENTS
    + Argument ("image1 image2", "input image 1 ('moving') and input image 2 ('template')").type_image_in()
    + Argument ("+ contrast1 contrast2", "optional list of additional input images used as additional contrasts. "
      "Can be used multiple times. contrastX and imageX must share the same coordinate system. ").type_image_in().optional().allow_multiple();

  OPTIONS
  + Option ("type", "the registration type. Valid choices are: "
                    "rigid, affine, nonlinear, rigid_affine, rigid_nonlinear, affine_nonlinear, rigid_affine_nonlinear (Default: affine_nonlinear)")
    + Argument ("choice").type_choice (transformation_choices)

  + Option ("transformed", "image1 after registration transformed and regridded to the space of image2. "
    "Note that -transformed needs to be repeated for each contrast if multi-constrast registration is used.").allow_multiple()
    + Argument ("image").type_image_out ()

  + Option ("transformed_midway", "image1 and image2 after registration transformed and regridded to the midway space. "
    "Note that -transformed_midway needs to be repeated for each contrast if multi-constrast registration is used.").allow_multiple()
    + Argument ("image1_transformed").type_image_out ()
    + Argument ("image2_transformed").type_image_out ()

  + Option ("mask1", "a mask to define the region of image1 to use for optimisation.")
    + Argument ("filename").type_image_in ()

  + Option ("mask2", "a mask to define the region of image2 to use for optimisation.")
    + Argument ("filename").type_image_in ()

  + Registration::rigid_options

  + Registration::affine_options

  + Registration::adv_init_options

  + Registration::lin_stage_options

  + Registration::nonlinear_options

  + Registration::fod_options

  + multiContrastOptions

  + DataType::options();
}

using value_type = double;

void run () {

  vector<Header> input1, input2;
  const size_t n_images = 1 + (argument.size() - 2 ) / 3;
  { // parse arguments and load input headers
    if (std::abs(float (n_images) - (1.0 + float (argument.size() - 2 ) / 3.0)) > 1.e-6) {
      std::string err;
      for (const auto & a : argument)
        err += " " + str(a);
      throw Exception ("unexpected number of input images. arguments:" + err);
    }

    bool is1 = true;
    for (const auto& arg : argument) {
      if (str(arg) == "+")
        continue;
      if (is1)
        input1.push_back (Header::open (str(arg)));
      else
        input2.push_back (Header::open (str(arg)));
      is1 = !is1;
    }
  }
  assert (input1.size() == n_images);
  if (input1.size() != input2.size())
    throw Exception ("require same number of input images for image 1 and image 2");

  for (size_t i=0; i<n_images; i++) {
    if (input1[i].ndim() != input2[i].ndim())
      throw Exception ("input images " + input1[i].name() + " and "
        + input2[i].name() + " do not have the same number of dimensions");
    check_3D_nonunity (input1[i]);
    check_3D_nonunity (input2[i]);
  }



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

  // reorientation_forbidden required for output of transformed images because do_reorientation might change
  const bool reorientation_forbidden (get_options ("noreorientation").size());
  // do_reorientation == false --> registration without reorientation.
  // will be do_reorientation set to false if registration of all input SH images has lmax==0
  bool do_reorientation = !reorientation_forbidden;

  Eigen::MatrixXd directions_cartesian;
  opt = get_options ("directions");
  if (opt.size())
    directions_cartesian = Math::Sphere::spherical2cartesian (load_matrix (opt[0][0])).transpose();

  // check header transformations for equality
  Eigen::MatrixXd trafo = MR::Transform(input1[0]).scanner2voxel.linear();
  for (size_t i=1; i<n_images; i++) {
    if (!trafo.isApprox(MR::Transform(input1[i]).scanner2voxel.linear(),1e-5))
      WARN ("Multi contrast image has different header transformation from first image. Ignoring transformation of " + str(input1[i].name()));
  }
  trafo = MR::Transform(input2[0]).scanner2voxel.linear();
  for (size_t i=1; i<n_images; i++) {
    if (!trafo.isApprox(MR::Transform(input2[i]).scanner2voxel.linear(),1e-5))
      WARN ("Multi contrast image has different header transformation from first image. Ignoring transformation of " + str(input2[i].name()));
  }

  // multi contrast settings
  vector<Registration::MultiContrastSetting> mc_params (n_images);
  for (auto& mc : mc_params) {
    mc.do_reorientation = do_reorientation;
  }
  // set parameters for each contrast
  for (size_t i=0; i<n_images; i++) {
    // compare input1 and input2 for consistency
    if (i>0) check_dimensions (input1[i], input1[i-1], 0, 3);
    if (i>0) check_dimensions (input2[i], input2[i-1], 0, 3);
    if ((input1[i].ndim() != 3) and (input1[i].ndim() != 4))
      throw Exception ("image dimensionality other than 3 or 4 are not supported. image " +
        str(input1[i].name()) + " is " + str(input1[i].ndim()) + " dimensional");

    const size_t nvols1 = input1[i].ndim() == 3 ? 1 : input1[i].size(3);
    const size_t nvols2 = input2[i].ndim() == 3 ? 1 : input2[i].size(3);
    if (nvols1 != nvols2)
      throw Exception ("input images do not have the same number of volumes: " + str(input2[i].name()) + " and " + str(input1[i].name()));

    // set do_reorientation and image_lmax
    if (nvols1 == 1) { // 3D or one volume
      mc_params[i].do_reorientation = false;
      mc_params[i].image_lmax = 0;
      CONSOLE ("3D input pair "+input1[i].name()+", "+input2[i].name());
    } else { // more than one volume
      value_type val = (std::sqrt (float (1 + 8 * nvols1)) - 3.0) / 4.0;
      if (!(val - (int)val) && do_reorientation && nvols1 > 1) {
        CONSOLE ("SH image input pair "+input1[i].name()+", "+input2[i].name());
        mc_params[i].do_reorientation = true;
        mc_params[i].image_lmax = Math::SH::LforN (nvols1);
        if (!directions_cartesian.cols())
          directions_cartesian = Math::Sphere::spherical2cartesian (DWI::Directions::electrostatic_repulsion_60()).transpose();
      } else {
        CONSOLE ("4D scalar input pair "+input1[i].name()+", "+input2[i].name());
        mc_params[i].do_reorientation = false;
        mc_params[i].image_lmax = 0;
      }
    }
    // set lmax to image_lmax and set image_nvols
    mc_params[i].lmax = mc_params[i].image_lmax;
    mc_params[i].image_nvols = input1[i].ndim() < 4 ? 1 : input1[i].size(3);
  }

  ssize_t max_mc_image_lmax = std::max_element(mc_params.begin(), mc_params.end(),
    [](const Registration::MultiContrastSetting& x, const Registration::MultiContrastSetting& y)
    {return x.lmax < y.lmax;})->lmax;

  do_reorientation = std::any_of(mc_params.begin(), mc_params.end(),
    [](Registration::MultiContrastSetting const& i){return i.do_reorientation;});
  if (do_reorientation)
    CONSOLE("performing FOD registration");
  if (!do_reorientation and directions_cartesian.cols())
    WARN ("-directions option ignored since no FOD reorientation is being performed");

  INFO ("maximum input lmax: "+str(max_mc_image_lmax));

  opt = get_options ("transformed");
  vector<std::string> im1_transformed_paths;
  if (opt.size()) {
    if (opt.size() > n_images)
      throw Exception ("number of -transformed images exceeds number of contrasts");
    if (opt.size() != n_images)
      WARN ("number of -transformed images lower than number of contrasts");
    for (size_t c = 0; c < opt.size(); c++) {
      Registration::check_image_output (opt[c][0], input2[c]);
      im1_transformed_paths.push_back(opt[c][0]);
      INFO (input1[c].name() + ", transformed to space of image2, will be written to " + im1_transformed_paths[c]);
    }
  }


  vector<std::string> input1_midway_transformed_paths;
  vector<std::string> input2_midway_transformed_paths;
  opt = get_options ("transformed_midway");
  if (opt.size()) {
    if (opt.size() > n_images)
      throw Exception ("number of -transformed_midway images exceeds number of contrasts");
    if (opt.size() != n_images)
      WARN ("number of -transformed_midway images lower than number of contrasts");
    for (size_t c = 0; c < opt.size(); c++) {
      Registration::check_image_output (opt[c][0], input2[c]);
      input1_midway_transformed_paths.push_back(opt[c][0]);
      INFO (input1[c].name() + ", transformed to midway space, will be written to " + input1_midway_transformed_paths[c]);
      Registration::check_image_output (opt[c][1], input1[c]);
      input2_midway_transformed_paths.push_back(opt[c][1]);
      INFO (input2[c].name() + ", transformed to midway space, will be written to " + input2_midway_transformed_paths[c]);
    }
  }

  opt = get_options ("mask1");
  Image<value_type> im1_mask;
  if (opt.size ()) {
    im1_mask = Image<value_type>::open(opt[0][0]);
    check_dimensions (input1[0], im1_mask, 0, 3);
  }

  opt = get_options ("mask2");
  Image<value_type> im2_mask;
  if (opt.size ()) {
    im2_mask = Image<value_type>::open(opt[0][0]);
    check_dimensions (input2[0], im2_mask, 0, 3);
  }


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

  opt = get_options ("rigid_1tomidway");
  bool output_rigid_1tomid = false;
  std::string rigid_1tomid_filename;
  if (opt.size()) {
   if (!do_rigid)
     throw Exception ("midway rigid transformation output requested when no rigid registration is requested");
   output_rigid_1tomid = true;
   rigid_1tomid_filename = std::string (opt[0][0]);
  }

  opt = get_options ("rigid_2tomidway");
  bool output_rigid_2tomid = false;
  std::string rigid_2tomid_filename;
  if (opt.size()) {
   if (!do_rigid)
     throw Exception ("midway rigid transformation output requested when no rigid registration is requested");
   output_rigid_2tomid = true;
   rigid_2tomid_filename = std::string (opt[0][0]);
  }

  Registration::Transform::Rigid rigid;
  opt = get_options ("rigid_init_matrix");
  bool init_rigid_matrix_set = false;
  if (opt.size()) {
    init_rigid_matrix_set = true;
    Eigen::Vector3 centre;
    transform_type rigid_transform = load_transform (opt[0][0], centre);
    rigid.set_transform (rigid_transform);
    if (!std::isfinite(centre(0))) {
      rigid_registration.set_init_translation_type (Registration::Transform::Init::set_centre_mass);
    } else {
      rigid.set_centre_without_transform_update(centre);
      rigid_registration.set_init_translation_type (Registration::Transform::Init::none);
    }
  }

  opt = get_options ("rigid_init_translation");
  if (opt.size()) {
    if (init_rigid_matrix_set)
      throw Exception ("options -rigid_init_matrix and -rigid_init_translation are mutually exclusive");
    Registration::set_init_translation_model_from_option (rigid_registration, (int)opt[0][0]);
  }

  opt = get_options ("rigid_init_rotation");
  if (opt.size()) {
    Registration::set_init_rotation_model_from_option (rigid_registration, (int)opt[0][0]);
  }

  opt = get_options ("rigid_scale");
  if (opt.size ()) {
    if (!do_rigid)
      throw Exception ("the rigid multi-resolution scale factors were input when no rigid registration is requested");
    rigid_registration.set_scale_factor (parse_floats (opt[0][0]));
  }

  // opt = get_options ("rigid_stage.iterations");
  // if (opt.size ()) {
  //   if (!do_rigid)
  //     throw Exception ("the rigid iterations were input when no rigid registration is requested");
  //   rigid_registration.set_stage_iterations (parse_ints (opt[0][0]));
  // }

  opt = get_options ("rigid_loop_density");
  if (opt.size ()) {
    if (!do_rigid)
      throw Exception ("the rigid sparsity factor was input when no rigid registration is requested");
    rigid_registration.set_loop_density (parse_floats (opt[0][0]));
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

  if (rigid_metric == Registration::NCC)
    throw Exception ("TODO: cross correlation metric not yet implemented");

  opt = get_options ("rigid_metric.diff.estimator");
  Registration::LinearRobustMetricEstimatorType rigid_estimator = Registration::None;
  if (opt.size()) {
    if (rigid_metric != Registration::Diff)
      throw Exception ("rigid_metric.diff.estimator set but cost function is not diff.");
    switch ((int)opt[0][0]) {
      case 0:
        rigid_estimator = Registration::L1;
        break;
      case 1:
        rigid_estimator = Registration::L2;
        break;
      case 2:
        rigid_estimator = Registration::LP;
        break;
      default:
        break;
    }
  }

  opt = get_options ("rigid_lmax");
  vector<int> rigid_lmax;
  if (opt.size ()) {
    if (!do_rigid)
      throw Exception ("the -rigid_lmax option has been set when no rigid registration is requested");
    if (max_mc_image_lmax == 0)
      throw Exception ("-rigid_lmax option is not valid if no input image is FOD image");
    rigid_lmax = parse_ints (opt[0][0]);
    for (size_t i = 0; i < rigid_lmax.size (); ++i)
       if (rigid_lmax[i] > max_mc_image_lmax) {
        WARN ("the requested -rigid_lmax exceeds the lmax of the input images, setting it to " + str(max_mc_image_lmax));
        rigid_lmax[i] = max_mc_image_lmax;
       }
    rigid_registration.set_lmax (rigid_lmax);
  }

  std::ofstream linear_logstream;
  opt = get_options ("rigid_log");
  if (opt.size()) {
    if (!do_rigid)
      throw Exception ("the -rigid_log option has been set when no rigid registration is requested");
    linear_logstream.open (opt[0][0]);
    rigid_registration.set_log_stream (linear_logstream.rdbuf());
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
  opt = get_options ("affine_init_matrix");
  bool init_affine_matrix_set = false;
  if (opt.size()) {
    if (init_rigid_matrix_set)
      throw Exception ("you cannot initialise registrations with both rigid and affine transformations");
    if (do_rigid)
      throw Exception ("you cannot initialise with -affine_init_matrix since a rigid registration is being performed");

    init_affine_matrix_set = true;
    Eigen::Vector3 centre;
    transform_type affine_transform = load_transform (opt[0][0], centre);
    affine.set_transform (affine_transform);
    if (!std::isfinite(centre(0))) {
      affine_registration.set_init_translation_type (Registration::Transform::Init::set_centre_mass);
    } else {
      affine.set_centre_without_transform_update(centre);
      affine_registration.set_init_translation_type (Registration::Transform::Init::none);
    }
  }

  opt = get_options ("affine_init_translation");
  if (opt.size()) {
    if (init_affine_matrix_set)
      throw Exception ("options -affine_init_matrix and -affine_init_translation are mutually exclusive");
    Registration::set_init_translation_model_from_option (affine_registration, (int)opt[0][0]);
  }

  opt = get_options ("affine_init_rotation");
  if (opt.size()) {
    if (init_affine_matrix_set)
      throw Exception ("options -affine_init_matrix and -affine_init_rotation are mutually exclusive");
    Registration::set_init_rotation_model_from_option (affine_registration, (int)opt[0][0]);
  }

  opt = get_options ("affine_scale");
  if (opt.size ()) {
    if (!do_affine)
      throw Exception ("the affine multi-resolution scale factors were input when no affine registration is requested");
    affine_registration.set_scale_factor (parse_floats (opt[0][0]));
  }

  // opt = get_options ("affine_stage.iterations");
  // if (opt.size ()) {
  //   if (!do_affine)
  //     throw Exception ("the affine repetition factors were input when no affine registration is requested");
  //   affine_registration.set_stage_iterations (parse_ints (opt[0][0]));
  // }

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

  if (affine_metric == Registration::NCC)
    throw Exception ("TODO cross correlation metric not yet implemented");

  opt = get_options ("affine_metric.diff.estimator");
  Registration::LinearRobustMetricEstimatorType affine_estimator = Registration::None;
  if (opt.size()) {
    if (affine_metric != Registration::Diff)
      throw Exception ("affine_metric.diff.estimator set but cost function is not diff.");
    switch ((int)opt[0][0]) {
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

  opt = get_options ("affine_niter");
  if (opt.size ()) {
    if (!do_affine)
      throw Exception ("the number of affine iterations have been input when no affine registration is requested");
    affine_registration.set_max_iter (parse_ints (opt[0][0]));
  }

  opt = get_options ("affine_lmax");
  vector<int> affine_lmax;
  if (opt.size ()) {
    if (!do_affine)
      throw Exception ("the -affine_lmax option has been set when no affine registration is requested");
    if (max_mc_image_lmax == 0)
      throw Exception ("-affine_lmax option is not valid if no input image is FOD image");
    affine_lmax = parse_ints (opt[0][0]);
    for (size_t i = 0; i < affine_lmax.size (); ++i)
      if (affine_lmax[i] > max_mc_image_lmax) {
        WARN ("the requested -affine_lmax exceeds the lmax of the input images, setting it to " + str(max_mc_image_lmax));
        affine_lmax[i] = max_mc_image_lmax;
      }
    affine_registration.set_lmax (affine_lmax);
  }

  opt = get_options ("affine_log");
  if (opt.size()) {
    if (!do_affine)
      throw Exception ("the -affine_log option has been set when no rigid registration is requested");
    linear_logstream.open (opt[0][0]);
    affine_registration.set_log_stream (linear_logstream.rdbuf());
  }


  // ****** LINEAR INITIALISATION AND STAGE OPTIONS *******
  if (!do_rigid and !do_affine) {
    for (auto& s: Registration::adv_init_options) {
      if (get_options(s.id).size()) {
        std::stringstream msg;
        msg << "cannot use option -" << s.id << " when no linear registration is requested";
        throw Exception (msg.str());
      }
    }
    for (auto& s: Registration::lin_stage_options) {
      if (get_options(s.id).size()) {
        std::stringstream msg;
        msg << "cannot use option -" << s.id << " when no linear registration is requested";
        throw Exception (msg.str());
      }
    }
  }

  if (do_rigid)
    Registration::parse_general_options (rigid_registration);
  if (do_affine)
    Registration::parse_general_options (affine_registration);

  // ****** NON-LINEAR REGISTRATION OPTIONS *******
  Registration::NonLinear nl_registration;
  opt = get_options ("nl_warp");
  std::string warp1_filename;
  std::string warp2_filename;
  if (opt.size()) {
    if (!do_nonlinear)
      throw Exception ("Non-linear warp output requested when no non-linear registration is requested");
    warp1_filename = std::string (opt[0][0]);
    warp2_filename = std::string (opt[0][1]);
  }

  opt = get_options ("nl_warp_full");
  std::string warp_full_filename;
  if (opt.size()) {
    if (!do_nonlinear)
      throw Exception ("Non-linear warp output requested when no non-linear registration is requested");
    warp_full_filename = std::string (opt[0][0]);
  }


  opt = get_options ("nl_init");
  bool nonlinear_init = false;
  if (opt.size()) {
    nonlinear_init = true;

    if (!do_nonlinear)
      throw Exception ("the non linear initialisation option -nl_init cannot be used when no non linear registration is requested");

    Image<default_type> input_warps = Image<default_type>::open (opt[0][0]);
    if (input_warps.ndim() != 5)
      throw Exception ("non-linear initialisation input is not 5D. Input must be from previous non-linear output");

    nl_registration.initialise (input_warps);

    if (do_affine) {
      WARN ("no affine registration will be performed when initialising with non-linear non-linear warps");
      do_affine = false;
    }
    if (do_rigid) {
      WARN ("no rigid registration will be performed when initialising with non-linear non-linear warps");
      do_rigid = false;
    }
    if (init_affine_matrix_set)
      WARN ("-affine_init has no effect since the non-linear init warp also contains the linear transform in the image header");
    if (init_rigid_matrix_set)
      WARN ("-rigid_init has no effect since the non-linear init warp also contains the linear transform in the image header");
  }


  opt = get_options ("nl_scale");
  if (opt.size ()) {
    if (!do_nonlinear)
      throw Exception ("the non-linear multi-resolution scale factors were input when no non-linear registration is requested");
    vector<default_type> scale_factors = parse_floats (opt[0][0]);
    if (nonlinear_init) {
      WARN ("-nl_scale option ignored since only the full resolution will be performed when initialising with non-linear warp");
    } else {
      nl_registration.set_scale_factor (scale_factors);
    }
  }

  opt = get_options ("nl_niter");
  if (opt.size ()) {
    if (!do_nonlinear)
      throw Exception ("the number of non-linear iterations have been input when no non-linear registration is requested");
    vector<int> iterations_per_level = parse_ints (opt[0][0]);
    if (nonlinear_init && iterations_per_level.size() > 1)
      throw Exception ("when initialising the non-linear registration the max number of iterations can only be defined for a single level");
    else
      nl_registration.set_max_iter (iterations_per_level);
  }

  opt = get_options ("nl_update_smooth");
  if (opt.size()) {
    if (!do_nonlinear)
      throw Exception ("the warp update field smoothing parameter was input when no non-linear registration is requested");
    nl_registration.set_update_smoothing (opt[0][0]);
  }

  opt = get_options ("nl_disp_smooth");
  if (opt.size()) {
    if (!do_nonlinear)
      throw Exception ("the displacement field smoothing parameter was input when no non-linear registration is requested");
    nl_registration.set_disp_smoothing (opt[0][0]);
  }

  opt = get_options ("nl_grad_step");
  if (opt.size()) {
    if (!do_nonlinear)
      throw Exception ("the initial gradient step size was input when no non-linear registration is requested");
    nl_registration.set_init_grad_step (opt[0][0]);
  }

  opt = get_options ("nl_lmax");
  vector<int> nl_lmax;
  if (opt.size()) {
    if (!do_nonlinear)
      throw Exception ("the -nl_lmax option has been set when no non-linear registration is requested");
    if (input1[0].ndim() < 4)
      throw Exception ("-nl_lmax option is not valid with 3D images");
    nl_lmax = parse_ints (opt[0][0]);
    nl_registration.set_lmax (nl_lmax);
    for (size_t i = 0; i < (nl_lmax).size (); ++i)
      if ((nl_lmax)[i] > max_mc_image_lmax)
        throw Exception ("the requested -nl_lmax exceeds the lmax of the input images");
  }


  // ******  MC options  *******
  // TODO: set tissue specific lmax?

  opt = get_options ("mc_weights");
  if (opt.size()) {
    vector<default_type> mc_weights = parse_floats (opt[0][0]);
    if (mc_weights.size() == 1)
      mc_weights.resize (n_images, mc_weights[0]);
    else if (mc_weights.size() != n_images)
      throw Exception ("number of mc_weights does not match number of contrasts");
    for (const default_type & w : mc_weights)
      if (w < 0.0) throw Exception ("mc_weights must be non-negative");

    if (do_nonlinear) {
      default_type sm = 0.0;
      std::for_each (mc_weights.begin(), mc_weights.end(), [&] (default_type n) {sm += n;});
      if (std::abs (sm - n_images) > 1.e-6)
        WARN ("mc_weights do not sum to the number of contrasts. This changes the regularisation of the nonlinear registration.");
    }

    for (size_t idx = 0; idx < n_images; idx++)
      mc_params[idx].weight = mc_weights[idx];
  }

  {
    ssize_t max_requested_lmax = 0;
    if (max_mc_image_lmax != 0) {
      if (do_rigid) max_requested_lmax = std::max(max_requested_lmax, rigid_registration.get_lmax());
      if (do_affine) max_requested_lmax = std::max(max_requested_lmax, affine_registration.get_lmax());
      if (do_nonlinear) max_requested_lmax = std::max(max_requested_lmax, nl_registration.get_lmax());
      INFO ("maximum used lmax: "+str(max_requested_lmax));
    }

    for (size_t idx = 0; idx < n_images; ++idx) {
      mc_params[idx].lmax = std::min (mc_params[idx].image_lmax, max_requested_lmax);
      if (input1[idx].ndim() == 3)
        mc_params[idx].nvols = 1;
      else if (mc_params[idx].do_reorientation) {
        mc_params[idx].nvols = Math::SH::NforL (mc_params[idx].lmax);
      } else
        mc_params[idx].nvols = input1[idx].size(3);
    }
    mc_params[0].start = 0;
    for (size_t idx = 1; idx < n_images; ++idx)
      mc_params[idx].start = mc_params[idx-1].start + mc_params[idx-1].nvols;

    for (const auto & mc : mc_params)
      DEBUG (str(mc));
  }

  if (mc_params.size() > 1) {
    if (do_rigid) rigid_registration.set_mc_parameters (mc_params);
    if (do_affine) affine_registration.set_mc_parameters (mc_params);
    if (do_nonlinear) nl_registration.set_mc_parameters (mc_params);
  }

  // ****** PARSING DONE, PRELOAD THE DATA *******
  // only load the volumes we actually need for the highest lmax requested
  // load multiple tissue types into the same 4D image
  // drop last axis if input is 4D with one volume for speed reasons
  Image<value_type> images1, images2;
  INFO ("preloading input1...");
  Registration::preload_data (input1, images1, mc_params);
  INFO ("preloading input2...");
  Registration::preload_data (input2, images2, mc_params);
  INFO ("preloading input images done");

  // ****** RUN RIGID REGISTRATION *******
  if (do_rigid) {
    CONSOLE ("running rigid registration");

    if (images2.ndim() == 4) {
      if (do_reorientation)
        rigid_registration.set_directions (directions_cartesian);
      // if (rigid_metric == Registration::NCC) // TODO
      if (rigid_metric == Registration::Diff) {
        if (rigid_estimator == Registration::None) {
          Registration::Metric::MeanSquared4D<Image<value_type>, Image<value_type>> metric;
          rigid_registration.run_masked (metric, rigid, images1, images2, im1_mask, im2_mask);
        } else if (rigid_estimator == Registration::L1) {
          Registration::Metric::L1 estimator;
          Registration::Metric::DifferenceRobust4D<Image<value_type>, Image<value_type>, Registration::Metric::L1> metric (images1, images2, estimator);
          rigid_registration.run_masked (metric, rigid, images1, images2, im1_mask, im2_mask);
        } else if (rigid_estimator == Registration::L2) {
          Registration::Metric::L2 estimator;
          Registration::Metric::DifferenceRobust4D<Image<value_type>, Image<value_type>, Registration::Metric::L2> metric (images1, images2, estimator);
          rigid_registration.run_masked (metric, rigid, images1, images2, im1_mask, im2_mask);
        } else if (rigid_estimator == Registration::LP) {
          Registration::Metric::LP estimator;
          Registration::Metric::DifferenceRobust4D<Image<value_type>, Image<value_type>, Registration::Metric::LP> metric (images1, images2, estimator);
          rigid_registration.run_masked (metric, rigid, images1, images2, im1_mask, im2_mask);
        } else throw Exception ("FIXME: estimator selection");
      } else throw Exception ("FIXME: metric selection");
    } else { // 3D
      if (rigid_metric == Registration::NCC){
        Registration::Metric::NormalisedCrossCorrelation metric;
        vector<size_t> extent(3,3);
        rigid_registration.set_extent (extent);
        rigid_registration.run_masked (metric, rigid, images1, images2, im1_mask, im2_mask);
      }
      else if (rigid_metric == Registration::Diff) {
        if (rigid_estimator == Registration::None) {
          Registration::Metric::MeanSquared metric;
          rigid_registration.run_masked (metric, rigid, images1, images2, im1_mask, im2_mask);
        } else if (rigid_estimator == Registration::L1) {
          Registration::Metric::L1 estimator;
          Registration::Metric::DifferenceRobust<Registration::Metric::L1> metric(estimator);
          rigid_registration.run_masked (metric, rigid, images1, images2, im1_mask, im2_mask);
        } else if (rigid_estimator == Registration::L2) {
          Registration::Metric::L2 estimator;
          Registration::Metric::DifferenceRobust<Registration::Metric::L2> metric(estimator);
          rigid_registration.run_masked (metric, rigid, images1, images2, im1_mask, im2_mask);
        } else if (rigid_estimator == Registration::LP) {
          Registration::Metric::LP estimator;
          Registration::Metric::DifferenceRobust<Registration::Metric::LP> metric(estimator);
          rigid_registration.run_masked (metric, rigid, images1, images2, im1_mask, im2_mask);
        } else throw Exception ("FIXME: estimator selection");
      } else throw Exception ("FIXME: metric selection");
    }

    if (output_rigid_1tomid)
      save_transform (rigid.get_centre(), rigid.get_transform_half(), rigid_1tomid_filename);

    if (output_rigid_2tomid)
      save_transform (rigid.get_centre(), rigid.get_transform_half_inverse(), rigid_2tomid_filename);

    if (output_rigid)
      save_transform (rigid.get_centre(), rigid.get_transform(), rigid_filename);
  }

  // ****** RUN AFFINE REGISTRATION *******
  if (do_affine) {
    CONSOLE ("running affine registration");

    if (do_rigid) {
      affine.set_centre (rigid.get_centre());
      affine.set_transform (rigid.get_transform());
      affine_registration.set_init_translation_type (Registration::Transform::Init::none);
    }

    if (images2.ndim() == 4) {
      if (do_reorientation)
        affine_registration.set_directions (directions_cartesian);
      // if (affine_metric == Registration::NCC) // TODO
      if (affine_metric == Registration::Diff) {
        if (affine_estimator == Registration::None) {
          Registration::Metric::MeanSquared4D<Image<value_type>, Image<value_type>> metric;
          affine_registration.run_masked (metric, affine, images1, images2, im1_mask, im2_mask);
        } else if (affine_estimator == Registration::L1) {
          Registration::Metric::L1 estimator;
          Registration::Metric::DifferenceRobust4D<Image<value_type>, Image<value_type>, Registration::Metric::L1> metric (images1, images2, estimator);
          affine_registration.run_masked (metric, affine, images1, images2, im1_mask, im2_mask);
        } else if (affine_estimator == Registration::L2) {
          Registration::Metric::L2 estimator;
          Registration::Metric::DifferenceRobust4D<Image<value_type>, Image<value_type>, Registration::Metric::L2> metric (images1, images2, estimator);
          affine_registration.run_masked (metric, affine, images1, images2, im1_mask, im2_mask);
        } else if (affine_estimator == Registration::LP) {
          Registration::Metric::LP estimator;
          Registration::Metric::DifferenceRobust4D<Image<value_type>, Image<value_type>, Registration::Metric::LP> metric (images1, images2, estimator);
          affine_registration.run_masked (metric, affine, images1, images2, im1_mask, im2_mask);
        } else throw Exception ("FIXME: estimator selection");
      } else throw Exception ("FIXME: metric selection");
    } else { // 3D
      if (affine_metric == Registration::NCC){
        Registration::Metric::NormalisedCrossCorrelation metric;
        vector<size_t> extent(3,3);
        affine_registration.set_extent (extent);
        affine_registration.run_masked (metric, affine, images1, images2, im1_mask, im2_mask);
      }
      else if (affine_metric == Registration::Diff) {
        if (affine_estimator == Registration::None) {
          Registration::Metric::MeanSquared metric;
          affine_registration.run_masked (metric, affine, images1, images2, im1_mask, im2_mask);
        } else if (affine_estimator == Registration::L1) {
          Registration::Metric::L1 estimator;
          Registration::Metric::DifferenceRobust<Registration::Metric::L1> metric(estimator);
          affine_registration.run_masked (metric, affine, images1, images2, im1_mask, im2_mask);
        } else if (affine_estimator == Registration::L2) {
          Registration::Metric::L2 estimator;
          Registration::Metric::DifferenceRobust<Registration::Metric::L2> metric(estimator);
          affine_registration.run_masked (metric, affine, images1, images2, im1_mask, im2_mask);
        } else if (affine_estimator == Registration::LP) {
          Registration::Metric::LP estimator;
          Registration::Metric::DifferenceRobust<Registration::Metric::LP> metric(estimator);
          affine_registration.run_masked (metric, affine, images1, images2, im1_mask, im2_mask);
        } else throw Exception ("FIXME: estimator selection");
      } else throw Exception ("FIXME: metric selection");
    }
    if (output_affine_1tomid)
      save_transform (affine.get_centre(), affine.get_transform_half(), affine_1tomid_filename);

    if (output_affine_2tomid)
      save_transform (affine.get_centre(), affine.get_transform_half_inverse(), affine_2tomid_filename);

    if (output_affine)
      save_transform (affine.get_centre(), affine.get_transform(), affine_filename);
  }


  // ****** RUN NON-LINEAR REGISTRATION *******
  if (do_nonlinear) {
    CONSOLE ("running non-linear registration");

    if (do_reorientation)
      nl_registration.set_aPSF_directions (directions_cartesian);

    if (do_affine || init_affine_matrix_set) {
      nl_registration.run (affine, images1, images2, im1_mask, im2_mask);
    } else if (do_rigid || init_rigid_matrix_set) {
      nl_registration.run (rigid, images1, images2, im1_mask, im2_mask);
    } else {
      Registration::Transform::Affine identity_transform;
      nl_registration.run (identity_transform, images1, images2, im1_mask, im2_mask);
    }
    if (warp_full_filename.size()) {
      //TODO add affine parameters to comments too?
      Header output_header = nl_registration.get_output_warps_header();
      nl_registration.write_params_to_header (output_header);
      nl_registration.write_linear_to_header (output_header);
      output_header.datatype() = DataType::from_command_line (DataType::Float32);
      auto output_warps = Image<float>::create (warp_full_filename, output_header);
      nl_registration.get_output_warps (output_warps);
    }

    if (warp1_filename.size()) {
      Header output_header (images2);
      output_header.ndim() = 4;
      output_header.size(3) =3;
      nl_registration.write_params_to_header (output_header);
      output_header.datatype() = DataType::from_command_line (DataType::Float32);
      auto warp1 = Image<default_type>::create (warp1_filename, output_header).with_direct_io();
      Registration::Warp::compute_full_deformation (nl_registration.get_im2_to_mid_linear().inverse(),
                                                    *(nl_registration.get_mid_to_im2()),
                                                    *(nl_registration.get_im1_to_mid()),
                                                    nl_registration.get_im1_to_mid_linear(), warp1);
    }

    if (warp2_filename.size()) {
      Header output_header (images1);
      output_header.ndim() = 4;
      output_header.size(3) = 3;
      nl_registration.write_params_to_header (output_header);
      output_header.datatype() = DataType::from_command_line (DataType::Float32);
      auto warp2 = Image<default_type>::create (warp2_filename, output_header).with_direct_io();
      Registration::Warp::compute_full_deformation (nl_registration.get_im1_to_mid_linear().inverse(),
                                                    *(nl_registration.get_mid_to_im1()),
                                                    *(nl_registration.get_im2_to_mid()),
                                                    nl_registration.get_im2_to_mid_linear(), warp2);
    }
  }


  if (im1_transformed_paths.size()) {
    CONSOLE ("Writing input images1 transformed to space of images2...");

    Image<default_type> deform_field;
    if (do_nonlinear) {
      Header deform_header (input2[0]);
      deform_header.ndim() = 4;
      deform_header.size(3) = 3;
      deform_field = Image<default_type>::scratch (deform_header);
      Registration::Warp::compute_full_deformation (nl_registration.get_im2_to_mid_linear().inverse(),
                                                    *(nl_registration.get_mid_to_im2()),
                                                    *(nl_registration.get_im1_to_mid()),
                                                    nl_registration.get_im1_to_mid_linear(),
                                                    deform_field);
    }

    for (size_t idx = 0; idx < im1_transformed_paths.size(); idx++) {
      CONSOLE ("... " + im1_transformed_paths[idx]);
      {
        // LogLevelLatch log_level (0);
        Image<value_type> im1_image = Image<value_type>::open (input1[idx].name());

        Header transformed_header (input2[idx]);
        transformed_header.datatype() = DataType::from_command_line (DataType::Float32);
        Image<value_type> im1_transformed = Image<value_type>::create (im1_transformed_paths[idx], transformed_header);

        const size_t nvols = im1_image.ndim() == 3 ? 1 : im1_image.size(3);
        const value_type val = (std::sqrt (float (1 + 8 * nvols)) - 3.0) / 4.0;
        const bool reorient_output =  !reorientation_forbidden && (nvols > 1) && !(val - (int)val);

        if (do_nonlinear) {
          Filter::warp<Interp::Cubic> (im1_image, im1_transformed, deform_field, 0.0);
          if (reorient_output)
            Registration::Transform::reorient_warp ("reorienting FODs",
                                                    im1_transformed,
                                                    deform_field,
                                                    Math::Sphere::spherical2cartesian (DWI::Directions::electrostatic_repulsion_300()).transpose());
        } else if (do_affine) {
          Filter::reslice<Interp::Cubic> (im1_image, im1_transformed, affine.get_transform(), Adapter::AutoOverSample, 0.0);
          if (reorient_output)
            Registration::Transform::reorient ("reorienting FODs",
                                               im1_transformed,
                                               im1_transformed,
                                               affine.get_transform(),
                                               Math::Sphere::spherical2cartesian (DWI::Directions::electrostatic_repulsion_300()).transpose());
        } else { // rigid
          Filter::reslice<Interp::Cubic> (im1_image, im1_transformed, rigid.get_transform(), Adapter::AutoOverSample, 0.0);
          if (reorient_output)
            Registration::Transform::reorient ("reorienting FODs",
                                               im1_transformed,
                                               im1_transformed,
                                               rigid.get_transform(),
                                               Math::Sphere::spherical2cartesian (DWI::Directions::electrostatic_repulsion_300()).transpose());
        }
      }
    }
  }


  if (input1_midway_transformed_paths.size() and input2_midway_transformed_paths.size()) {
    Header midway_header;
    Image<default_type> im1_deform_field, im2_deform_field;

    if (do_nonlinear)
      midway_header = Header (*nl_registration.get_im1_to_mid());
    else if (do_affine)
      midway_header = compute_minimum_average_header (input1[0], input2[0], affine.get_transform_half_inverse(), affine.get_transform_half());
    else // rigid
      midway_header = compute_minimum_average_header (input1[0], input2[0], rigid.get_transform_half_inverse(), rigid.get_transform_half());
    midway_header.datatype() = DataType::from_command_line (DataType::Float32);

    // process input1 then input2 to reduce memory consumption
    CONSOLE ("Writing input1 transformed to midway...");
    if (do_nonlinear) {
      im1_deform_field = Image<default_type>::scratch (*(nl_registration.get_im1_to_mid()));
      Registration::Warp::compose_linear_deformation (nl_registration.get_im1_to_mid_linear(), *(nl_registration.get_im1_to_mid()), im1_deform_field);
    }

    for (size_t idx = 0; idx < input1_midway_transformed_paths.size(); idx++) {
      CONSOLE ("... " + input1_midway_transformed_paths[idx]);
      {
        // LogLevelLatch log_level (0);
        Image<value_type> im1_image = Image<value_type>::open (input1[idx].name());
        midway_header.ndim() = im1_image.ndim();
        if (midway_header.ndim() == 4)
          midway_header.size(3) = im1_image.size(3);

        const size_t nvols = im1_image.ndim() == 3 ? 1 : im1_image.size(3);
        const value_type val = (std::sqrt (float (1 + 8 * nvols)) - 3.0) / 4.0;
        const bool reorient_output =  !reorientation_forbidden && (nvols > 1) && !(val - (int)val);

        if (do_nonlinear) {
          auto im1_midway = Image<default_type>::create (input1_midway_transformed_paths[idx], midway_header);
          Filter::warp<Interp::Cubic> (im1_image, im1_midway, im1_deform_field, 0.0);
          if (reorient_output)
            Registration::Transform::reorient_warp ("reorienting ODFs", im1_midway, im1_deform_field,
                                                    Math::Sphere::spherical2cartesian (DWI::Directions::electrostatic_repulsion_300()).transpose());
        } else if (do_affine) {
          auto im1_midway = Image<default_type>::create (input1_midway_transformed_paths[idx], midway_header);
          Filter::reslice<Interp::Cubic> (im1_image, im1_midway, affine.get_transform_half(), Adapter::AutoOverSample, 0.0);
          if (reorient_output)
            Registration::Transform::reorient ("reorienting ODFs", im1_midway, im1_midway, affine.get_transform_half(), Math::Sphere::spherical2cartesian (DWI::Directions::electrostatic_repulsion_300()).transpose());
        } else { // rigid
          auto im1_midway = Image<default_type>::create (input1_midway_transformed_paths[idx], midway_header);
          Filter::reslice<Interp::Cubic> (im1_image, im1_midway, rigid.get_transform_half(), Adapter::AutoOverSample, 0.0);
          if (reorient_output)
            Registration::Transform::reorient ("reorienting ODFs", im1_midway, im1_midway, rigid.get_transform_half(), Math::Sphere::spherical2cartesian (DWI::Directions::electrostatic_repulsion_300()).transpose());
        }
      }
    }

    CONSOLE ("Writing input2 transformed to midway...");
    if (do_nonlinear) {
      im2_deform_field = Image<default_type>::scratch (*(nl_registration.get_im2_to_mid()));
      Registration::Warp::compose_linear_deformation (nl_registration.get_im2_to_mid_linear(), *(nl_registration.get_im2_to_mid()), im2_deform_field);
    }

    for (size_t idx = 0; idx < input2_midway_transformed_paths.size(); idx++) {
      CONSOLE ("... " + input2_midway_transformed_paths[idx]);
      {
        // LogLevelLatch log_level (0);
        Image<value_type> im2_image = Image<value_type>::open (input2[idx].name());
        midway_header.ndim() = im2_image.ndim();
        if (midway_header.ndim() == 4)
          midway_header.size(3) = im2_image.size(3);

        const size_t nvols = im2_image.ndim() == 3 ? 1 : im2_image.size(3);
        const value_type val = (std::sqrt (float (1 + 8 * nvols)) - 3.0) / 4.0;
        const bool reorient_output =  !reorientation_forbidden && (nvols > 1) && !(val - (int)val);

        if (do_nonlinear) {
          auto im2_midway = Image<default_type>::create (input2_midway_transformed_paths[idx], midway_header);
          Filter::warp<Interp::Cubic> (im2_image, im2_midway, im2_deform_field, 0.0);
          if (reorient_output)
            Registration::Transform::reorient_warp ("reorienting ODFs", im2_midway, im2_deform_field,
                                                    Math::Sphere::spherical2cartesian (DWI::Directions::electrostatic_repulsion_300()).transpose());
        } else if (do_affine) {
          auto im2_midway = Image<default_type>::create (input2_midway_transformed_paths[idx], midway_header);
          Filter::reslice<Interp::Cubic> (im2_image, im2_midway, affine.get_transform_half_inverse(), Adapter::AutoOverSample, 0.0);
          if (reorient_output)
            Registration::Transform::reorient ("reorienting ODFs", im2_midway, im2_midway, affine.get_transform_half_inverse(), Math::Sphere::spherical2cartesian (DWI::Directions::electrostatic_repulsion_300()).transpose());
        } else { // rigid
          auto im2_midway = Image<default_type>::create (input2_midway_transformed_paths[idx], midway_header);
          Filter::reslice<Interp::Cubic> (im2_image, im2_midway, rigid.get_transform_half_inverse(), Adapter::AutoOverSample, 0.0);
          if (reorient_output)
            Registration::Transform::reorient ("reorienting ODFs", im2_midway, im2_midway, rigid.get_transform_half_inverse(), Math::Sphere::spherical2cartesian (DWI::Directions::electrostatic_repulsion_300()).transpose());
        }
      }
    }
  }

  if (get_options ("affine_log").size() or get_options ("rigid_log").size())
    linear_logstream.close();
}
