/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2014.

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


#include "dwi/tractography/mapping/mapper_plugins.h"

#include "image/nav.h"

namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {




void TWIImagePluginBase::set_backtrack()
{
  backtrack = true;
  if (statistic != ENDS_CORR)
    return;
  Image::Header H (*data);
  H.set_ndim (3);
  H.datatype() = DataType::Bit;
  backtrack_mask.reset (new Image::BufferScratch<bool> (H));
  input_voxel_type v_data (voxel);
  Image::BufferScratch<bool>::voxel_type v_mask (*backtrack_mask);
  auto f = [] (Image::BufferPreload<float>::voxel_type& in, Image::BufferScratch<bool>::voxel_type& mask) {
    for (in[3] = 0; in[3] != in.dim(3); ++in[3]) {
      if (std::isfinite (in.value()) && in.value()) {
        mask.value() = true;
        return true;
      }
    }
    mask.value() = false;
    return true;
  };

  Image::ThreadedLoop ("pre-calculating mask of valid time-series voxels...", v_mask)
      .run (f, v_data, v_mask);
}




const ssize_t TWIImagePluginBase::get_end_index (const std::vector< Point<float> >& tck, const bool end) const
{
  ssize_t index = end ? tck.size() - 1 : 0;
  if (backtrack) {

    const ssize_t step = end ? -1 : 1;

    if (statistic == ENDS_CORR) {

      Image::BufferScratch<bool>::voxel_type v_mask (*backtrack_mask);
      for (; index >= 0 && index < ssize_t(tck.size()); index += step) {
        const Point<float> p = interp.scanner2voxel (tck[index]);
        const Point<int> v (std::round (p[0]), std::round (p[1]), std::round (p[2]));
        if (Image::Nav::within_bounds (v_mask, v) && Image::Nav::get_value_at_pos (v_mask, v)) {
          interp.scanner (tck[index]); // For the calling function to get the position
          return index;
        }
      }
      return -1;

    } else {

      while (interp.scanner (tck[index])) {
        index += step;
        if (index == -1 || index == ssize_t(tck.size()))
          return -1;
      }

    }

  } else {
    if (interp.scanner (tck[index]))
      return -1;
  }
  return index;
}
const Point<float> TWIImagePluginBase::get_end_point (const std::vector< Point<float> >& tck, const bool end) const
{
  const ssize_t index = get_end_index (tck, end);
  if (index == -1)
    return Point<float>();
  return tck[index];
}







void TWIScalarImagePlugin::load_factors (const std::vector< Point<float> >& tck, std::vector<float>& factors) const
{
  if (statistic == ENDS_MIN || statistic == ENDS_MEAN || statistic == ENDS_MAX || statistic == ENDS_PROD) {

    // Only the track endpoints contribute
    for (size_t tck_end_index = 0; tck_end_index != 2; ++tck_end_index) {
      const Point<float> endpoint = get_end_point (tck, tck_end_index);
      if (endpoint.valid())
        factors.push_back (interp.value());
      else
        factors.push_back (0.0);
    }

  } else if (statistic == ENDS_CORR) {

    // Use trilinear interpolation
    // Store values into local vectors, since it's a two-pass operation
    factors.assign (1, 0.0f);
    std::vector<float> values[2];
    for (size_t tck_end_index = 0; tck_end_index != 2; ++tck_end_index) {
      const Point<float> endpoint = get_end_point (tck, tck_end_index);
      if (!endpoint.valid())
        return;
      values[tck_end_index].reserve (interp.dim(3));
      for (interp[3] = 0; interp[3] != interp.dim(3); ++interp[3])
        values[tck_end_index].push_back (interp.value());
    }

    // Calculate the Pearson correlation coefficient
    double sums[2] = { 0.0, 0.0 };
    for (ssize_t i = 0; i != interp.dim(3); ++i) {
      sums[0] += values[0][i];
      sums[1] += values[1][i];
    }
    const double means[2] = { sums[0] / double(interp.dim(3)), sums[1] / double(interp.dim(3)) };

    double product = 0.0;
    double variances[2] = { 0.0, 0.0 };
    for (ssize_t i = 0; i != interp.dim(3); ++i) {
      product += ((values[0][i] - means[0]) * (values[1][i] - means[i]));
      variances[0] += Math::pow2 (values[0][i] - means[0]);
      variances[1] += Math::pow2 (values[1][i] - means[1]);
    }
    const double product_expectation = product / double (interp.dim(3));
    const double stdevs[2] = { std::sqrt (variances[0] / double(interp.dim(3)-1)),
                              std::sqrt (variances[1] / double(interp.dim(3)-1)) };

    if (stdevs[0] && stdevs[1])
      factors[0] = product_expectation / (stdevs[0] * stdevs[1]);

  } else {

    // The entire length of the track contributes
    for (std::vector< Point<float> >::const_iterator i = tck.begin(); i != tck.end(); ++i) {
      if (!interp.scanner (*i))
        factors.push_back (interp.value());
      else
        factors.push_back (0.0);
    }

  }
}





void TWIFODImagePlugin::load_factors (const std::vector< Point<float> >& tck, std::vector<float>& factors) const
{
  if (statistic == ENDS_MAX || statistic == ENDS_MEAN || statistic == ENDS_MIN || statistic == ENDS_PROD) {

    for (size_t tck_end_index = 0; tck_end_index != 2; ++tck_end_index) {
      const ssize_t index = get_end_index (tck, tck_end_index);
      if (index == -1) {
        factors.push_back (0.0);
      } else {
        for (interp[3] = 0; interp[3] != interp.dim(3); ++interp[3])
          sh_coeffs[interp[3]] = interp.value();
        const Point<float> dir = (tck[(index == ssize_t(tck.size()-1)) ? index : (index+1)] - tck[index ? (index-1) : 0]).normalise();
        factors.push_back (precomputer->value (sh_coeffs, dir));
      }
    }

  } else {

    for (size_t i = 0; i != tck.size(); ++i) {
      const Point<float>& p = tck[i];
      if (!interp.scanner (p)) {
        // Get the FOD at this (interploated) point
        for (interp[3] = 0; interp[3] != interp.dim(3); ++interp[3])
          sh_coeffs[interp[3]] = interp.value();
        // Get the FOD amplitude along the streamline tangent
        const Point<float> dir = (tck[(i == tck.size()-1) ? i : (i+1)] - tck[i ? (i-1) : 0]).normalise();
        factors.push_back (precomputer->value (sh_coeffs, dir));
      } else {
        factors.push_back (0.0);
      }
    }

  }

}






}
}
}
}




