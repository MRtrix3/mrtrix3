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
#include "registration/nonlinear.h"
#include "registration/shared.h"
#include "registration/metric/demons.h"
#include "registration/metric/demons_cc.h"
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

  SYNOPSIS = "Register two images together using a symmetric non-linear transformation model";

  DESCRIPTION
      + "FOD registration (with apodised point spread reorientation) will be performed if the number of volumes "
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
    + Argument ("image1 image2", "input image 1 and input image 2").type_image_in()
    + Argument ("+ contrast1 contrast2", "optional list of additional input images used as additional contrasts. "
      "Can be used multiple times. contrastX and imageX must share the same coordinate system. ").type_image_in().optional().allow_multiple();

  OPTIONS
  + Option ("affine", "input affine transformation")
    + Argument ("filename").type_file_in ()

  + Option ("mask1", "a mask to define the region of image1 to use for optimisation.")
    + Argument ("filename").type_image_in ()

  + Option ("mask2", "a mask to define the region of image2 to use for optimisation.")
    + Argument ("filename").type_image_in ()

  + Registration::nonlinear_options

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

  bool do_nonlinear = true;

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

  Registration::Transform::Affine affine;
  opt = get_options ("affine");
  bool init_affine_matrix_set = false;
  if (opt.size()) {
    init_affine_matrix_set = true;
    transform_type init_affine = load_transform (opt[0][0]);
    affine.set_transform (init_affine);
  }

  // ****** REGISTRATION OPTIONS *******
  Registration::NonLinear nl_registration;
  opt = get_options ("nl_warp");
  std::string warp1_filename;
  std::string warp2_filename;
  if (opt.size()) {
    warp1_filename = std::string (opt[0][0]);
    warp2_filename = std::string (opt[0][1]);
  }

  opt = get_options ("nl_warp_full");
  std::string warp_full_filename;
  if (opt.size()) {
    warp_full_filename = std::string (opt[0][0]);
  }


  opt = get_options ("nl_init");
  bool nonlinear_init = false;
  if (opt.size()) {
    nonlinear_init = true;

    Image<default_type> input_warps = Image<default_type>::open (opt[0][0]);
    if (input_warps.ndim() != 5)
      throw Exception ("non-linear initialisation input is not 5D. Input must be from previous non-linear output");

    nl_registration.initialise (input_warps);

    if (init_affine_matrix_set)
      WARN ("-affine_init has no effect since the non-linear init warp also contains the linear transform in the image header");
  }


  opt = get_options ("nl_scale");
  if (opt.size ()) {
    vector<default_type> scale_factors = parse_floats (opt[0][0]);
    if (nonlinear_init) {
      WARN ("-nl_scale option ignored since only the full resolution will be performed when initialising with non-linear warp");
    } else {
      nl_registration.set_scale_factor (scale_factors);
    }
  }

  opt = get_options ("nl_niter");
  if (opt.size ()) {
    vector<int> iterations_per_level = parse_ints (opt[0][0]);
    if (nonlinear_init && iterations_per_level.size() > 1)
      throw Exception ("when initialising the non-linear registration the max number of iterations can only be defined for a single level");
    else
      nl_registration.set_max_iter (iterations_per_level);
  }
  opt = get_options ("cc");
  if (opt.size()) {
    nl_registration.metric_cc((int)opt[0][0]);
  }

  opt = get_options ("nl_update_smooth");
  if (opt.size()) {
    nl_registration.set_update_smoothing (opt[0][0]);
  }

  opt = get_options ("nl_disp_smooth");
  if (opt.size()) {
    nl_registration.set_disp_smoothing (opt[0][0]);
  }

  opt = get_options ("nl_grad_step");
  if (opt.size()) {
    nl_registration.set_init_grad_step (opt[0][0]);
  }

  opt = get_options ("diagnostics_image");
  if (opt.size()) {
    nl_registration.set_diagnostics_image (opt[0][0]);
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
      max_requested_lmax = std::max(max_requested_lmax, nl_registration.get_lmax());
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
    nl_registration.set_mc_parameters (mc_params);
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

  // affine.set_centre (rigid.get_centre());
  // affine.set_transform (rigid.get_transform());
  // affine_registration.set_init_translation_type (Registration::Transform::Init::none);


  // ****** RUN NON-LINEAR REGISTRATION *******
  CONSOLE ("running non-linear registration");

  if (do_reorientation)
    nl_registration.set_aPSF_directions (directions_cartesian);

  if (init_affine_matrix_set) {
    nl_registration.run (affine, images1, images2, im1_mask, im2_mask);
  } else {
    Registration::Transform::Affine identity_transform;
    nl_registration.run (identity_transform, images1, images2, im1_mask, im2_mask);
  }
  if (warp_full_filename.size()) {
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
