/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2013.

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

#include "command.h"
#include "args.h"
#include "exception.h"
#include "mrtrix.h"
#include "progressbar.h"

#include "image/buffer.h"
#include "image/buffer_scratch.h"
#include "image/copy.h"
#include "image/loop.h"
#include "image/position.h"
#include "image/threaded_loop.h"
#include "image/utils.h"
#include "image/value.h"

#include "math/math.h"
#include "math/SH.h"
#include "math/vector.h"

#include "thread/queue.h"

#include "dwi/directions/set.h"
#include "dwi/gradient.h"
#include "dwi/shells.h"
#include "dwi/sdeconv/constrained.h"
#include "dwi/sdeconv/rf_estimation.h"





using namespace MR;
using namespace MR::DWI;
using namespace MR::DWI::RF;
using namespace App;



#define DWI2RESPONSE_DEFAULT_MAX_ITERS 20
#define DWI2RESPONSE_DEFAULT_MAX_CHANGE 0.5

#define DWI2RESPONSE_DEFAULT_VOLUME_RATIO 0.15
#define DWI2RESPONSE_DEFAULT_DISPERSION_MULTIPLIER 1.0
#define DWI2RESPONSE_DEFAULT_INTEGRAL_STDEV_MULTIPLIER 2.0




const OptionGroup TermOption = OptionGroup ("Options for terminating the optimisation algorithm")

    + Option ("max_iters", "maximum number of iterations per pass (set to zero to disable)")
      + Argument ("value").type_integer (0, DWI2RESPONSE_DEFAULT_MAX_ITERS, 1e6)

    + Option ("max_change", "maximum percentile change in any response function coefficient; "
                            "if no individual coefficient changes by more than this fraction, the algorithm is terminated.")
      + Argument ("value").type_float (0.0, DWI2RESPONSE_DEFAULT_MAX_CHANGE, 100.0);




const OptionGroup SFOption = OptionGroup ("Thresholds for single-fibre voxel selection")

    + Option ("volume_ratio", "required volume ratio between largest FOD lobe and the sum of all other positive lobes in the voxel")
      + Argument ("value").type_float (0.0, DWI2RESPONSE_DEFAULT_VOLUME_RATIO, 1.0)

    + Option ("dispersion_multiplier", "dispersion of FOD lobe must not exceed some threshold as determined by this factor and FOD dispersion statistics. "
                                       "The threshold is: (mean + (multiplier * (mean - min))). "
                                       "Criterion is only applied in second pass of RF estimation.")
      + Argument ("value").type_float (0.0, DWI2RESPONSE_DEFAULT_DISPERSION_MULTIPLIER, 100.0)

    + Option ("integral_multiplier", "integral of FOD lobe must not be outside some range as determined by this factor and FOD lobe integral statistics. "
                                     "The range is: (mean +- (multiplier * stdev)). "
                                     "Criterion is only applied in second pass of RF estimation.")
      + Argument ("value").type_float (0.0, DWI2RESPONSE_DEFAULT_INTEGRAL_STDEV_MULTIPLIER, 1.0);





void usage () {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION 
    + "generate an appropriate response function from the image data for spherical deconvolution";

  REFERENCES = "Tax, C. M.; Jeurissen, B.; Vos, S. B.; Viergever, M. A. & Leemans, A. "
               "Recursive calibration of the fiber response function for spherical deconvolution of diffusion MRI data. "
               "NeuroImage, 2014, 86, 67-80";

  ARGUMENTS
    + Argument ("dwi_in",       "the input diffusion-weighted images").type_image_in()
    + Argument ("response_out", "the output rotational harmonic coefficients").type_file_out();

  OPTIONS

    + DWI::GradOption
    + DWI::ShellOption

    + Option ("mask", "provide an initial mask image")
      + Argument ("image").type_image_in()

    + Option ("lmax", "specify the maximum harmonic degree of the response function to estimate")
      + Argument ("value").type_integer (2, 8, 20)

    + Option ("sf", "output a mask highlighting the final selection of single-fibre voxels")
      + Argument ("image").type_image_out()

    + Option ("test_all", "by default, only those voxels selected as single-fibre in the previous iteration are evaluated. "
                          "Set this option to re-test all voxels at every iteration (slower).")

    + TermOption
    + SFOption;

  // TODO More options:
  // * Response function estimation method - simple averaging (a la 0.2) or Bayesian w. Rician noise correction
  //     Have a final pass after optimisation that estimates the final RF using the more advanced method
  //     (with or without Rician bias correction)
  //     (currently can just use this binary to derive the mask, then feed to get_response)

}





void run () 
{

  Image::Header H (argument[0]);

  Image::Info info (H);
  info.set_ndim (3);
  Image::BufferScratch<bool> mask (info);
  auto v_mask = mask.voxel();

  std::string mask_path;
  Options opt = get_options ("mask");
  if (opt.size()) {
    mask_path = std::string(opt[0][0]);
    Image::Buffer<bool> in (mask_path);
    if (!Image::dimensions_match (H, in, 0, 3))
      throw Exception ("Input mask image does not match DWI");
    if (!(in.ndim() == 3 || (in.ndim() == 4 && in.dim(3) == 1)))
      throw Exception ("Input mask image must be a 3D image");
    auto v_in = in.voxel();
    Image::copy (v_in, v_mask, 0, 3);
  } else {
    for (auto l = Image::LoopInOrder (v_mask) (v_mask); l; ++l) 
      v_mask.value() = true;
  }

  DWI::CSDeconv<float>::Shared shared (H);

  const size_t max_lmax = Math::SH::LforN (shared.dwis.size());
  size_t lmax = std::min (size_t(8), max_lmax);
  opt = get_options ("lmax");
  if (opt.size()) {
    const size_t desired_lmax = int(opt[0][0]);
    if (desired_lmax % 2)
      throw Exception ("lmax must be an even number");
    if (desired_lmax > max_lmax)
      throw Exception ("Image data does not support estimating response function above an lmax of " + str(max_lmax));
    lmax = desired_lmax;
  }
  shared.lmax = lmax;

  Image::Buffer<float> dwi (H);
  DWI::Directions::Set directions (1281);

  Math::Vector<float> response (lmax/2+1);
  response.zero();

  {
    // Initialise response function
    // Use lmax = 2, get the DWI intensity mean and standard deviation within the mask and
    //   use these as the first two coefficients
    auto v_dwi = dwi.voxel();
    double sum = 0.0, sq_sum = 0.0;
    size_t count = 0;
    Image::LoopInOrder loop (dwi, "initialising response function... ", 0, 3);
    for (auto l = loop (v_dwi, v_mask); l; ++l) {
      if (v_mask.value()) {
        for (size_t volume_index = 0; volume_index != shared.dwis.size(); ++volume_index) {
          v_dwi[3] = shared.dwis[volume_index];
          const float value = v_dwi.value();
          sum += value;
          sq_sum += Math::pow2 (value);
          ++count;
        }
      }
    }
    response[0] = sum / double (count);
    response[1] = - 0.5 * std::sqrt ((sq_sum / double(count)) - Math::pow2 (response[0]));
    // Account for scaling in SH basis
    response *= std::sqrt (4.0 * Math::pi);
  }
  INFO ("Initial response function is [" + str(response, 2) + "]");

  // Algorithm termination options
  opt = get_options ("max_iters");
  const size_t max_iters = opt.size() ? int(opt[0][0]) : DWI2RESPONSE_DEFAULT_MAX_ITERS;
  opt = get_options ("max_change");
  const float max_change = 0.01 * (opt.size() ? float(opt[0][0]) : DWI2RESPONSE_DEFAULT_MAX_CHANGE);

  // Should all voxels (potentially within a user-specified mask) be tested at every iteration?
  opt = get_options ("test_all");
  const bool reset_mask = opt.size();

  // Single-fibre voxel selection options
  opt = get_options ("volume_ratio");
  const float volume_ratio = opt.size() ? float(opt[0][0]) : DWI2RESPONSE_DEFAULT_VOLUME_RATIO;
  opt = get_options ("dispersion_multiplier");
  const float dispersion_multiplier = opt.size() ? float(opt[0][0]) : DWI2RESPONSE_DEFAULT_DISPERSION_MULTIPLIER;
  opt = get_options ("integral_multiplier");
  const float integral_multiplier = opt.size() ? float(opt[0][0]) : DWI2RESPONSE_DEFAULT_INTEGRAL_STDEV_MULTIPLIER;

  SFThresholds thresholds (volume_ratio); // Only threshold the lobe volume ratio for now; other two are not yet used

  size_t total_iter = 0;
  bool first_pass = true;
  size_t prev_sf_count = 0;
  {
    bool iterate = true;
    size_t iter = 0;
    ProgressBar progress ("optimising response function.. ");
    do {

      ++iter;

      {
        MR::LogLevelLatch latch (0);
        shared.set_response (response);
        shared.init();
      }

      ++progress;

      if (reset_mask) {
        if (mask_path.size()) {
          Image::Buffer<bool> in (mask_path);
          auto v_in = in.voxel();
          Image::copy (v_in, v_mask, 0, 3);
        } else {
          for (auto l = Image::LoopInOrder(v_mask) (v_mask); l; ++l)
            v_mask.value() = true;
        }
        ++progress;
      }

      std::vector<FODSegResult> seg_results;
      {
        FODCalcAndSeg processor (dwi, mask, shared, directions, lmax, seg_results);
        Image::ThreadedLoop loop (mask, 1, 0, 3);
        loop.run (processor);
      }

      ++progress;

      if (!first_pass)
        thresholds.update (seg_results, dispersion_multiplier, integral_multiplier, iter);

      ++progress;

      Response output (lmax);
      mask.zero();
      {
        SFSelector selector (seg_results, thresholds, mask);
        ResponseEstimator estimator (dwi, shared, lmax, output);
        Thread::run_queue (selector, FODSegResult(), Thread::multi (estimator));
      }
      if (!output.get_count())
        throw Exception ("Cannot estimate response function; all voxels have been excluded from selection");
      const Math::Vector<float> new_response = output.result();
      const size_t sf_count = output.get_count();

      ++progress;

      if (App::log_level >= 2)
        std::cerr << "\n";
      INFO ("Iteration " + str(iter) + ", " + str(sf_count) + " SF voxels, new response function: [" + str(new_response, 2) + "]");

      if (sf_count == prev_sf_count) {
        INFO ("terminating due to convergence of single-fibre voxel selection");
        iterate = false;
      }
      if (iter == max_iters) {
        INFO ("terminating due to completing maximum number of iterations");
        iterate = false;
      }
      bool rf_changed = false;
      for (size_t i = 0; i != response.size(); ++i) {
        if (std::abs ((new_response[i] - response[i]) / new_response[i]) > max_change)
          rf_changed = true;
      }
      if (!rf_changed) {
        INFO ("terminating due to negligible changes in the response function coefficients");
        iterate = false;
      }

      if (!iterate && first_pass) {
        iterate = true;
        first_pass = false;
        INFO ("commencing second-pass of response function estimation");
        total_iter = iter;
        iter = 0;
      }

      response = new_response;
      prev_sf_count = sf_count;

      //v_mask.save ("mask_pass_" + str(first_pass?1:2) + "_iter_" + str(iter) + ".mif");

    } while (iterate);

    total_iter += iter;

  }

  INFO ("final response function: [" + str(response, 2) + "] (reached after " + str(total_iter) + " iterations using " + str(prev_sf_count) + " voxels)");
  response.save (argument[1]);

  opt = get_options ("sf");
  if (opt.size())
    v_mask.save (std::string (opt[0][0]));

}

