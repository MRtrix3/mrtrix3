/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 2011.

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


#include "dwi/tractography/mapping/mapper.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {




template <>
void TrackMapperBase<SetVoxel>::voxelise (const std::vector< Point<float> >& tck, SetVoxel& voxels) const
{

  Voxel vox;
  for (std::vector< Point<float> >::const_iterator i = tck.begin(); i != tck.end(); ++i) {
    vox = round (interp_out.scanner2voxel (*i));
    if (check (vox, H_out))
      voxels.insert (vox);
  }

}



template <>
void TrackMapperBase<SetVoxelDir>::voxelise (const std::vector< Point<float> >& tck, SetVoxelDir& voxels) const
{

  std::vector< Point<float> >::const_iterator prev = tck.begin();
  const std::vector< Point<float> >::const_iterator last = tck.end() - 1;

  VoxelDir vox;
  for (std::vector< Point<float> >::const_iterator i = tck.begin(); i != last; ++i) {
    vox = round (interp_out.scanner2voxel (*i));
    if (check (vox, H_out)) {
      vox.dir = *(i+1) - *prev;
      SetVoxelDir::iterator existing_vox = voxels.find (vox);
      if (existing_vox == voxels.end()) {
        voxels.insert (vox);
      } else {
        VoxelDir new_vox (*existing_vox);
        new_vox.dir += vox.dir;
        voxels.erase (existing_vox);
        voxels.insert (new_vox);
      }
      prev = i;
    }
  }
  vox = round (interp_out.scanner2voxel (*last));
  if (check (vox, H_out)) {
    vox.dir = *last - *prev;
    SetVoxelDir::iterator existing_vox = voxels.find (vox);
    if (existing_vox == voxels.end()) {
      voxels.insert (vox);
    } else {
      VoxelDir new_vox (*existing_vox);
      new_vox.dir += vox.dir;
      voxels.erase (existing_vox);
      voxels.insert (new_vox);
    }
  }

}




template <>
void TrackMapperTWI<SetVoxel>::voxelise (const std::vector< Point<float> >& tck, SetVoxel& voxels) const
{

  if (contrast == ENDPOINT) {

    Voxel vox;
    const size_t os_ratio = TrackMapperBase<SetVoxel>::os_factor;
    for (size_t i = os_ratio; i != (2 * os_ratio) + 1; ++i) {
      vox = round (interp_out.scanner2voxel (tck[1]));
      if (check (vox, H_out))
        voxels.insert (vox);
    }
    for (size_t i = tck.size() - os_ratio - 1; i != tck.size() - (2 * os_ratio) - 2; --i) {
      vox = round (interp_out.scanner2voxel (tck[1]));
      if (check (vox, H_out))
        voxels.insert (vox);
    }

  } else {

    TrackMapperBase<SetVoxel>::voxelise (tck, voxels);

  }

}



template <>
void TrackMapperTWI<SetVoxelFactor>::voxelise (const std::vector< Point<float> >& tck, SetVoxelFactor& voxels) const
{

  VoxelFactor vox;
  for (size_t i = 0; i != tck.size(); ++i) {
    vox = round (interp_out.scanner2voxel (tck[i]));
    if (check (vox, H_out)) {

      // Get a linearly-interpolated value from factors[] based upon factors[] being
      //   generated with non-interpolated data, and index 'i' here representing interpolated data
      const float ideal_index = float(i) / float(os_factor);
      const size_t lower_index = MAX(floor (ideal_index), 0);
      const size_t upper_index = MIN(ceil  (ideal_index), tck.size() - 1);
      const float mu = ideal_index - lower_index;
      const float factor = (mu * factors[upper_index]) + ((1.0 - mu) * factors[lower_index]);

      // Change here from base classes: need to explicitly check whether this voxel has been visited
      SetVoxelFactor::iterator v = voxels.find (vox);
      if (v == voxels.end()) {
        vox.set_factor (factor);
        voxels.insert (vox);
      } else {
        vox = *v;
        vox.add_contribution (factor);
        voxels.erase (v);
        voxels.insert (vox);
      }

    }
  }

}




template <>
void TrackMapperTWI<SetVoxel>::set_factor (const std::vector< Point<float> >& tck, SetVoxel& out)
{

  std::vector<float> values;
  double factor = 0.0;
  size_t count = 0;

  switch (contrast) {

    case TDI:       factor = 1.0; break;
    case DECTDI:    factor = 1.0; break;
    case ENDPOINT:  factor = 1.0; break;
    case LENGTH:    factor = (step_size * (tck.size() - 1)); break;
    case INVLENGTH: factor = (step_size / (tck.size() - 1)); break;

    case SCALAR_MAP:
    case SCALAR_MAP_COUNT:
    case FOD_AMP:
    case CURVATURE:

      values.reserve (tck.size());
      load_values (tck, values); // This should call the overloaded virtual function for TrackMapperImage

      switch (track_statistic) {

        case SUM:
          for (std::vector<float>::const_iterator i = values.begin(); i != values.end(); ++i) {
            if (finite (*i))
              factor += *i;
          }
          break;

        case MIN:
          factor = INFINITY;
          for (std::vector<float>::const_iterator i = values.begin(); i != values.end(); ++i) {
            if (finite (*i))
              factor = MIN (factor, *i);
          }
          break;

        case MEAN:
          for (std::vector<float>::const_iterator i = values.begin(); i != values.end(); ++i) {
            if (finite (*i)) {
              factor += *i;
              ++count;
            }
          }
          factor = (count ? (factor / float(count)) : 0.0);
          break;

        case MAX:
          factor = -INFINITY;
          for (std::vector<float>::const_iterator i = values.begin(); i != values.end(); ++i) {
            if (finite (factor))
              factor = MAX (factor, *i);
          }
          break;

        case MEDIAN:
          if (values.empty()) {
            factor = 0.0;
          } else {
            nth_element (values.begin(), values.begin() + (values.size() / 2), values.end());
            factor = *(values.begin() + (values.size() / 2));
          }
          break;

        case FMRI_MIN:
          factor = MIN(values[0], values[1]);
          break;

        case FMRI_MEAN:
          factor = 0.5 * (values[0] + values[1]);
          break;

        case FMRI_MAX:
          factor = MAX(values[0], values[1]);
          break;

        default:
          throw Exception ("Undefined / unsupported track statistic in TrackMapper<SetVoxel>::set_factor()");

      }
      break;

    default:
      throw Exception ("Undefined / unsupported contrast mechanism in TrackMapper<SetVoxel>::set_factor()");

  }

  if (contrast == SCALAR_MAP_COUNT)
    factor = (factor ? 1.0 : 0.0);

  if (!finite (factor))
    factor = 0.0;

  out.factor = factor;

}



template <>
void TrackMapperTWI<SetVoxelDir>::set_factor (const std::vector< Point<float> >& tck, SetVoxelDir& out)
{
  out.factor = 1.0;
}



template <>
void TrackMapperTWI<SetVoxelFactor>::set_factor (const std::vector< Point<float> >& tck, SetVoxelFactor& /*unused*/)
{

  // Firstly, get the interpolated value at each point
  std::vector<float> values;
  values.reserve (tck.size());
  load_values (tck, values); // This should call the overloaded virtual function for TrackMapperImage

  // From each point, get an estimate of the appropriate factor based upon its distance to all other points
  // Distances accumulate along length of track
  factors.clear();
  factors.reserve (tck.size());

  for (size_t i = 0; i != tck.size(); ++i) {

    double sum = 0.0, norm = 0.0;

    if (values[i] != INFINITY) {
      sum  = values[i];
      norm = 1.0; // Gaussian is unnormalised -> e^0 = 1
    }

    float distance = 0.0;
    for (size_t j = i; j--; ) { // Decrement AFTER null test, so loop runs with j = 0
      distance += step_size;
      if (values[j] != INFINITY) {
        const float this_weight = exp (-distance * distance / gaussian_denominator);
        norm += this_weight;
        sum  += this_weight * values[j];
      }
    }
    distance = 0.0;
    for (size_t j = i + 1; j < tck.size(); ++j) {
      distance += step_size;
      if (values[j] != INFINITY) {
        const float this_weight = exp (-distance * distance / gaussian_denominator);
        norm += this_weight;
        sum  += this_weight * values[j];
      }
    }

    if (norm)
      factors.push_back (sum / norm);
    else
      factors.push_back (0.0);

  }

}





}
}
}
}




