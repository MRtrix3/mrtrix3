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

#ifndef __dwi_tractography_mapping_mapper_h__
#define __dwi_tractography_mapping_mapper_h__



#include <vector>

#include "point.h"

#include "image/buffer_preload.h"
#include "image/info.h"
#include "image/transform.h"
#include "math/matrix.h"
#include "math/SH.h"
#include "thread/queue.h"

#include "dwi/directions/set.h"

#include "image/interp/linear.h"

#include "dwi/tractography/resample.h"
#include "dwi/tractography/streamline.h"

#include "dwi/tractography/mapping/mapping.h"
#include "dwi/tractography/mapping/twi_stats.h"
#include "dwi/tractography/mapping/voxel.h"


// Didn't bother making this a command-line option, since curvature contrast results were very poor regardless of smoothing
#define CURVATURE_TRACK_SMOOTHING_FWHM 10.0 // In mm






namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {




template <class Cont>
class TrackMapperBase
{

  public:
    TrackMapperBase (const Image::Info& template_image, const size_t upsample_ratio = 1, const bool mapzero = false) :
        upsampler (upsample_ratio),
        info      (template_image),
        transform (info),
        map_zero  (mapzero) { }

    TrackMapperBase (const TrackMapperBase& that) :
        upsampler (that.upsampler),
        info      (that.info),
        transform (info),
        map_zero  (that.map_zero) { }

    virtual ~TrackMapperBase() { }


    bool operator() (Streamline<float>& in, Cont& out)
    {
      out.clear();
      out.index = in.index;
      out.weight = in.weight;
      if (in.empty())
        return true;
      if (preprocess (in, out) || map_zero) {
        upsampler (in);
        voxelise (in, out);
        postprocess (in, out);
      }
      return true;
    }


  private:
    Upsampler<float> upsampler;


  protected:
    const Image::Info& info;
    Image::Transform transform;
    const bool map_zero;


    size_t get_upsample_factor() const { return upsampler.get_ratio(); }

    virtual void voxelise    (const std::vector< Point<float> >&, Cont&) const { throw Exception ("Running empty virtual function TrackMapperBase::voxelise()"); }
    virtual bool preprocess  (const std::vector< Point<float> >& tck, Cont& out) { out.factor = 1.0; return true; }
    virtual void postprocess (const std::vector< Point<float> >&, Cont&) const { }

};

template <> void TrackMapperBase<SetVoxel>   ::voxelise (const std::vector< Point<float> >&, SetVoxel&)    const;
template <> void TrackMapperBase<SetVoxelDEC>::voxelise (const std::vector< Point<float> >&, SetVoxelDEC&) const;
template <> void TrackMapperBase<SetVoxelDir>::voxelise (const std::vector< Point<float> >&, SetVoxelDir&) const;





class TrackMapperDixel : public TrackMapperBase<SetDixel>
{
  public:
    TrackMapperDixel (const Image::Header& template_image, const size_t upsample_ratio, const bool map_zero, const DWI::Directions::FastLookupSet& directions) :
        TrackMapperBase<SetDixel> (template_image, upsample_ratio, map_zero),
        dirs (directions) { }

  protected:
    const DWI::Directions::FastLookupSet& dirs;

  private:
    void voxelise (const std::vector< Point<float> >&, SetDixel&) const;

};




template <class Cont>
class TrackMapperTWI : public TrackMapperBase<Cont>
{
  public:
    TrackMapperTWI (const Image::Header& template_image, const size_t upsample_ratio, const bool map_zero, const float step, const contrast_t c, const tck_stat_t s, const float denom = 0.0) :
        TrackMapperBase<Cont> (template_image, upsample_ratio, map_zero),
        contrast              (c),
        track_statistic       (s),
        gaussian_denominator  (denom),
        step_size             (step) { }

    TrackMapperTWI (const TrackMapperTWI& that) :
        TrackMapperBase<Cont> (that),
        contrast              (that.contrast),
        track_statistic       (that.track_statistic),
        gaussian_denominator  (that.gaussian_denominator),
        step_size             (that.step_size) { }

    virtual ~TrackMapperTWI() { }


  protected:
    virtual void load_factors (const std::vector< Point<float> >&);
    const contrast_t contrast;
    const tck_stat_t track_statistic;

    // Members for when the contribution of a track is not constant along its length (i.e. Gaussian smoothed along the track)
    const float gaussian_denominator;
    std::vector<float> factors;
    void gaussian_smooth_factors();


  private:
    const float step_size;

    void set_factor (const std::vector< Point<float> >&, Cont&);

    // Call the inheited virtual function unless a specialisation for this class exists
    void voxelise (const std::vector< Point<float> >& tck, Cont& voxels) const { TrackMapperBase<Cont>::voxelise (tck, voxels); }

    // Overload virtual function
    bool preprocess (const std::vector< Point<float> >& tck, Cont& out) { set_factor (tck, out); return out.factor; }

};




template <class Cont>
void TrackMapperTWI<Cont>::load_factors (const std::vector< Point<float> >& tck)
{

  if (contrast != CURVATURE)
    throw Exception ("Function TrackMapperTWI::load_factors() only works with curvature contrast");

  std::vector< Point<float> > tangents;
  tangents.reserve (tck.size());

  // Would like to be able to manipulate the length over which the tangent calculation is affected
  // However don't want to just take a pair of distant points and get the tangent that way; would rather
  //   find a way to 'smooth' the curvature in a non-scalar fashion i.e. inverted curvature cancels
  // Ideally would like to get a curvature measurement & azimuth at each point; these can be averaged
  //   using a Gaussian kernel
  // But how to define azimuth & make it consistent between points?
  // Average principal normal vectors using a gaussian kernel, re-determine the curvature

  for (size_t i = 0; i != tck.size(); ++i) {
    Point<float> this_tangent;
    if (i == 0)
      this_tangent = ((tck[1]   - tck[0]  ).normalise());
    else if (i == tck.size() - 1)
      this_tangent = ((tck[i]   - tck[i-1]).normalise());
    else
      this_tangent = ((tck[i+1] - tck[i-1]).normalise());
    if (this_tangent.valid())
      tangents.push_back (this_tangent);
    else
      tangents.push_back (Point<float> (0.0, 0.0, 0.0));
  }

  // For those tangents which are invalid, fill with valid factors from neighbours
  for (size_t i = 0; i != tangents.size(); ++i) {
    if (tangents[i] == Point<float> (0.0, 0.0, 0.0)) {

      if (i == 0) {
        size_t j;
        for (j = 1; (j < tck.size() - 1) && (tangents[j] != Point<float> (0.0, 0.0, 0.0)); ++j);
        tangents[i] = tangents[j];
      } else if (i == tangents.size() - 1) {
        size_t k;
        for (k = i - 1; k && (tangents[k] != Point<float> (0.0, 0.0, 0.0)); --k);
        tangents[i] = tangents[k];
      } else {
        size_t j, k;
        for (j = 1; (j < tck.size() - 1) && (tangents[j] != Point<float> (0.0, 0.0, 0.0)); ++j);
        for (k = i - 1; k && (tangents[k] != Point<float> (0.0, 0.0, 0.0)); --k);
        tangents[i] = (tangents[j] + tangents[k]).normalise();
      }

    }
  }

  // Smooth both the tangent vectors and the principal normal vectors according to a Gaussuan kernel
  // Remember: tangent vectors are unit length, but for principal normal vectors length must be preserved!

  std::vector< Point<float> > smoothed_tangents;
  smoothed_tangents.reserve (tangents.size());

  static const float gaussian_theta = CURVATURE_TRACK_SMOOTHING_FWHM / (2.0 * sqrt (2.0 * log (2.0)));
  static const float gaussian_denominator = 2.0 * gaussian_theta * gaussian_theta;

  for (size_t i = 0; i != tck.size(); ++i) {

    Point<float> this_tangent (0.0, 0.0, 0.0);
    std::pair<float, Point<float> > this_normal (std::make_pair (0.0, Point<float>(0.0, 0.0, 0.0)));

    for (size_t j = 0; j != tck.size(); ++j) {
      const float distance = step_size * Math::abs ((int)i - (int)j);
      const float this_weight = exp (-distance * distance / gaussian_denominator);
      this_tangent += (tangents[j] * this_weight);
    }

    smoothed_tangents.push_back (this_tangent.normalise());

  }

  for (size_t i = 0; i != tck.size(); ++i) {

    // Risk of acos() returning NAN if the dot product is greater than 1.0
    float this_value;
    if (i == 0)
      this_value = acos (smoothed_tangents[ 1 ].dot (smoothed_tangents[ 0 ]))       / step_size;
    else if (i == tck.size() - 1)
      this_value = acos (smoothed_tangents[ i ].dot (smoothed_tangents[i-1]))       / step_size;
    else
      this_value = acos (smoothed_tangents[i+1].dot (smoothed_tangents[i-1])) * 0.5 / step_size;

    if (isnan (this_value))
      factors.push_back (0.0);
    else
      factors.push_back (this_value);

  }

}





template <class Cont>
void TrackMapperTWI<Cont>::gaussian_smooth_factors ()
{

  std::vector<float> unsmoothed (factors);

  for (size_t i = 0; i != unsmoothed.size(); ++i) {

    double sum = 0.0, norm = 0.0;

    if (std::isfinite (unsmoothed[i])) {
      sum  = unsmoothed[i];
      norm = 1.0; // Gaussian is unnormalised -> e^0 = 1
    }

    float distance = 0.0;
    for (size_t j = i; j--; ) { // Decrement AFTER null test, so loop_ runs with j = 0
      distance += step_size;
      if (std::isfinite (unsmoothed[j])) {
        const float this_weight = exp (-distance * distance / gaussian_denominator);
        norm += this_weight;
        sum  += this_weight * unsmoothed[j];
      }
    }
    distance = 0.0;
    for (size_t j = i + 1; j < unsmoothed.size(); ++j) {
      distance += step_size;
      if (std::isfinite (unsmoothed[j])) {
        const float this_weight = exp (-distance * distance / gaussian_denominator);
        norm += this_weight;
        sum  += this_weight * unsmoothed[j];
      }
    }

    if (norm)
      factors[i] = (sum / norm);
    else
      factors[i] = 0.0;

  }

}






template <class Cont>
void TrackMapperTWI<Cont>::set_factor (const std::vector< Point<float> >& tck, Cont& out)
{

  size_t count = 0;

  switch (contrast) {

    case TDI:         out.factor = 1.0; break;
    case PRECISE_TDI: out.factor = 1.0; break;
    case ENDPOINT:    out.factor = 1.0; break;
    case LENGTH:      out.factor = (step_size * (tck.size() - 1)); break;
    case INVLENGTH:   out.factor = (step_size / (tck.size() - 1)); break;

    case SCALAR_MAP:
    case SCALAR_MAP_COUNT:
    case FOD_AMP:
    case CURVATURE:

      factors.clear();
      factors.reserve (tck.size());
      load_factors (tck); // This should call the overloaded virtual function for TrackMapperImage

      switch (track_statistic) {

        case T_SUM:
          out.factor = 0.0;
          for (std::vector<float>::const_iterator i = factors.begin(); i != factors.end(); ++i) {
            if (std::isfinite (*i))
              out.factor += *i;
          }
          break;

        case T_MIN:
          out.factor = INFINITY;
          for (std::vector<float>::const_iterator i = factors.begin(); i != factors.end(); ++i) {
            if (std::isfinite (*i))
              out.factor = MIN (out.factor, *i);
          }
          break;

        case T_MEAN:
          out.factor = 0.0;
          for (std::vector<float>::const_iterator i = factors.begin(); i != factors.end(); ++i) {
            if (std::isfinite (*i)) {
              out.factor += *i;
              ++count;
            }
          }
          out.factor = (count ? (out.factor / float(count)) : 0.0);
          break;

        case T_MAX:
          out.factor = -INFINITY;
          for (std::vector<float>::const_iterator i = factors.begin(); i != factors.end(); ++i) {
            if (std::isfinite (*i))
              out.factor = MAX (out.factor, *i);
          }
          break;

        case T_MEDIAN:
          if (factors.empty()) {
            out.factor = 0.0;
          } else {
            nth_element (factors.begin(), factors.begin() + (factors.size() / 2), factors.end());
            out.factor = *(factors.begin() + (factors.size() / 2));
          }
          break;

        case T_MEAN_NONZERO:
          out.factor = 0.0;
          for (std::vector<float>::const_iterator i = factors.begin(); i != factors.end(); ++i) {
            if (std::isfinite (*i) && *i) {
              out.factor += *i;
              ++count;
            }
          }
          out.factor = (count ? (out.factor / float(count)) : 0.0);
          break;

        case GAUSSIAN:
          gaussian_smooth_factors();
          out.factor = 0.0;
          for (std::vector<float>::const_iterator i = factors.begin(); i != factors.end(); ++i) {
            if (*i) {
              out.factor = 1.0;
              break;
            }
          }
          break;

        case ENDS_MIN:
          assert (factors.size() == 2);
          out.factor = (Math::abs(factors[0]) < Math::abs(factors[1])) ? factors[0] : factors[1];
          break;

        case ENDS_MEAN:
          assert (factors.size() == 2);
          out.factor = 0.5 * (factors[0] + factors[1]);
          break;

        case ENDS_MAX:
          assert (factors.size() == 2);
          out.factor = (Math::abs(factors[0]) > Math::abs(factors[1])) ? factors[0] : factors[1];
          break;

        case ENDS_PROD:
          assert (factors.size() == 2);
          if ((factors[0] < 0.0 && factors[1] < 0.0) || (factors[0] > 0.0 && factors[1] > 0.0))
            out.factor = factors[0] * factors[1];
          else
            out.factor = 0.0;
          break;

        default:
          throw Exception ("FIXME: Undefined / unsupported track statistic in TrackMapperTWI::get_factor()");

      }
      break;

    default:
      throw Exception ("FIXME: Undefined / unsupported contrast mechanism in TrackMapperTWI::get_factor()");

  }

  if (contrast == SCALAR_MAP_COUNT)
    out.factor = (out.factor ? 1.0 : 0.0);

  if (!std::isfinite (out.factor))
    out.factor = 0.0;

}






template <class Cont>
class TrackMapperTWIImage : public TrackMapperTWI<Cont>
{

  typedef Image::BufferPreload<float>::voxel_type input_voxel_type;

  public:
    TrackMapperTWIImage (const Image::Header& template_image, const size_t upsample_ratio, const bool map_zero, const float step, const contrast_t c, const tck_stat_t s, const float denom, Image::BufferPreload<float>& input_image) :
        TrackMapperTWI<Cont> (template_image, upsample_ratio, map_zero, step, c, s, denom),
        voxel                (input_image),
        interp               (voxel),
        lmax                 (0),
        sh_coeffs            (NULL)
    {
      if (c == FOD_AMP) {
        lmax = Math::SH::LforN (voxel.dim(3));
        sh_coeffs = new float[voxel.dim(3)];
        precomputer.init (lmax);
      }
    }

    TrackMapperTWIImage (const TrackMapperTWIImage& that) :
        TrackMapperTWI<Cont> (that),
        voxel                (that.voxel),
        interp               (voxel),
        lmax                 (that.lmax),
        sh_coeffs            (NULL)
    {
      if (that.sh_coeffs) {
        sh_coeffs = new float[voxel.dim(3)];
        precomputer.init (lmax);
      }
    }

    ~TrackMapperTWIImage()
    {
      if (sh_coeffs) {
        delete[] sh_coeffs;
        sh_coeffs = NULL;
      }
    }


  private:
    input_voxel_type voxel;
    Image::Interp::Linear< input_voxel_type > interp;

    size_t lmax;
    float* sh_coeffs;
    Math::SH::PrecomputedAL<float> precomputer;

    void load_factors (const std::vector< Point<float> >&);

    // New helper function; find the last point on the streamline from which valid image information can be read
    const Point<float> get_last_point_in_fov (const std::vector< Point<float> >&, const bool);

};




template <class Cont>
void TrackMapperTWIImage<Cont>::load_factors (const std::vector< Point<float> >& tck)
{

  switch (TrackMapperTWI<Cont>::contrast) {

    case SCALAR_MAP:
    case SCALAR_MAP_COUNT:

      if (TrackMapperTWI<Cont>::track_statistic == ENDS_MIN || TrackMapperTWI<Cont>::track_statistic == ENDS_MEAN || TrackMapperTWI<Cont>::track_statistic == ENDS_MAX || TrackMapperTWI<Cont>::track_statistic == ENDS_PROD) { // Only the track endpoints contribute

        for (size_t tck_end_index = 0; tck_end_index != 2; ++tck_end_index) {
          const Point<float> endpoint = get_last_point_in_fov (tck, tck_end_index);
          if (endpoint.valid())
            TrackMapperTWI<Cont>::factors.push_back (interp.value());
          else
            TrackMapperTWI<Cont>::factors.push_back (NAN);
        }

      } else { // The entire length of the track contributes

        for (std::vector< Point<float> >::const_iterator i = tck.begin(); i != tck.end(); ++i) {
          if (!interp.scanner (*i))
            TrackMapperTWI<Cont>::factors.push_back (interp.value());
          else
            TrackMapperTWI<Cont>::factors.push_back (NAN);
        }

      }
      break;

    case FOD_AMP:
      for (size_t i = 0; i != tck.size(); ++i) {
        const Point<float>& p = tck[i];
        if (!interp.scanner (p)) {
          // Get the interpolated spherical harmonics at this point
          for (interp[3] = 0; interp[3] != interp.dim(3); ++interp[3])
            sh_coeffs[interp[3]] = interp.value();
          const Point<float> dir = (tck[(i == tck.size()-1) ? i : (i+1)] - tck[i ? (i-1) : 0]).normalise();
          TrackMapperTWI<Cont>::factors.push_back (precomputer.value (sh_coeffs, dir));
        } else {
          TrackMapperTWI<Cont>::factors.push_back (NAN);
        }
      }
      break;

    default:
      throw Exception ("FIXME: Undefined / unsupported contrast mechanism in TrackMapperTWIImage::load_factors()");

  }

}



template <class Cont>
const Point<float> TrackMapperTWIImage<Cont>::get_last_point_in_fov (const std::vector< Point<float> >& tck, const bool end)
{
  size_t index = end ? tck.size() - 1 : 0;
  const int step = end ? -1 : 1;
  while (interp.scanner (tck[index])) {
    index += step;
    if (index == tck.size())
      return Point<float>();
  }
  return tck[index];
}





}
}
}
}

#endif



