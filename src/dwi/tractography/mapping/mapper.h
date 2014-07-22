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


    template <class Cont>
    bool operator() (Streamline<float>& in, Cont& out) const
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
    const Image::Info info;
    Image::Transform transform;
    const bool map_zero;


    size_t get_upsample_factor() const { return upsampler.get_ratio(); }


    void voxelise (const std::vector< Point<float> >&, SetVoxel&)    const;
    void voxelise (const std::vector< Point<float> >&, SetVoxelDEC&) const;
    void voxelise (const std::vector< Point<float> >&, SetVoxelDir&) const;
    virtual void voxelise (const std::vector< Point<float> >&, SetVoxelFactor&)    const { throw Exception ("Running empty virtual function TrackMapperBase::voxelise() (SetVoxelFactor)"); }
    virtual void voxelise (const std::vector< Point<float> >&, SetVoxelDECFactor&) const { throw Exception ("Running empty virtual function TrackMapperBase::voxelise() (SetVoxelDECFactor)"); }
    virtual void voxelise (const std::vector< Point<float> >&, SetDixel&)          const { throw Exception ("Running empty virtual function TrackMapperBase::voxelise() (SetDixel)"); }

    virtual bool preprocess  (const std::vector< Point<float> >& tck, SetVoxelExtras& out) const { out.factor = 1.0; return true; }
    virtual void postprocess (const std::vector< Point<float> >& tck, SetVoxelExtras& out) const { }

};







class TrackMapperDixel : public TrackMapperBase
{
  public:
    TrackMapperDixel (const Image::Header& template_image, const size_t upsample_ratio, const bool map_zero, const DWI::Directions::FastLookupSet& directions) :
        TrackMapperBase (template_image, upsample_ratio, map_zero),
        dirs (directions) { }

  protected:
    const DWI::Directions::FastLookupSet& dirs;

  private:
    void voxelise (const std::vector< Point<float> >&, SetDixel&) const;

};




class TrackMapperTWI : public TrackMapperBase
{
  public:
    TrackMapperTWI (const Image::Header& template_image, const size_t upsample_ratio, const bool map_zero, const float step, const contrast_t c, const tck_stat_t s, const float denom = 0.0) :
        TrackMapperBase       (template_image, upsample_ratio, map_zero),
        contrast              (c),
        track_statistic       (s),
        gaussian_denominator  (denom),
        step_size             (step) { }

    TrackMapperTWI (const TrackMapperTWI& that) :
        TrackMapperBase       (that),
        contrast              (that.contrast),
        track_statistic       (that.track_statistic),
        gaussian_denominator  (that.gaussian_denominator),
        step_size             (that.step_size) { }

    virtual ~TrackMapperTWI() { }


  protected:
    virtual void load_factors (const std::vector< Point<float> >&) const;
    const contrast_t contrast;
    const tck_stat_t track_statistic;

    // Members for when the contribution of a track is not constant along its length (i.e. Gaussian smoothed along the track)
    const float gaussian_denominator;
    mutable std::vector<float> factors;
    void gaussian_smooth_factors() const;


  private:
    const float step_size;


    void set_factor (const std::vector< Point<float> >&, SetVoxelExtras&) const;


    void voxelise (const std::vector< Point<float> >& tck, SetVoxel&          voxels) const;
    void voxelise (const std::vector< Point<float> >& tck, SetVoxelDEC&       voxels) const;
    void voxelise (const std::vector< Point<float> >& tck, SetVoxelDir&       voxels) const { throw Exception ("Running empty virtual function TrackMapperTWI::voxelise() (SetVoxelDir)"); }
    void voxelise (const std::vector< Point<float> >& tck, SetVoxelFactor&    voxels) const;
    void voxelise (const std::vector< Point<float> >& tck, SetVoxelDECFactor& voxels) const;
    void voxelise (const std::vector< Point<float> >& tck, SetDixel&          voxels) const { throw Exception ("Running empty virtual function TrackMapperTWI::voxelise() (SetDixel)"); }

    // Overload virtual function
    bool preprocess (const std::vector< Point<float> >& tck, SetVoxelExtras& out) const { set_factor (tck, out); return out.factor; }

};





class TrackMapperTWIImage : public TrackMapperTWI
{

  typedef Image::BufferPreload<float>::voxel_type input_voxel_type;

  public:
    TrackMapperTWIImage (const Image::Header& template_image, const size_t upsample_ratio, const bool map_zero, const float step, const contrast_t c, const tck_stat_t s, const float denom, Image::BufferPreload<float>& input_image) :
        TrackMapperTWI       (template_image, upsample_ratio, map_zero, step, c, s, denom),
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
        TrackMapperTWI       (that),
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
    mutable Image::Interp::Linear< input_voxel_type > interp;

    size_t lmax;
    float* sh_coeffs;
    Math::SH::PrecomputedAL<float> precomputer;

    void load_factors (const std::vector< Point<float> >&) const;

    // New helper function; find the last point on the streamline from which valid image information can be read
    const Point<float> get_last_point_in_fov (const std::vector< Point<float> >&, const bool) const;

};







}
}
}
}

#endif



