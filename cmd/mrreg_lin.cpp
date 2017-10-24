/* Copyright (c) 2008-2017 the MRtrix3 contributors
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
#include "filter/reslice.h"
#include "interp/cubic.h"
#include "transform.h"
#include "registration/multi_contrast.h"
#include "registration/shared.h"
#include "registration/linear2.h"
#include "registration/metric/mean_squared.h"
#include "registration/metric/difference_robust.h"
#include "registration/metric/local_cross_correlation.h"
#include "registration/transform/affine.h"
#include "dwi/directions/predefined.h"
#include "math/average_space.h"
#include "math/SH.h"
#include "math/sphere.h"
#include "transform.h"


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au) & Max Pietsch (maximilian.pietsch@kcl.ac.uk)";

  SYNOPSIS = "Register two images together using a symmetric linear transformation model";

  DESCRIPTION
      + "By default this application will perform a symmetric affine registration."

      + "FOD registration (with apodised point spread reorientation) will be performed by default if the number of volumes "
        "in the 4th dimension equals the number of coefficients in an antipodally symmetric spherical harmonic series (e.g. 6, 15, 28 etc). "
        "The -no_reorientation option can be used to force reorientation off if required.";

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
  + Option ("mask1", "a mask to define the region of image1 to use for optimisation.")
    + Argument ("filename").type_image_in ()

  + Option ("mask2", "a mask to define the region of image2 to use for optimisation.")
    + Argument ("filename").type_image_in ()

  + Option ("nonsymmetric", "Use non-symmetric registration with fixed template image.")

  + Registration::affine_options

  + Registration::lin_stage_options

  + Registration::multiContrastOptions

  + Registration::fod_options

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
  }


  bool do_affine = true;

  // reorientation_forbidden required for output of transformed images because do_reorientation might change
  const bool reorientation_forbidden (get_options ("noreorientation").size());
  // do_reorientation == false --> registration without reorientation.
  // will be do_reorientation set to false if registration of all input SH images has lmax==0
  bool do_reorientation = !reorientation_forbidden;

  Eigen::MatrixXd directions_cartesian;
  auto opt = get_options ("directions");
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

  // Non-symmetric registration
  bool do_nonsymmetric = get_options("nonsymmetric").size();


  // ****** AFFINE REGISTRATION OPTIONS *******
  Registration::Linear affine_registration;

  affine_registration.use_nonsymmetric(do_nonsymmetric);

  opt = get_options ("affine");
  bool output_affine = false;
  std::string affine_filename;
  if (opt.size()) {
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
  // bool init_affine_matrix_set = false;
  if (opt.size()) {
  //   init_affine_matrix_set = true;
    Eigen::Vector3 centre;
    transform_type init_affine = load_transform (opt[0][0], centre);
    affine.set_transform (init_affine);
    affine.set_centre_without_transform_update (centre); // centre is NaN if not present in matrix file.
  }

  affine.use_nonsymmetric(do_nonsymmetric);


  //   affine_registration.set_init_translation_type (Registration::Transform::Init::set_centre_mass);
  // }

  // opt = get_options ("affine_init_translation");
  // if (opt.size()) {
  //   if (init_affine_matrix_set)
  //     throw Exception ("options -affine_init_matrix and -affine_init_translation are mutually exclusive");
  //   Registration::set_init_translation_model_from_option (affine_registration, (int)opt[0][0]);
  // }

  // opt = get_options ("affine_init_rotation");
  // if (opt.size()) {
  //   if (init_affine_matrix_set)
  //     throw Exception ("options -affine_init_matrix and -affine_init_rotation are mutually exclusive");
  //   Registration::set_init_rotation_model_from_option (affine_registration, (int)opt[0][0]);
  // }

  opt = get_options ("affine_scale");
  if (opt.size ()) {
    if (!do_affine)
      throw Exception ("the affine multi-resolution scale factors were input when no affine registration is requested");
    affine_registration.set_scale_factor (parse_floats (opt[0][0]));
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
        affine_metric = Registration::LCC;
        break;
      default:
        break;
    }
  }


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

  opt = get_options ("type");
  if (opt.size ()) {
    switch ((int) opt[0][0]) {
    case 0:
      affine_registration.set_transform_projector (Registration::Transform::TransformProjectionType::rigid_nonsym);
      break;
    case 1:
      affine_registration.set_transform_projector (Registration::Transform::TransformProjectionType::affine);
      break;
    case 2:
      affine_registration.set_transform_projector (Registration::Transform::TransformProjectionType::affine_nonsym);
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

  std::ofstream linear_logstream;
  opt = get_options ("affine_log");
  if (opt.size()) {
    linear_logstream.open (opt[0][0]);
    affine_registration.set_log_stream (linear_logstream.rdbuf());
  }


//  // ****** PARSE OPTIONS *******
//  for (auto& s: Registration::lin_stage_options) {
//    if (get_options(s.id).size()) {
//      std::stringstream msg;
//      msg << "cannot use option -" << s.id << " when no linear registration is requested";
//      throw Exception (msg.str());
//    }
//  }

  Registration::parse_general_options (affine_registration);


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

    for (size_t idx = 0; idx < n_images; idx++)
      mc_params[idx].weight = mc_weights[idx];
  }

  {
    ssize_t max_requested_lmax = 0;
    if (max_mc_image_lmax != 0) {
      max_requested_lmax = std::max(max_requested_lmax, affine_registration.get_lmax());
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

  if (mc_params.size() > 1)
    affine_registration.set_mc_parameters (mc_params);

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

  // ****** RUN AFFINE REGISTRATION *******
  CONSOLE ("running affine registration");

  if (images2.ndim() == 4) {
    if (do_reorientation)
      affine_registration.set_directions (directions_cartesian);
    if (affine_metric == Registration::LCC) {
      throw Exception ("TODO local CC for 4D"); // TODO
    } else if (affine_metric == Registration::Diff) {
      if (affine_estimator == Registration::None) {
        if (do_nonsymmetric) {
          Registration::Metric::MeanSquared4DNonSymmetric<Image<value_type>, Image<value_type>> metric;
          affine_registration.run_masked (metric, affine, images1, images2, im1_mask, im2_mask);
        } else {
          Registration::Metric::MeanSquared4D<Image<value_type>, Image<value_type>> metric;
          affine_registration.run_masked (metric, affine, images1, images2, im1_mask, im2_mask);
        }
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
    if (affine_metric == Registration::LCC){
      Registration::Metric::LocalCrossCorrelation metric;
      vector<size_t> extent(3,3);
      affine_registration.set_extent (extent);
      affine_registration.run_masked (metric, affine, images1, images2, im1_mask, im2_mask);
    }
    else if (affine_metric == Registration::Diff) {
      if (affine_estimator == Registration::None) {
        if (do_nonsymmetric) {
          Registration::Metric::MeanSquaredNonSymmetric metric;
          affine_registration.run_masked (metric, affine, images1, images2, im1_mask, im2_mask);
        } else {
          Registration::Metric::MeanSquared metric;
          affine_registration.run_masked (metric, affine, images1, images2, im1_mask, im2_mask);
        }
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

  if (get_options ("affine_log").size())
    linear_logstream.close();
}
