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





const ssize_t TWIImagePluginBase::get_last_index_in_fov (const std::vector< Point<float> >& tck, const bool end) const
{
  ssize_t index = end ? tck.size() - 1 : 0;
  if (zero_outside_fov) {
    if (interp.scanner (tck[index]))
      return -1;
  } else {
    const ssize_t step = end ? -1 : 1;
    while (interp.scanner (tck[index])) {
      index += step;
      if (index == -1 || index == ssize_t(tck.size()))
        return -1;
    }
  }
  return index;
}
const Point<float> TWIImagePluginBase::get_last_point_in_fov (const std::vector< Point<float> >& tck, const bool end) const
{
  const ssize_t index = get_last_index_in_fov (tck, end);
  if (index == -1)
    return Point<float>();
  return tck[index];
}







void TWIScalarImagePlugin::load_factors (const std::vector< Point<float> >& tck, std::vector<float>& factors) const
{
  if (statistic == ENDS_MIN || statistic == ENDS_MEAN || statistic == ENDS_MAX || statistic == ENDS_PROD) {

    // Only the track endpoints contribute
    for (size_t tck_end_index = 0; tck_end_index != 2; ++tck_end_index) {
      const Point<float> endpoint = get_last_point_in_fov (tck, tck_end_index);
      if (endpoint.valid())
        factors.push_back (interp.value());
      else
        factors.push_back (0.0);
    }

  } else if (statistic == ENDS_CORR) {

    // Pearson correlation coefficient between the two endpoints
    factors.assign (1, 0.0f);
    input_voxel_type start (voxel), end (voxel);
    const Point<float> p_start (get_last_point_in_fov (tck, false));
    if (!p_start) return;
    Image::Nav::set_pos (start, interp, 0, 3);
    const Point<float> p_end (get_last_point_in_fov (tck, true));
    if (!p_end) return;
    Image::Nav::set_pos (end, interp, 0, 3);

    double start_sum = 0.0, end_sum = 0.0;
    for (start[3] = end[3] = 0; start[3] != start.dim (3); ++start[3], ++end[3]) {
      start_sum += start.value();
      end_sum   += end  .value();
    }
    const float start_mean = start_sum / double (start.dim (3));
    const float end_mean   = end_sum   / double (end  .dim (3));

    double product = 0.0, start_sum_variance = 0.0, end_sum_variance = 0.0;
    for (start[3] = end[3] = 0; start[3] != start.dim (3); ++start[3], ++end[3]) {
      product += ((start.value() - start_mean) * (end.value() - end_mean));
      start_sum_variance += Math::pow2 (start.value() - start_mean);
      end_sum_variance   += Math::pow2 (end  .value() - end_mean);
    }
    const float product_expectation = product / double (start.dim(3));
    const float start_stdev = std::sqrt (start_sum_variance / double(start.dim(3) - 1));
    const float end_stdev   = std::sqrt (end_sum_variance   / double(end  .dim(3) - 1));

    factors[0] = product_expectation / (start_stdev * end_stdev);

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
      const ssize_t index = get_last_index_in_fov (tck, tck_end_index);
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




