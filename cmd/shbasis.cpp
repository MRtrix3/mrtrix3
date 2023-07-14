/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "app.h"
#include "command.h"
#include "datatype.h"
#include "header.h"
#include "image.h"
#include "memory.h"
#include "progressbar.h"
#include "types.h"

#include "algo/loop.h"
#include "math/SH.h"
#include "misc/bitset.h"


using namespace MR;
using namespace App;


const char* conversions[] = { "old", "new", "force_oldtonew", "force_newtoold", nullptr };
enum conv_t { NONE, OLD, NEW, FORCE_OLDTONEW, FORCE_NEWTOOLD };


void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Examine the values in spherical harmonic images to estimate (and optionally change) the SH basis used";

  DESCRIPTION
    + "In previous versions of MRtrix, the convention used for storing spherical harmonic "
      "coefficients was a non-orthonormal basis (the m!=0 coefficients were a factor of "
      "sqrt(2) too large). This error has been rectified in newer versions of MRtrix, "
      "but will cause issues if processing SH data that was generated using an older version "
      "of MRtrix (or vice-versa)."

    + "This command provides a mechanism for testing the basis used in storage of image data "
      "representing a spherical harmonic series per voxel, and allows the user to forcibly "
      "modify the raw image data to conform to the desired basis."

    + "Note that the \"force_*\" conversion choices should only be used in cases where this "
      "command has previously been unable to automatically determine the SH basis from the "
      "image data, but the user themselves are confident of the SH basis of the data."

    + Math::SH::encoding_description;


  ARGUMENTS
    + Argument ("SH", "the input image(s) of SH coefficients.").allow_multiple().type_image_in();


  OPTIONS
    + Option ("convert", "convert the image data in-place to the desired basis; "
                         "options are: " + join(conversions, ",") + ".")
      + Argument ("mode").type_choice (conversions);

}





// Perform a linear regression on the power ratio in each order
// Omit l=2 - tends to be abnormally small due to non-isotropic brain-wide fibre distribution
std::pair<float, float> get_regression (const vector<float>& ratios)
{
  const size_t n = ratios.size() - 1;
  Eigen::VectorXf Y (n), b (2);
  Eigen::MatrixXf A (n, 2);
  for (size_t i = 1; i != ratios.size(); ++i) {
    Y[i-1] = ratios[i];
    A(i-1,0) = 1.0f;
    A(i-1,1) = (2*i)+2;
  }
  b = (A.transpose() * A).ldlt().solve (A.transpose() * Y);
  return std::make_pair (b[0], b[1]);
}





template <typename value_type>
void check_and_update (Header& H, const conv_t conversion)
{

  const size_t N = H.size(3);
  const size_t lmax = Math::SH::LforN (N);

  // Flag which volumes are m==0 and which are not
  BitSet mzero_terms (N, false);
  for (size_t l = 2; l <= lmax; l += 2)
    mzero_terms[Math::SH::index (l, 0)] = true;

  // Open in read-write mode if there's a chance of modification
  auto image = H.get_image<value_type> (true);

  // Need to mask out voxels where the DC term is zero
  Header header_mask (H);
  header_mask.ndim() = 3;
  header_mask.datatype() = DataType::Bit;
  auto mask = Image<bool>::scratch (header_mask);
  {
    for (auto i = Loop ("Masking image based on DC term", image, 0, 3) (image, mask); i; ++i) {
      const value_type value = image.value();
      if (value && std::isfinite (value)) {
        mask.value() = true;
      } else {
        mask.value() = false;
      }
    }
  }

  // Get sums independently for each l

  // Each order has a different power, and a different number of m!=0 volumes.
  // Therefore, calculate the mean-square intensity for the m==0 and m!=0
  // volumes independently, and report ratio for each harmonic order
  std::unique_ptr<ProgressBar> progress;
  if (App::log_level > 0 && App::log_level < 2)
    progress.reset (new ProgressBar ("Evaluating SH basis of image \"" + H.name() + "\"", N-1));

  vector<float> ratios;

  for (size_t l = 2; l <= lmax; l += 2) {

    double mzero_sum = 0.0, mnonzero_sum = 0.0;
    for (image.index(3) = ssize_t (Math::SH::NforL(l-2)); image.index(3) != ssize_t (Math::SH::NforL(l)); ++image.index(3)) {
      double sum = 0.0;
      for (auto i = Loop (image, 0, 3) (image, mask); i; ++i) {
        if (mask.value())
          sum += Math::pow2 (value_type(image.value()));
      }
      if (mzero_terms[image.index(3)]) {
        mzero_sum += sum;
        DEBUG ("Volume " + str(image.index(3)) + ", m==0, sum " + str(sum));
      } else {
        mnonzero_sum += sum;
        DEBUG ("Volume " + str(image.index(3)) + ", m!=0, sum " + str(sum));
      }
      if (progress)
      ++*progress;
    }

    const double mnonzero_MSoS = mnonzero_sum / (2.0 * l);
    const float power_ratio = mnonzero_MSoS / mzero_sum;
    ratios.push_back (power_ratio);

    INFO ("SH order " + str(l) + ", ratio of m!=0 to m==0 power: " + str(power_ratio) +
        ", m==0 power: " + str (mzero_sum));

  }

  if (progress)
    progress.reset (nullptr);

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
  if (abs(regression.second) > grad_threshold) {
    WARN ("Image \"" + H.name() + "\" may have been derived from poor directional encoding, or have some other underlying data problem");
    WARN ("(m!=0 to m==0 power ratio changing by " + str(2.0*regression.second) + " per even order)");
  }

  // Adjust the image data in-place if necessary
  if (multiplier && (multiplier != 1.0)) {

    ProgressBar progress ("Modifying SH basis of image \"" + H.name() + "\"", N-1);
    for (image.index(3) = 1; image.index(3) != ssize_t(N); ++image.index(3)) {
      if (!mzero_terms[image.index(3)]) {
        for (auto i = Loop (image, 0, 3) (image); i; ++i)
          image.value() *= multiplier;
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
  auto opt = get_options ("convert");
  if (opt.size()) {
    switch (int(opt[0][0])) {
      case 0: conversion = OLD; break;
      case 1: conversion = NEW; break;
      case 2: conversion = FORCE_OLDTONEW; break;
      case 3: conversion = FORCE_NEWTOOLD; break;
      default: assert (0); break;
    }
  }

  for (vector<ParsedArgument>::const_iterator i = argument.begin(); i != argument.end(); ++i) {

    const std::string path = *i;
    Header H = Header::open (path);
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

}

