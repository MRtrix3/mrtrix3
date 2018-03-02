/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "algo/histogram.h"

namespace MR
{
  namespace Algo
  {
    namespace Histogram
    {


      using namespace App;

      const OptionGroup Options = OptionGroup ("Histogram generation options")

      + Option ("bins", "Manually set the number of bins to use to generate the histogram.")
      + Argument ("num").type_integer (2)

      + Option ("template", "Use an existing histogram file as the template for histogram formation")
      + Argument ("file").type_file_in()

      + Option ("mask", "Calculate the histogram only within a mask image.")
      + Argument ("image").type_image_in()

      + Option ("ignorezero", "ignore zero-valued data during histogram construction.");




      void Calibrator::from_file (const std::string& path)
      {
        Eigen::MatrixXd M;
        try {
          M = load_matrix (path);
          if (M.cols() == 1)
            throw Exception ("Histogram template must have at least 2 columns");
          vector<default_type>().swap (data);
          auto V = M.row(0);
          num_bins = V.size();
          bin_width = (V[num_bins-1] - V[0]) / default_type(num_bins-1);
          min = V[0] - (0.5 * bin_width);
          max = V[num_bins-1] + (0.5 * bin_width);
          for (size_t i = 0; i != num_bins; ++i) {
            if (std::abs (get_bin_centre(i) - V[i]) > 1e-5)
              throw Exception ("Non-equal spacing in histogram bin centres");
          }
        } catch (Exception& e) {
          throw Exception (e, "Could not use file \"" + path + "\" as histogram template");
        }

      }



      void Calibrator::finalize (const size_t num_volumes, const bool is_integer)
      {
        if (!std::isfinite (bin_width)) {
          if (num_bins) {
            bin_width = (max - min) / default_type(num_bins);
          } else {
            // Freedman-Diaconis rule for selecting bin size for a histogram
            // Sometimes data from multiple volumes are used for calibration, but
            //   histograms are generated for individual volumes
            // Need to adjust the bin width accordingly... kinda ugly hack
            // Will need to revisit if mrstats gets capability to compute statistics across all volumes rather than splitting
            bin_width = 2.0 * get_iqr() * std::pow(static_cast<default_type>(data.size() / num_volumes), -1.0/3.0);
            vector<default_type>().swap (data); // No longer required; free the memory used
            // If the input data are integers, the bin width should also be an integer, to avoid getting
            //   regular spike artifacts in the histogram
            if (is_integer) {
              bin_width = std::round (bin_width);
              num_bins = std::ceil ((max - min) / bin_width);
            } else {
              // Now set the number of bins, and recalculate the bin width, to ensure
              //   evenly-spaced bins from min to max
              num_bins = std::round ((max - min) / get_bin_width());
              bin_width = (max - min) / default_type(num_bins);
            }
          }
        }
      }



      default_type Calibrator::get_iqr() {
        assert (data.size());
        const size_t lower_index = std::round (0.25*data.size());
        std::nth_element (data.begin(), data.begin() + lower_index, data.end());
        const default_type lower = data[data.size()/4];
        const size_t upper_index = std::round (0.75*data.size());
        std::nth_element (data.begin(), data.begin() + upper_index, data.end());
        const default_type upper = data[upper_index];
        return (upper - lower);
      }






      Data::cdf_type Data::cdf() const
      {
        cdf_type result (list.size());
        size_t count = 0;
        for (size_t i = 0; i != size_t(list.size()); ++i) {
          count += list[i];
          result[i] = count;
        }
        result /= count;
        return result;
      }



      default_type Data::first_min () const
      {
        ssize_t p1 = 0;
        while (list[p1] <= list[p1+1] && p1+2 < list.size())
          ++p1;
        for (ssize_t p = p1; p < list.size(); ++p) {
          if (2*list[p] < list[p1])
            break;
          if (list[p] >= list[p1])
            p1 = p;
        }

        ssize_t m1 (p1+1);
        while (list[m1] >= list[m1+1] && m1+2 < list.size())
          ++m1;
        for (ssize_t m = m1; m < list.size(); ++m) {
          if (list[m] > 2*list[m1])
            break;
          if (list[m] <= list[m1])
            m1 = m;
        }

        return info.get_min() + (info.get_bin_width() * (m1 + 0.5));
      }



      default_type Data::entropy () const {
        size_t totalFrequency = 0;
        for (size_t i = 0; i < size_t(list.size()); i++)
          totalFrequency += list[i];
        default_type imageEntropy = 0;
        for (size_t i = 0; i < size_t(list.size()); i++){
          const default_type probability = static_cast<default_type>(list[i]) / static_cast<default_type>(totalFrequency);
          if (probability > 0.99 / totalFrequency)
            imageEntropy += -probability * log(probability);
        }
        return imageEntropy;
      }






      Matcher::Matcher (const Data& input, const Data& target) :
          calib_input  (input .get_calibration()),
          calib_target (target.get_calibration())
      {
        // Need to have the CDF for each of the two histograms
        const auto cdf_input  = input.cdf();
        const auto cdf_target = target.cdf();

        // Each index in the input CDF needs to map to a (floating-point) index in the target CDF
        //   (linearly approximate the index that would result in the same value in the target CDF)
        mapping = vector_type::Zero (cdf_input.size() + 1);
        size_t upper_target_index = 1;
        for (size_t input_index = 1; input_index != size_t(cdf_input.size()); ++input_index) {
          while (upper_target_index < size_t(cdf_target.size()) && cdf_target[upper_target_index] < cdf_input[input_index])
            ++upper_target_index;
          const size_t lower_target_index = upper_target_index - 1;
          const default_type mu = (cdf_input[input_index] - cdf_target[lower_target_index]) / (cdf_target[upper_target_index] - cdf_target[lower_target_index]);
          mapping[input_index] = lower_target_index + mu;
        }
      }



      default_type Matcher::operator() (const default_type in) const
      {
        const default_type input_bin_float = (in - calib_input.get_min()) / calib_input.get_bin_width();
        default_type output_pos;
        if (input_bin_float < 0.0) {
          output_pos = 0.0;
        } else if (input_bin_float >= default_type(calib_input.get_num_bins())) {
          output_pos = default_type(calib_input.get_num_bins());
        } else {
          const size_t lower = std::floor (input_bin_float);
          const default_type mu = input_bin_float - lower;
          output_pos = ((1.0 - mu) * mapping[lower]) + (mu * mapping[lower+1]);
        }
        return calib_target.get_min() + (output_pos * calib_target.get_bin_width());
      }




    }
  }
}
