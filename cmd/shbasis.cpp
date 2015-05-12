/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 06/02/14.

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


#include <vector>

#include <gsl/gsl_fit.h>

#include "app.h"
#include "bitset.h"
#include "command.h"
#include "datatype.h"
#include "progressbar.h"
#include "memory.h"

#include "image/buffer.h"
#include "image/buffer_scratch.h"
#include "image/header.h"
#include "image/loop.h"
#include "image/utils.h"
#include "image/voxel.h"

#include "math/SH.h"


using namespace MR;
using namespace App;


const char* conversions[] = { "old", "new", "native", "force_oldtonew", "force_newtoold", NULL };
enum conv_t { NONE, OLD, NEW, FORCE_OLDTONEW, FORCE_NEWTOOLD };


void usage ()
{

  AUTHOR = "Robert E. Smith (r.smith@brain.org.au)";


  DESCRIPTION
    + "examine the values in spherical harmonic images to estimate (and optionally change) the SH basis used."

    + "In previous versions of MRtrix, the convention used for storing spherical harmonic "
      "coefficients was a non-orthonormal basis (the m!=0 coefficients were a factor of "
      "sqrt(2) too large). This error has been rectified in the new MRtrix (assuming that "
      "compilation was performed without the USE_NON_ORTHONORMAL_SH_BASIS symbol defined), "
      "but will cause issues if processing SH data that was generated using an older version "
      "of MRtrix (or vice-versa)."

    + "This command provides a mechanism for testing the basis used in storage of image data "
      "representing a spherical harmonic series per voxel, and allows the user to forcibly "
      "modify the raw image data to conform to the desired basis.";


  ARGUMENTS
    + Argument ("SH", "the input image(s) of SH coefficients.").allow_multiple().type_image_in();


  OPTIONS
    + Option ("convert", "convert the image data in-place to the desired basis (if necessary). "
                         "Options are: old, new, native (whichever basis MRtrix is compiled for; "
                         "most likely the new orthonormal basis), force_oldtonew, force_newtoold. "
                         "Note that for the \"force_*\" choices should ideally only be used in "
                         "cases where the command is unable to automatically determine the SH basis "
                         "using the existing image data.")
      + Argument ("mode").type_choice (conversions);

}





// Perform a linear regression on the power ratio in each order
// Omit l=2 - tends to be abnormally small due to non-isotropic brain-wide fibre distribution
// Use this to project the power ratio at either l=0 or l=lmax (depending on the gradient);
//   this has proven to be a better predictor of SH basis for poor data
// Also, if the regression has a substantial gradient, warn the user
// Threshold on gradient will depend on the basis of the image
//
std::pair<float, float> get_regression (const std::vector<float>& ratios)
{
  const size_t n = ratios.size() - 1;
  double x[n], y[n];
  for (size_t i = 1; i != ratios.size(); ++i) {
    x[i-1] = (2*i)+2;
    y[i-1] = ratios[i];
  }
  double c0, c1, cov00, cov01, cov11, sumsq;
  gsl_fit_linear (x, 1, y, 1, n, &c0, &c1, &cov00, &cov01, &cov11, &sumsq);
  return std::make_pair (c0, c1);
}





template <typename value_type>
void check_and_update (Image::Header& H, const conv_t conversion)
{

  const size_t lmax = Math::SH::LforN (H.dim(3));

  // Flag which volumes are m==0 and which are not
  const ssize_t N = H.dim(3);
  BitSet mzero_terms (N, false);
  for (size_t l = 2; l <= lmax; l += 2)
    mzero_terms[Math::SH::index (l, 0)] = true;

  // Open in read-write mode if there's a chance of modification
  typename Image::Buffer<value_type> buffer (H, (conversion != NONE));
  auto v = buffer.voxel();

  // Need to mask out voxels where the DC term is zero
  Image::Info info_mask (H);
  info_mask.set_ndim (3);
  info_mask.datatype() = DataType::Bit;
  Image::BufferScratch<bool> mask (info_mask);
  auto v_mask = mask.voxel();
  size_t voxel_count = 0;
  {
    Image::LoopInOrder loop (v, "Masking image based on DC term...", 0, 3);
    for (auto i = loop (v, v_mask); i; ++i) {
      const value_type value = v.value();
      if (value && std::isfinite (value)) {
        v_mask.value() = true;
        ++voxel_count;
      } else {
        v_mask.value() = false;
      }
    }
  }

  // Get sums independently for each l
 
  // Each order has a different power, and a different number of m!=0 volumes.
  // Therefore, calculate the mean-square intensity for the m==0 and m!=0
  // volumes independently, and report ratio for each harmonic order
  std::unique_ptr<ProgressBar> progress;
  if (App::log_level > 0 && App::log_level < 2)
    progress.reset (new ProgressBar ("Evaluating SH basis of image \"" + H.name() + "\"...", N-1));

  std::vector<float> ratios;

  for (size_t l = 2; l <= lmax; l += 2) {

    double mzero_sum = 0.0, mnonzero_sum = 0.0;
    Image::LoopInOrder loop (v, 0, 3);
    for (v[3] = ssize_t (Math::SH::NforL(l-2)); v[3] != ssize_t (Math::SH::NforL(l)); ++v[3]) {
      double sum = 0.0;
      for (auto i = loop (v, v_mask); i; ++i) {
        if (v_mask.value())
          sum += Math::pow2 (value_type(v.value()));
      }
      if (mzero_terms[v[3]]) {
        mzero_sum += sum;
        DEBUG ("Volume " + str(v[3]) + ", m==0, sum " + str(sum));
      } else {
        mnonzero_sum += sum;
        DEBUG ("Volume " + str(v[3]) + ", m!=0, sum " + str(sum));
      }
      if (progress)
      ++*progress;
    }

    const double mnonzero_MSoS = mnonzero_sum / (2.0 * l);
    const float power_ratio = mnonzero_MSoS/mzero_sum;
    ratios.push_back (power_ratio);

    INFO ("SH order " + str(l) + ", ratio of m!=0 to m==0 power: " + str(power_ratio) +
        ", m==0 power: " + str (mzero_sum));

  }

  if (progress)
    progress = NULL;

  // First is ratio to be used for SH basis decision, second is gradient of regression
  std::pair<float, float> regression = std::make_pair (0.0f, 0.0f);
  size_t l_for_decision;
  float power_ratio;

  // The gradient will change depending on the current basis, so the threshold needs to also
  // The gradient is as a function of l, not of even orders
  float grad_threshold = 0.02;

  switch (lmax) {

    // Lmax == 2: only one order to use
    case 2:
      power_ratio = ratios.front();
      l_for_decision = 2;
      break;

    // Lmax = 4: Use l=4 order to determine SH basis, can't check gradient since l=2 is untrustworthy
    case 4:
      power_ratio = ratios.back();
      l_for_decision = 4;
      break;

    // Lmax = 6: Use l=4 order to determine SH basis, but checking the gradient is not reliable:
    //   artificially double the threshold so the power ratio at l=6 needs to be substantially
    //   different to l=4 to throw a warning
    case 6:
      regression = std::make_pair (ratios[1] - 2*(ratios[2]-ratios[1]), 0.5*(ratios[2]-ratios[1]));
      power_ratio = ratios[1];
      l_for_decision = 4;
      grad_threshold *= 2.0;
      break;

    // Lmax >= 8: Do a linear regression from l=4 to l=lmax, project back to l=0
    // (this is a more reliable quantification on poor data than l=4 alone)
    default:
      regression = get_regression (ratios);
      power_ratio = regression.first;
      l_for_decision = 0;
      break;

  }

  // If the gradient is in fact positive (i.e. power ration increases for larger l), use the
  //   regression to pull the power ratio from l=lmax
  if (regression.second > 0.0) {
    l_for_decision = lmax;
    power_ratio = regression.first + (lmax * regression.second);
  }

  DEBUG ("Power ratio for assessing SH basis is " + str(power_ratio) + " as " + (lmax < 8 ? "derived from" : "regressed to") + " l=" + str(l_for_decision));

  // Threshold to make decision on what basis the data are currently stored in
  value_type multiplier = 1.0;
  if ((power_ratio > (5.0/3.0)) && (power_ratio < (7.0/3.0))) {

    CONSOLE ("Image \"" + str(H.name()) + "\" appears to be in the old non-orthonormal basis");
    switch (conversion) {
      case NONE: break;
      case OLD: break;
      case NEW: multiplier = Math::sqrt1_2; break;
      case FORCE_OLDTONEW: multiplier = Math::sqrt1_2; break;
      case FORCE_NEWTOOLD: WARN ("Refusing to convert image \"" + H.name() + "\" from new to old basis, as data appear to already be in the old non-orthonormal basis"); return;
    }
    grad_threshold *= 2.0;

  } else if ((power_ratio > (2.0/3.0)) && (power_ratio < (4.0/3.0))) {

    CONSOLE ("Image \"" + str(H.name()) + "\" appears to be in the new orthonormal basis");
    switch (conversion) {
      case NONE: break;
      case OLD: multiplier = Math::sqrt2; break;
      case NEW: break;
      case FORCE_OLDTONEW: WARN ("Refusing to convert image \"" + H.name() + "\" from old to new basis, as data appear to already be in the new orthonormal basis"); return;
      case FORCE_NEWTOOLD: multiplier = Math::sqrt2; break;
    }

  } else {

    multiplier = 0.0;
    WARN ("Cannot make unambiguous decision on SH basis of image \"" + H.name()
        + "\" (power ratio " + (lmax < 8 ? "in" : "regressed to") + " " + str(l_for_decision) + " is " + str(power_ratio) + ")");

    if (conversion == FORCE_OLDTONEW) {
      WARN ("Forcing conversion of image \"" + H.name() + "\" from old to new SH basis on user request; however NO GUARANTEE IS PROVIDED on appropriateness of this conversion!");
      multiplier = Math::sqrt1_2;
    } else if (conversion == FORCE_NEWTOOLD) {
      WARN ("Forcing conversion of image \"" + H.name() + "\" from new to old SH basis on user request; however NO GUARANTEE IS PROVIDED on appropriateness of this conversion!");
      multiplier = Math::sqrt2;
    }

  }

  // Decide whether the user needs to be warned about a poor diffusion encoding scheme
  if (regression.second)
    DEBUG ("Gradient of regression is " + str(regression.second) + "; threshold is " + str(grad_threshold));
  if (std::abs(regression.second) > grad_threshold) {
    WARN ("Image \"" + H.name() + "\" may have been derived from poor directional encoding, or have some other underlying data problem");
    WARN ("(m!=0 to m==0 power ratio changing by " + str(2.0*regression.second) + " per even order)");
  }

  // Adjust the image data in-place if necessary
  if (multiplier && (multiplier != 1.0)) {

    Image::LoopInOrder loop (v, 0, 3);
    ProgressBar progress ("Modifying SH basis of image \"" + H.name() + "\"...", N-1);
    for (v[3] = 1; v[3] != N; ++v[3]) {
      if (!mzero_terms[v[3]]) {
        for (auto i = loop (v); i; ++i) 
          v.value() *= multiplier;
      }
      ++progress;
    }

  } else if (multiplier && (conversion != NONE)) {
    INFO ("Image \"" + H.name() + "\" already in desired basis; nothing to do");
  }

}









void run ()
{
  conv_t conversion = NONE;
  Options opt = get_options ("convert");
  if (opt.size()) {
    switch (int(opt[0][0])) {
      case 0: conversion = OLD; break;
      case 1: conversion = NEW; break;
      case 2:
#ifndef USE_NON_ORTHONORMAL_SH_BASIS
        conversion = NEW;
#else
        conversion = OLD;
#endif
        break;
      case 3: conversion = FORCE_OLDTONEW; break;
      case 4: conversion = FORCE_NEWTOOLD; break;
      default: assert (0); break;
    }
  }

  for (std::vector<ParsedArgument>::const_iterator i = argument.begin(); i != argument.end(); ++i) {

    const std::string path = *i;
    Image::Header H (path);
    try {
      Math::SH::check (H);
    }
    catch (Exception& E) {
      E.display(0);
      continue;
    }

    if (H.datatype().bytes() == 4)
      check_and_update<float>  (H, conversion);
    else
      check_and_update<double> (H, conversion);

  }

};

