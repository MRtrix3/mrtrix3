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


#include <sstream>

#include "command.h"
#include "progressbar.h"
#include "algo/threaded_loop.h"
#include "image.h"
#include "math/sphere.h"
#include "math/SH.h"
#include "dwi/directions/predefined.h"
#include "filter/reslice.h"
#include "interp/cubic.h"

using namespace MR;
using namespace App;

#define DEFAULT_LUM_CR 0.3
#define DEFAULT_LUM_CG 0.5
#define DEFAULT_LUM_CB 0.2
#define DEFAULT_LUM_GAMMA 2.2

void usage ()
{
  AUTHOR = "Thijs Dhollander (thijs.dhollander@gmail.com)";

  COPYRIGHT =
    "Copyright (C) 2014 The Florey Institute of Neuroscience and Mental Health, Melbourne, Australia. "
    "This is free software; see the source for copying conditions. "
    "There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.";

  DESCRIPTION
    + "Generate FOD-based DEC maps, with optional panchromatic sharpening and/or luminance/perception correction."
    + "By default, the FOD-based DEC is weighted by the integral of the FOD. To weight by another scalar map, use the outputmap option. This option can also be used for panchromatic sharpening, e.g., by supplying a T1 (or other sensible) anatomical volume with a higher spatial resolution.";

  REFERENCES
    + "Dhollander T, Smith RE, Tournier JD, Jeurissen B, Connelly A. " // Internal
      "Time to move on: an FOD-based DEC map to replace DTI's trademark DEC FA. "
      "Proc Intl Soc Mag Reson Med, 2015, 23, 1027."
    + "Dhollander T, Raffelt D, Smith RE, Connelly A. " // Internal
      "Panchromatic sharpening of FOD-based DEC maps by structural T1 information. "
      "Proc Intl Soc Mag Reson Med, 2015, 23, 566.";

  ARGUMENTS
    + Argument ("input","The input FOD image (spherical harmonic coefficients).").type_image_in ()
    + Argument ("output","The output DEC image (weighted RGB triplets).").type_image_out ();

  OPTIONS
    + Option ("mask","Only perform DEC computation within the specified mask image.")
    + Argument ("image").type_image_in()

    + Option ("threshold","FOD amplitudes below the threshold value are considered zero.")
    + Argument ("value").type_float()

    + Option ("outputmap","Weight the computed DEC map by a provided outputmap. If the outputmap has a different grid, the DEC map is first resliced and renormalised. To achieve panchromatic sharpening, provide an image with a higher spatial resolution than the input FOD image; e.g., a T1 anatomical volume. Only the DEC is subject to the mask, so as to allow for partial colouring of the outputmap. \nDefault when this option is *not* provided: integral of input FOD, subject to the same mask/threshold as used for DEC computation.")
    + Argument ("image").type_image_in()

    + Option ("no-weight","Do not weight the DEC map (reslicing and renormalising still possible by explicitly providing the outputmap option as a template).")

    + Option ("lum","Correct for luminance/perception, using default values Cr,Cg,Cb = " + str(DEFAULT_LUM_CR, 2) + "," + str(DEFAULT_LUM_CG, 2) + "," + str(DEFAULT_LUM_CB, 2) + " and gamma = " + str(DEFAULT_LUM_GAMMA, 2) + " (*not* correcting is the theoretical equivalent of Cr,Cg,Cb = 1,1,1 and gamma = 2).")

    + Option ("lum-coefs","The coefficients Cr,Cg,Cb to correct for luminance/perception. \nNote: this implicitly switches on luminance/perception correction, using a default gamma = " + str(DEFAULT_LUM_GAMMA, 2) + " unless specified otherwise.")
    + Argument ("values").type_sequence_float()

    + Option ("lum-gamma","The gamma value to correct for luminance/perception. \nNote: this implicitly switches on luminance/perception correction, using a default Cr,Cg,Cb = " + str(DEFAULT_LUM_CR, 2) + "," + str(DEFAULT_LUM_CG, 2) + "," + str(DEFAULT_LUM_CB, 2) + " unless specified otherwise.")
    + Argument ("value").type_float();
}

typedef float value_type;
constexpr value_type UNIT = 0.577350269189626;

class DecTransform { MEMALIGN(DecTransform)

  public:

  Eigen::MatrixXd sht;
  Eigen::Matrix<double, Eigen::Dynamic, 3> decs;
  double thresh;

  DecTransform (int lmax, const Eigen::Matrix<double, Eigen::Dynamic, 2>& dirs, double thresh) :
    sht (Math::SH::init_transform(dirs, lmax)),
    decs (Math::Sphere::spherical2cartesian(dirs).cwiseAbs()),
    thresh (thresh) { }

};

class DecComputer { MEMALIGN(DecComputer)

  private:

  const DecTransform& dectrans;
  Image<bool> mask_img;
  Image<value_type> int_img;
  Eigen::VectorXd amps, fod;

  public:

  DecComputer (const DecTransform& dectrans, Image<bool>& mask_img, Image<value_type>& int_img) :
    dectrans (dectrans),
    mask_img (mask_img),
    int_img (int_img),
    amps (dectrans.sht.rows()),
    fod (dectrans.sht.cols()) { }

  void operator() (Image<value_type>& fod_img, Image<value_type>& dec_img) {

    if (mask_img.valid()) {
      assign_pos_of(fod_img, 0, 3).to(mask_img);
      if (!mask_img.value()) {
        dec_img.row(3) = UNIT;
        return;
      }
    }

    fod = fod_img.row(3);
    amps.noalias() = dectrans.sht * fod;

    Eigen::Vector3d dec = Eigen::Vector3d::Zero();
    double ampsum = 0.0;
    for (ssize_t i = 0; i < amps.rows(); i++) {
      if (!std::isnan(dectrans.thresh) && amps(i) < dectrans.thresh)
        continue;
      dec += dectrans.decs.row(i).transpose() * amps(i);
      ampsum += amps(i);
    }
    dec = dec.cwiseMax(0.0);
    ampsum = std::max(ampsum, 0.0);

    double decnorm = dec.norm();

    if (decnorm == 0.0)
      dec_img.row(3) = UNIT;
    else
      dec_img.row(3) = dec / decnorm;

    if (int_img.valid()) {
      assign_pos_of(fod_img, 0, 3).to(int_img);
      int_img.value() = (ampsum / amps.rows()) * 4.0 * Math::pi;
    }

  }

};

class DecWeighter { MEMALIGN(DecWeighter)

  private:

  Eigen::Array<value_type, 3, 1> coefs;
  value_type gamma;
  Image<value_type> w_img;
  value_type grey;

  public:

  DecWeighter (Eigen::Matrix<value_type, 3, 1> coefs, value_type gamma, Image<value_type>& w_img) :
    coefs (coefs),
    gamma (gamma),
    w_img (w_img),
    grey (1.0 / std::pow(coefs.sum(), 1.0 / gamma)) {}

  void operator() (Image<value_type>& dec_img) {

    value_type w = 1.0;
    if (w_img.valid()) {
      assign_pos_of(dec_img, 0, 3).to(w_img);
      w = w_img.value();
      if (w <= 0.0) {
        for (auto l = Loop (3) (dec_img); l; ++l)
          dec_img.value() = 0.0;
        return;
      }
    }

    Eigen::Array<value_type, 3, 1> dec;
    for (auto l = Loop (3) (dec_img); l; ++l)
      dec[dec_img.index(3)] = dec_img.value();

    dec = dec.cwiseMax(0.0);

    value_type br = std::pow((dec.pow(gamma) * coefs).sum() , 1.0 / gamma);

    if (br == 0.0)
      dec.fill(grey * w);
    else
      dec = dec * (w / br);

    for (auto l = Loop (3) (dec_img); l; ++l)
       dec_img.value() = dec[dec_img.index(3)];

  }

};

void run () {

  auto fod_hdr = Header::open(argument[0]);
  Math::SH::check(fod_hdr);

  auto mask_hdr = Header();
  auto optm = get_options("mask");
  if (optm.size()) {
    mask_hdr = Header::open(optm[0][0]);
    check_dimensions (mask_hdr, fod_hdr, 0, 3);
  }

  float thresh = get_option_value ("threshold", NAN);

  bool needtolum = false;
  Eigen::Array<value_type, 3, 1> coefs (1.0, 1.0, 1.0);
  value_type gamma = 2.0;
  auto optlc = get_options("lum-coefs");
  auto optlg = get_options("lum-gamma");
  if (get_options("lum").size() || optlc.size() || optlg.size()) {
    needtolum = true;
    coefs << DEFAULT_LUM_CR , DEFAULT_LUM_CG , DEFAULT_LUM_CB; gamma = DEFAULT_LUM_GAMMA;
    if (optlc.size()) {
      auto lc = parse_floats(optlc[0][0]);
      if (lc.size() != 3)
        throw Exception ("expecting exactly 3 coefficients for the lum-coefs option, provided as a comma-separated list Cr,Cg,Cb ; e.g., " + str(DEFAULT_LUM_CR, 2) + "," + str(DEFAULT_LUM_CG, 2) + "," + str(DEFAULT_LUM_CB, 2) + "");
      coefs(0) = lc[0]; coefs(1) = lc[1]; coefs(2) = lc[2];
    }
    if (optlg.size())
      gamma = optlg[0][0];
  }

  bool needtoslice = false;
  auto map_hdr = Header();
  auto opto = get_options ("outputmap");
  if (opto.size()) {
    map_hdr = Header::open(opto[0][0]);
    if (!dimensions_match(map_hdr, fod_hdr, 0, 3) ||
        !spacings_match(map_hdr, fod_hdr, 0, 3) ||
        !map_hdr.transform().isApprox(map_hdr.transform(),1e-42))
      needtoslice = true;
  }

  auto out_img = Image<value_type>();
  auto w_img = Image<value_type>();

  {
    auto dec_img = Image<value_type>();

    {
      auto fod_img = fod_hdr.get_image<value_type>().with_direct_io(3);

      auto dec_hdr = Header(fod_img);
      dec_hdr.ndim() = 4;
      dec_hdr.size(3) = 3;
      Stride::set (dec_hdr, Stride::contiguous_along_axis (3, dec_hdr));
      dec_img = Image<value_type>::scratch(dec_hdr,"DEC map");

      Eigen::Matrix<double, 1281, 2> dirs = DWI::Directions::tesselation_1281();

      auto mask_img = Image<bool>();
      if (mask_hdr.valid())
        mask_img = mask_hdr.get_image<bool>();

      if (!get_options("no-weight").size() && !map_hdr) {
        auto int_hdr = Header(dec_img);
        int_hdr.size(3) = 1;
        w_img = Image<value_type>::scratch(int_hdr,"FOD integral map");
      }

      ThreadedLoop ("computing colours", fod_img, 0, 3)
        .run (DecComputer (DecTransform (Math::SH::LforN(fod_img.size(3)), dirs, thresh), mask_img, w_img), fod_img, dec_img);  
    }

    auto out_hdr = map_hdr.valid() ? Header(map_hdr) : Header(dec_img);
    out_hdr.datatype() = DataType::Float32;
    out_hdr.ndim() = 4;
    out_hdr.size(3) = 3;
    Stride::set (out_hdr, Stride::contiguous_along_axis (3, out_hdr));
    out_img = Image<value_type>::create(argument[1],out_hdr);

    if (needtoslice)
      Filter::reslice<Interp::Cubic> (dec_img, out_img, Adapter::NoTransform, Adapter::AutoOverSample, UNIT);
    else
      copy (dec_img, out_img);
  }

  if (!get_options("no-weight").size() && map_hdr.valid())
    w_img = map_hdr.get_image<value_type>();

  if (w_img.valid() || needtolum || needtoslice)
    ThreadedLoop ("(re)weighting", out_img, 0, 3, 2)
      .run (DecWeighter (coefs, gamma, w_img), out_img);

}

