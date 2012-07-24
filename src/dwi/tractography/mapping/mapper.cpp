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
void TrackMapperTWI<SetVoxel>::voxelise (const std::vector< Point<float> >& tck, SetVoxel& voxels) const
{

  if (contrast == ENDPOINT) {

    Voxel vox = round (interp_out.scanner2voxel (tck.front()));
    if (check (vox, H_out))
      voxels.insert (vox);

    vox = round (interp_out.scanner2voxel (tck.back()));
    if (check (vox, H_out))
      voxels.insert (vox);

  } else {

    TrackMapperBase<SetVoxel>::voxelise (tck, voxels);

  }

}



template <>
void TrackMapperBase<SetVoxelDEC>::voxelise (const std::vector< Point<float> >& tck, SetVoxelDEC& voxels) const
{

  std::vector< Point<float> >::const_iterator prev = tck.begin();
  const std::vector< Point<float> >::const_iterator last = tck.end() - 1;

  VoxelDEC vox;
  for (std::vector< Point<float> >::const_iterator i = tck.begin(); i != last; ++i) {
    vox = round (interp_out.scanner2voxel (*i));
    if (check (vox, H_out)) {
      SetVoxelDEC::iterator existing_vox = voxels.find (vox);
      if (existing_vox == voxels.end()) {
        vox.set_dir (*(i+1) - *prev);
        voxels.insert (vox);
      } else {
        existing_vox->add_dir (*(i+1) - *prev);
      }
    }
    prev = i;
  }

  vox = round (interp_out.scanner2voxel (*last));
  if (check (vox, H_out)) {
    SetVoxelDEC::iterator existing_vox = voxels.find (vox);
    if (existing_vox == voxels.end()) {
      vox.set_dir (*last - *prev);
      voxels.insert (vox);
    } else {
      existing_vox->add_dir (*last - *prev);
    }
  }

  for (SetVoxelDEC::iterator i = voxels.begin(); i != voxels.end(); ++i)
    i->norm_dir();

}


template <>
void TrackMapperTWI<SetVoxelDEC>::voxelise (const std::vector< Point<float> >& tck, SetVoxelDEC& voxels) const
{

  if (contrast == ENDPOINT) {

    VoxelDEC vox = round (interp_out.scanner2voxel (tck.front()));
    if (check (vox, H_out)) {
      vox.set_dir (tck[0] - tck[1]);
      voxels.insert (vox);
    }

    vox = round (interp_out.scanner2voxel (tck.back()));
    if (check (vox, H_out)) {
      vox.set_dir (tck[tck.size() - 1] - tck[tck.size() - 2]);
      voxels.insert (vox);
    }

  } else {

    TrackMapperBase<SetVoxelDEC>::voxelise (tck, voxels);

  }

}



template <>
void TrackMapperBase<SetVoxelDir>::voxelise (const std::vector< Point<float> >& tck, SetVoxelDir& voxels) const
{

  typedef Point<float> PointF;

  static const float accuracy = Math::pow2 (0.005 * maxvalue (H_out.vox (0), H_out.vox (1), H_out.vox (2)));

  Math::Hermite<float> hermite (0.1);

  const PointF start_vector_offset = (tck.size() > 2) ? ((tck.front() - tck[1]) - (tck[1] - tck[2])) : PointF (0.0, 0.0, 0.0);
  const PointF start_vector = (tck.front() - tck[1]) + start_vector_offset;
  const PointF tck_proj_front = tck.front() + start_vector;

  const unsigned int last_point = tck.size() - 1;

  const PointF end_vector_offset = (tck.size() > 2) ? ((tck[last_point] - tck[last_point - 1]) - (tck[last_point - 1] - tck[last_point - 2])) : PointF (0.0, 0.0, 0.0);
  const PointF end_vector = (tck[last_point] - tck[last_point - 1]) + end_vector_offset;
  const PointF tck_proj_back = tck.back() + end_vector;

  unsigned int p = 0;
  PointF p_end = tck.front();
  float mu = 0.0;
  bool end_track = false;
  Voxel next_voxel (round (interp_out.scanner2voxel (tck.front())));

  while (!end_track) {

    const PointF p_start (p_end);

    const Voxel this_voxel = next_voxel;

    while ((p != tck.size()) && (round (interp_out.scanner2voxel (tck[p])) == this_voxel)) {
      ++p;
      mu = 0.0;
    }

    if (p == tck.size()) {
      p_end = tck.back();
      end_track = true;
    } else {

      float mu_min = mu;
      float mu_max = 1.0;
      Voxel mu_voxel = this_voxel;

      PointF p_mu = tck[p];

      do {

        p_end = p_mu;

        mu = 0.5 * (mu_min + mu_max);
        const PointF* p_one = (p == 1) ? &tck_proj_front : &tck[p - 2];
        const PointF* p_four = (p == tck.size() - 1) ? &tck_proj_back : &tck[p + 1];

        hermite.set (mu);
        p_mu = hermite.value (*p_one, tck[p - 1], tck[p], *p_four);

        mu_voxel = round (interp_out.scanner2voxel (p_mu));
        if (mu_voxel == this_voxel) {
          mu_min = mu;
        } else {
          mu_max = mu;
          next_voxel = mu_voxel;
        }

      } while (dist2 (p_mu, p_end) > accuracy);

    }

    const PointF traversal_vector (p_end - p_start);
    if (traversal_vector.norm2()) {
      SetVoxelDir::iterator existing_vox = voxels.find (this_voxel);
      if (existing_vox == voxels.end()) {
        VoxelDir voxel_dir (this_voxel);
        voxel_dir.add_dir (traversal_vector);
        voxels.insert (voxel_dir);
      } else {
        existing_vox->add_dir (traversal_vector);
      }
    }

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
void TrackMapperTWI<SetVoxelDECFactor>::voxelise (const std::vector< Point<float> >& tck, SetVoxelDECFactor& voxels) const
{

  Point<float> prev = tck.front();
  const Point<float>& last = tck[tck.size() - 1];

  VoxelDECFactor vox;
  for (unsigned int i = 0; i != tck.size() - 1; ++i) {
    vox = round (interp_out.scanner2voxel (tck[i]));
    if (check (vox, H_out)) {

      const float ideal_index = float(i) / float(os_factor);
      const size_t lower_index = MAX(floor (ideal_index), 0);
      const size_t upper_index = MIN(ceil  (ideal_index), tck.size() - 1);
      const float mu = ideal_index - lower_index;
      const float factor = (mu * factors[upper_index]) + ((1.0 - mu) * factors[lower_index]);

      SetVoxelDECFactor::iterator existing_vox = voxels.find (vox);
      if (existing_vox == voxels.end()) {
        vox.set_factor (factor);
        vox.set_dir (tck[i+1] - prev);
        voxels.insert (vox);
      } else {
        existing_vox->add_dir (tck[i+1] - prev);
        existing_vox->add_contribution (factor);
      }
    }
    prev = tck[i];
  }

  vox = round (interp_out.scanner2voxel (last));
  if (check (vox, H_out)) {

    const float ideal_index = float(tck.size() - 1) / float(os_factor);
    const size_t lower_index = MAX(floor (ideal_index), 0);
    const size_t upper_index = MIN(ceil  (ideal_index), tck.size() - 1);
    const float mu = ideal_index - lower_index;
    const float factor = (mu * factors[upper_index]) + ((1.0 - mu) * factors[lower_index]);

    SetVoxelDECFactor::iterator existing_vox = voxels.find (vox);
    if (existing_vox == voxels.end()) {
      vox.set_dir (last - prev);
      vox.set_factor (factor);
      voxels.insert (vox);
    } else {
      existing_vox->add_dir (last - prev);
      existing_vox->add_contribution (factor);
    }
  }

  for (SetVoxelDECFactor::iterator i = voxels.begin(); i != voxels.end(); ++i)
    i->norm_dir();

}





}
}
}
}




