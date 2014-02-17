/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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
    vox = round (transform.scanner2voxel (*i));
    if (check (vox, info))
      voxels.insert (vox);
  }

}


template <>
void TrackMapperTWI<SetVoxel>::voxelise (const std::vector< Point<float> >& tck, SetVoxel& voxels) const
{

  if (contrast == ENDPOINT) {

    Voxel vox = round (transform.scanner2voxel (tck.front()));
    if (check (vox, info))
      voxels.insert (vox);

    vox = round (transform.scanner2voxel (tck.back()));
    if (check (vox, info))
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
    vox = round (transform.scanner2voxel (*i));
    if (check (vox, info)) {
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

  vox = round (transform.scanner2voxel (*last));
  if (check (vox, info)) {
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

    VoxelDEC vox = round (transform.scanner2voxel (tck.front()));
    if (check (vox, info)) {
      vox.set_dir (tck[0] - tck[1]);
      voxels.insert (vox);
    }

    vox = round (transform.scanner2voxel (tck.back()));
    if (check (vox, info)) {
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

  static const float accuracy = Math::pow2 (0.005 * maxvalue (info.vox (0), info.vox (1), info.vox (2)));

  if (tck.size() < 2)
    return;

  Math::Hermite<float> hermite (0.1);

  const PointF tck_proj_front = (tck[0] * 2.0) - tck[1];
  const PointF tck_proj_back  = (tck[1] * 2.0) - tck[0];

  unsigned int p = 0;
  PointF p_voxel_exit = tck.front();
  float mu = 0.0;
  bool end_track = false;
  Voxel next_voxel (round (transform.scanner2voxel (tck.front())));

  while (!end_track) {

    const PointF p_voxel_entry (p_voxel_exit);
    PointF p_prev (p_voxel_entry);
    float length = 0.0;
    const Voxel this_voxel = next_voxel;

    while ((p != tck.size()) && ((next_voxel = round (transform.scanner2voxel (tck[p]))) == this_voxel)) {
      length += dist (p_prev, tck[p]);
      p_prev = tck[p];
      ++p;
      mu = 0.0;
    }

    if (p == tck.size()) {
      p_voxel_exit = tck.back();
      end_track = true;
    } else {

      float mu_min = mu;
      float mu_max = 1.0;
      Voxel mu_voxel = this_voxel;

      const PointF* p_one  = (p == 1)              ? &tck_proj_front : &tck[p - 2];
      const PointF* p_four = (p == tck.size() - 1) ? &tck_proj_back  : &tck[p + 1];

      PointF p_mu = tck[p];

      do {

        p_voxel_exit = p_mu;

        mu = 0.5 * (mu_min + mu_max);

        hermite.set (mu);
        p_mu = hermite.value (*p_one, tck[p - 1], tck[p], *p_four);

        mu_voxel = round (transform.scanner2voxel (p_mu));
        if (mu_voxel == this_voxel) {
          mu_min = mu;
        } else {
          mu_max = mu;
          next_voxel = mu_voxel;
        }

      } while (dist2 (p_mu, p_voxel_exit) > accuracy);
      p_voxel_exit = p_mu;

    }

    length += dist (p_prev, p_voxel_exit);
    PointF traversal_vector (p_voxel_exit - p_voxel_entry);
    if (traversal_vector.normalise()) {
      VoxelDir voxel_dir (this_voxel, traversal_vector, length);
      voxels.insert (voxel_dir);
    }

  }

}




void TrackMapperDixel::voxelise (const std::vector< Point<float> >& tck, SetDixel& dixels) const
{

  typedef Point<float> PointF;

  static const float accuracy = Math::pow2 (0.005 * maxvalue (info.vox (0), info.vox (1), info.vox (2)));

  if (tck.size() < 2)
    return;

  Math::Hermite<float> hermite (0.1);

  const PointF tck_proj_front = (tck[0] * 2.0) - tck[1];
  const PointF tck_proj_back  = (tck[1] * 2.0) - tck[0];

  unsigned int p = 0;
  PointF p_voxel_exit = tck.front();
  float mu = 0.0;
  bool end_track = false;
  Voxel next_voxel (round (transform.scanner2voxel (tck.front())));

  while (!end_track) {

    const PointF p_voxel_entry (p_voxel_exit);
    PointF p_prev (p_voxel_entry);
    float length = 0.0;
    const Voxel this_voxel = next_voxel;

    while ((p != tck.size()) && ((next_voxel = round (transform.scanner2voxel (tck[p]))) == this_voxel)) {
      length += dist (p_prev, tck[p]);
      p_prev = tck[p];
      ++p;
      mu = 0.0;
    }

    if (p == tck.size()) {
      p_voxel_exit = tck.back();
      end_track = true;
    } else {

      float mu_min = mu;
      float mu_max = 1.0;
      Voxel mu_voxel = this_voxel;

      const PointF* p_one  = (p == 1)              ? &tck_proj_front : &tck[p - 2];
      const PointF* p_four = (p == tck.size() - 1) ? &tck_proj_back  : &tck[p + 1];

      PointF p_mu = tck[p];

      do {

        p_voxel_exit = p_mu;

        mu = 0.5 * (mu_min + mu_max);

        hermite.set (mu);
        p_mu = hermite.value (*p_one, tck[p - 1], tck[p], *p_four);

        mu_voxel = round (transform.scanner2voxel (p_mu));
        if (mu_voxel == this_voxel) {
          mu_min = mu;
        } else {
          mu_max = mu;
          next_voxel = mu_voxel;
        }

      } while (dist2 (p_mu, p_voxel_exit) > accuracy);
      p_voxel_exit = p_mu;

    }

    length += dist (p_prev, p_voxel_exit);
    PointF traversal_vector (p_voxel_exit - p_voxel_entry);
    if (traversal_vector.normalise()) {

      // Only here does this voxelise() function differ from TrackMapperBase<SetVoxelDir>::voxelise()
      // Map to the appropriate direction & add to the set; only add to an existing dixel if
      //   both the voxel and direction match; otherwise, a single streamline may contribute to
      //   multiple dixels within a voxel)
      // Although it would be possible to functionalise TrackMapperBase<SetVoxelDir>::voxelise(),
      //   call this function, then map individual VoxelDirs to dixels, this would require
      //   twice as many memory requests and potentially more cache misses. So I'm sticking to the
      //   code duplication solution.
      const size_t bin = dirs.select_direction (traversal_vector);
      Dixel this_dixel (this_voxel, bin, length);

      SetDixel::iterator existing_dixel = dixels.find (this_dixel);
      if (existing_dixel == dixels.end())
        dixels.insert (this_dixel);
      else
        existing_dixel->set_value (existing_dixel->get_value() + length);

    }

  }

}




template <>
void TrackMapperTWI<SetVoxelFactor>::voxelise (const std::vector< Point<float> >& tck, SetVoxelFactor& voxels) const
{

  VoxelFactor vox;
  for (size_t i = 0; i != tck.size(); ++i) {
    vox = round (transform.scanner2voxel (tck[i]));
    if (check (vox, info)) {

      // Get a linearly-interpolated value from factors[] based upon factors[] being
      //   generated with non-interpolated data, and index 'i' here representing interpolated data
      const float ideal_index = float(i) / float(get_upsample_factor());
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
    vox = round (transform.scanner2voxel (tck[i]));
    if (check (vox, info)) {

      const float ideal_index = float(i) / float(get_upsample_factor());
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

  vox = round (transform.scanner2voxel (last));
  if (check (vox, info)) {

    const float ideal_index = float(tck.size() - 1) / float(get_upsample_factor());
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




