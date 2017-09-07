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


#include "dwi/tractography/mapping/mapper_plugins.h"

#include "image_helpers.h"
#include "algo/threaded_loop.h"

namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Mapping {




        void TWIImagePluginBase::set_backtrack()
        {
          backtrack = true;
          if (statistic != ENDS_CORR)
            return;
          Header H (interp);
          H.ndim() = 3;
          H.datatype() = DataType::Bit;
          backtrack_mask = Image<bool>::scratch (H, "TWI back-tracking mask");
          Image<float> data (interp);
          auto f = [] (Image<float>& in, Image<bool>& mask) {
            for (in.index(3) = 0; in.index(3) != in.size(3); ++in.index(3)) {
              if (std::isfinite (in.value()) && in.value()) {
                mask.value() = true;
                return true;
              }
            }
            mask.value() = false;
            return true;
          };

          ThreadedLoop ("pre-calculating mask of valid time-series voxels", backtrack_mask)
              .run (f, data, backtrack_mask);
        }




        const ssize_t TWIImagePluginBase::get_end_index (const Streamline<>& tck, const bool end) const
        {
          ssize_t index = end ? tck.size() - 1 : 0;
          if (backtrack) {

            const ssize_t step = end ? -1 : 1;

            if (statistic == ENDS_CORR) {

              for (; index >= 0 && index < ssize_t(tck.size()); index += step) {
                const Eigen::Vector3 p = interp.scanner2voxel * tck[index].cast<default_type>();
                const Eigen::Array3i v ( { std::round (p[0]), std::round (p[1]), std::round (p[2]) } );
                if (!is_out_of_bounds (backtrack_mask, v)) {
                  assign_pos_of (v, 0, 3).to (backtrack_mask);
                  if (backtrack_mask.value())
                    return index;
                }
              }
              return -1;

            } else {

              while (!interp.scanner (tck[index])) {
                index += step;
                if (index == -1 || index == ssize_t(tck.size()))
                  return -1;
              }

            }

          } else {
            if (!interp.scanner (tck[index]))
              return -1;
          }
          return index;
        }

        const Streamline<>::point_type TWIImagePluginBase::get_end_point (const Streamline<>& tck, const bool end) const
        {
          const ssize_t index = get_end_index (tck, end);
          if (index == -1)
            return { NaN, NaN, NaN };
          return tck[index];
        }








        void TWIScalarImagePlugin::load_factors (const Streamline<>& tck, vector<default_type>& factors) const
        {
          if (statistic == ENDS_MIN || statistic == ENDS_MEAN || statistic == ENDS_MAX || statistic == ENDS_PROD) {

            // Only the track endpoints contribute
            for (size_t tck_end_index = 0; tck_end_index != 2; ++tck_end_index) {
              const ssize_t index = get_end_index (tck, tck_end_index);
              if (index >= 0) {
                if (interp.scanner (tck[index]))
                  factors.push_back (interp.value());
                else
                  factors.push_back (NaN);
              } else {
                factors.push_back (NaN);
              }
            }

          } else {

            // The entire length of the track contributes
            for (const auto& i : tck) {
              if (interp.scanner (i))
                factors.push_back (interp.value());
              else
                factors.push_back (NaN);
            }

          }
        }





        void TWIFODImagePlugin::load_factors (const Streamline<>& tck, vector<default_type>& factors) const
        {
          assert (statistic != ENDS_CORR);
          if (statistic == ENDS_MAX || statistic == ENDS_MEAN || statistic == ENDS_MIN || statistic == ENDS_PROD) {

            for (size_t tck_end_index = 0; tck_end_index != 2; ++tck_end_index) {
              const ssize_t index = get_end_index (tck, tck_end_index);
              if (index > 0) {
                if (interp.scanner (tck[index])) {
                  for (interp.index(3) = 0; interp.index(3) != interp.size(3); ++interp.index(3))
                    sh_coeffs[interp.index(3)] = interp.value();
                  const Eigen::Vector3 dir = (tck[(index == ssize_t(tck.size()-1)) ? index : (index+1)] - tck[index ? (index-1) : 0]).cast<default_type>().normalized();
                  factors.push_back (precomputer->value (sh_coeffs, dir));
                } else {
                  factors.push_back (NaN);
                }
              } else {
                factors.push_back (NaN);
              }
            }

          } else {

            for (size_t i = 0; i != tck.size(); ++i) {
              if (interp.scanner (tck[i])) {
                // Get the FOD at this (interploated) point
                for (interp.index(3) = 0; interp.index(3) != interp.size(3); ++interp.index(3))
                  sh_coeffs[interp.index(3)] = interp.value();
                // Get the FOD amplitude along the streamline tangent
                const Eigen::Vector3 dir = (tck[(i == tck.size()-1) ? i : (i+1)] - tck[i ? (i-1) : 0]).cast<default_type>().normalized();
                factors.push_back (precomputer->value (sh_coeffs, dir));
              } else {
                factors.push_back (NaN);
              }
            }

          }

        }




        void TWDFCStaticImagePlugin::load_factors (const Streamline<>& tck, vector<default_type>& factors) const
        {
          // Use trilinear interpolation
          // Store values into local vectors, since it's a two-pass operation
          factors.assign (1, NaN);
          vector<float> values[2];
          for (size_t tck_end_index = 0; tck_end_index != 2; ++tck_end_index) {
            const ssize_t index = get_end_index (tck, bool(tck_end_index));
            if (index < 0)
              return;
            if (!interp.scanner (tck[index]))
              return;
            values[tck_end_index].reserve (interp.size(3));
            for (interp.index(3) = 0; interp.index(3) != interp.size(3); ++interp.index(3))
              values[tck_end_index].push_back (interp.value());
          }

          // Calculate the Pearson correlation coefficient
          default_type sums[2] = { 0.0, 0.0 };
          for (ssize_t i = 0; i != interp.size(3); ++i) {
            sums[0] += values[0][i];
            sums[1] += values[1][i];
          }
          const default_type means[2] = { sums[0] / default_type(interp.size(3)), sums[1] / default_type(interp.size(3)) };

          default_type product = 0.0;
          default_type variances[2] = { 0.0, 0.0 };
          for (ssize_t i = 0; i != interp.size(3); ++i) {
            product += ((values[0][i] - means[0]) * (values[1][i] - means[1]));
            variances[0] += Math::pow2 (values[0][i] - means[0]);
            variances[1] += Math::pow2 (values[1][i] - means[1]);
          }
          const default_type product_expectation = product / default_type(interp.size(3));
          const default_type stdevs[2] = { std::sqrt (variances[0] / default_type(interp.size(3)-1)),
                                           std::sqrt (variances[1] / default_type(interp.size(3)-1)) };

          if (stdevs[0] && stdevs[1])
            factors[0] = product_expectation / (stdevs[0] * stdevs[1]);
        }




        void TWDFCDynamicImagePlugin::load_factors (const Streamline<>& tck, vector<default_type>& factors) const
        {
          assert (statistic == ENDS_CORR);
          factors.assign (1, NaN);

          // Use trilinear interpolation
          // Store values into local vectors, since it's a two-pass operation
          vector<default_type> values[2];
          for (size_t tck_end_index = 0; tck_end_index != 2; ++tck_end_index) {
            const ssize_t index = get_end_index (tck, tck_end_index);
            if (index < 0)
              return;
            if (!interp.scanner (tck[index]))
              return;
            values[tck_end_index].reserve (kernel.size());
            for (ssize_t i = 0; i != kernel.size(); ++i) {
              interp.index(3) = sample_centre - kernel_centre + i;
              if (interp.index(3) >= 0 && interp.index(3) < interp.size(3))
                values[tck_end_index].push_back (interp.value());
              else
                values[tck_end_index].push_back (NaN);
            }
          }

          // Calculate the Pearson correlation coefficient within the kernel window
          default_type sums[2] = { 0.0, 0.0 };
          default_type kernel_sum = 0.0, kernel_sq_sum = 0.0;
          for (size_t i = 0; i != kernel.size(); ++i) {
            if (std::isfinite (values[0][i])) {
              sums[0] += kernel[i] * values[0][i];
              sums[1] += kernel[i] * values[1][i];
              kernel_sum += kernel[i];
              kernel_sq_sum += Math::pow2 (kernel[i]);
            }
          }
          const default_type means[2] = { sums[0] / kernel_sum, sums[1] / kernel_sum };
          const default_type denom = kernel_sum - (kernel_sq_sum / kernel_sum);

          default_type corr = 0.0, start_variance = 0.0, end_variance = 0.0;
          for (size_t i = 0; i != kernel.size(); ++i) {
            if (std::isfinite (values[0][i])) {
              corr           += kernel[i] * (values[0][i] - means[0]) * (values[1][i] - means[1]);
              start_variance += kernel[i] * Math::pow2 (values[0][i] - means[0]);
              end_variance   += kernel[i] * Math::pow2 (values[1][i] - means[1]);
            }
          }
          corr           /= denom;
          start_variance /= denom;
          end_variance   /= denom;

          if (start_variance && end_variance)
            factors[0] = corr / std::sqrt (start_variance * end_variance);
        }





      }
    }
  }
}




