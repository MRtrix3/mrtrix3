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

#ifndef __dwi_tractography_mapping_gaussian_mapper_h__
#define __dwi_tractography_mapping_gaussian_mapper_h__


#include "image/info.h"

#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/twi_stats.h"
#include "dwi/tractography/mapping/gaussian/voxel.h"



namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {
namespace Gaussian {




class TrackMapper : public Mapping::TrackMapperTWI
{

    typedef Mapping::TrackMapperTWI BaseMapper;

  public:
    TrackMapper (const Image::Info& template_image, const contrast_t c) :
        BaseMapper (template_image, c, GAUSSIAN),
        gaussian_denominator  (0.0)
    {
      assert (c == SCALAR_MAP || c == SCALAR_MAP_COUNT || c == FOD_AMP || c == CURVATURE);
    }

    TrackMapper (const TrackMapper& that) :
        BaseMapper (that),
        gaussian_denominator (that.gaussian_denominator) { }

    ~TrackMapper() { }


    void set_gaussian_FWHM   (const float FWHM)
    {
      if (track_statistic != GAUSSIAN)
        throw Exception ("Cannot set Gaussian FWHM unless the track statistic is Gaussian");
      const float theta = FWHM / (2.0 * sqrt (2.0 * log (2.0)));
      gaussian_denominator = 2.0 * Math::pow2 (theta);
    }


    // Have to re-define the functor here, so that the alternative voxelise() methods can be called
    //   (Can't define these virtual because they're templated)
    // Also means that the call to run_queue() must cast to Gaussian::TrackMapper rather than using base class
    template <class Cont>
    bool operator() (Streamline<>& in, Cont& out) const
    {
      out.clear();
      out.index = in.index;
      out.weight = in.weight;
      if (in.empty())
        return true;
      if (preprocess (in, out) || map_zero) {
        upsampler (in);
        if (precise)
          voxelise_precise (in, out);
        else if (ends_only)
          voxelise_ends (in, out);
        else
          voxelise (in, out);
        postprocess (in, out);
      }
      return true;
    }




  protected:
    float gaussian_denominator;
    void  gaussian_smooth_factors (const Streamline<>&) const;

    // Overload corresponding functions in TrackMapperTWI
    void set_factor (const Streamline<>& tck, SetVoxelExtras& out) const;
    bool preprocess (const Streamline<>& tck, SetVoxelExtras& out) const { set_factor (tck, out); return true; }

    // Three versions of voxelise() function, just as in base class: difference is that here the
    //   corresponding TWI factor for each voxel mapping must be determined and passed to add_to_set()
    template <class Cont> void voxelise         (const Streamline<>&, Cont&) const;
    template <class Cont> void voxelise_precise (const Streamline<>&, Cont&) const;
    template <class Cont> void voxelise_ends    (const Streamline<>&, Cont&) const;

    inline void add_to_set (SetVoxel&   , const Point<int>&, const Point<float>&, const float, const float) const;
    inline void add_to_set (SetVoxelDEC&, const Point<int>&, const Point<float>&, const float, const float) const;
    inline void add_to_set (SetDixel&   , const Point<int>&, const Point<float>&, const float, const float) const;
    inline void add_to_set (SetVoxelTOD&, const Point<int>&, const Point<float>&, const float, const float) const;

    // Convenience function to convert from streamline position index to a linear-interpolated
    //   factor value (TrackMapperTWI member field factors[] only contains one entry per pre-upsampled point)
    inline float tck_index_to_factor (const size_t) const;


};



template <class Cont>
void TrackMapper::voxelise (const Streamline<>& tck, Cont& output) const
{

  size_t prev = 0;
  const size_t last = tck.size() - 1;

  Point<int> vox;
  for (size_t i = 0; i != last; ++i) {
    vox = round (transform.scanner2voxel (tck[i]));
    if (check (vox, info)) {
      const Point<float> dir ((tck[i+1] - tck[prev]).normalise());
      const float factor = tck_index_to_factor (i);
      add_to_set (output, vox, dir, 1.0f, factor);
    }
    prev = i;
  }

  vox = round (transform.scanner2voxel (tck[last]));
  if (check (vox, info)) {
    const Point<float> dir ((tck[last] - tck[prev]).normalise());
    const float factor = tck_index_to_factor (last);
    add_to_set (output, vox, dir, 1.0f, factor);
  }

  for (typename Cont::iterator i = output.begin(); i != output.end(); ++i)
    i->normalise();

}






template <class Cont>
void TrackMapper::voxelise_precise (const Streamline<>& tck, Cont& out) const
{
  typedef Point<float> PointF;

  static const float accuracy = Math::pow2 (0.005 * minvalue (info.vox (0), info.vox (1), info.vox (2)));

  if (tck.size() < 2)
    return;

  Math::Hermite<float> hermite (0.1);

  const PointF tck_proj_front = (tck[      0     ] * 2.0) - tck[     1      ];
  const PointF tck_proj_back  = (tck[tck.size()-1] * 2.0) - tck[tck.size()-2];

  unsigned int p = 0;
  PointF p_voxel_exit = tck.front();
  float mu = 0.0;
  bool end_track = false;
  Point<int> next_voxel (round (transform.scanner2voxel (tck.front())));

  do {

    const PointF p_voxel_entry (p_voxel_exit);
    PointF p_prev (p_voxel_entry);
    float length = 0.0;
    const float index_voxel_entry = float(p) + mu;
    const Point<int> this_voxel = next_voxel;

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

      const PointF* p_one  = (p == 1)              ? &tck_proj_front : &tck[p - 2];
      const PointF* p_four = (p == tck.size() - 1) ? &tck_proj_back  : &tck[p + 1];

      PointF p_min = p_prev;
      PointF p_max = tck[p];

      while (dist2 (p_min, p_max) > accuracy) {

        mu = 0.5 * (mu_min + mu_max);
        hermite.set (mu);
        const PointF p_mu = hermite.value (*p_one, tck[p - 1], tck[p], *p_four);
        const Point<int> mu_voxel = round (transform.scanner2voxel (p_mu));

        if (mu_voxel == this_voxel) {
          mu_min = mu;
          p_min = p_mu;
        } else {
          mu_max = mu;
          p_max = p_mu;
          next_voxel = mu_voxel;
        }

      }
      p_voxel_exit = p_max;

    }

    length += dist (p_prev, p_voxel_exit);
    PointF traversal_vector ((p_voxel_exit - p_voxel_entry).normalise());
    if (traversal_vector.valid() && check (this_voxel, info)) {
      const float index_voxel_exit = float(p) + mu;
      const size_t mean_tck_index = std::round (0.5 * (index_voxel_entry + index_voxel_exit));
      const float factor = tck_index_to_factor (mean_tck_index);
      add_to_set (out, this_voxel, traversal_vector, length, factor);
    }

  } while (!end_track);

}



template <class Cont>
void TrackMapper::voxelise_ends (const Streamline<>& tck, Cont& out) const
{
  for (size_t end = 0; end != 2; ++end) {
    const Point<int> vox = round (transform.scanner2voxel (end ? tck.back() : tck.front()));
    if (check (vox, info)) {
      const Point<float> dir = (end ? (tck[tck.size()-1] - tck[tck.size()-2]) : (tck[0] - tck[1])).normalise();
      const float factor = (end ? factors.back() : factors.front());
      add_to_set (out, vox, dir, 1.0f, factor);
    }
  }
}



inline void TrackMapper::add_to_set (SetVoxel&    out, const Point<int>& v, const Point<float>& d, const float l, const float f) const
{
  out.insert (v, l, f);
}
inline void TrackMapper::add_to_set (SetVoxelDEC& out, const Point<int>& v, const Point<float>& d, const float l, const float f) const
{
  out.insert (v, d, l, f);
}
inline void TrackMapper::add_to_set (SetDixel&    out, const Point<int>& v, const Point<float>& d, const float l, const float f) const
{
  assert (dixel_plugin);
  const size_t bin = (*dixel_plugin) (d);
  out.insert (v, bin, l, f);
}
inline void TrackMapper::add_to_set (SetVoxelTOD& out, const Point<int>& v, const Point<float>& d, const float l, const float f) const
{
  assert (tod_plugin);
  Math::Vector<float> sh;
  (*tod_plugin) (sh, d);
  out.insert (v, sh, l, f);
}





inline float TrackMapper::tck_index_to_factor (const size_t i) const
{
  const float ideal_index = float(i) / float(upsampler.get_ratio());
  const size_t lower_index = std::max (size_t(std::floor (ideal_index)), size_t(0));
  const size_t upper_index = std::min (size_t(std::ceil  (ideal_index)), size_t(factors.size() - 1));
  const float mu = ideal_index - float(lower_index);
  return ((mu * factors[upper_index]) + ((1.0f-mu) * factors[lower_index]));
}





}
}
}
}
}

#endif



