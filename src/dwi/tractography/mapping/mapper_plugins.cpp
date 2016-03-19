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

    // Pearson correlation coefficient between the two endpoints
    factors.assign (1, 0.0f);
    input_voxel_type start (voxel), end (voxel);
    const ssize_t start_index (get_end_index (tck, false));
    if (start_index < 0) return;
    Point<float> p = interp.scanner2voxel (tck[start_index]);
    Point<int> v (std::round (p[0]), std::round (p[1]), std::round (p[2]));
    Image::Nav::set_pos (start, v, 0, 3);
    const ssize_t end_index (get_end_index (tck, true));
    if (end_index < 0) return;
    p = interp.scanner2voxel (tck[end_index]);
    v.set (std::round (p[0]), std::round (p[1]), std::round (p[2]));
    Image::Nav::set_pos (end, v, 0, 3);

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

    if (start_stdev && end_stdev)
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




