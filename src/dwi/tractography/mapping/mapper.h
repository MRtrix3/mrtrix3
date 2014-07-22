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
#include "thread/queue.h"

#include "dwi/directions/set.h"

#include "dwi/tractography/resample.h"
#include "dwi/tractography/streamline.h"

#include "dwi/tractography/mapping/mapper_plugins.h"
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
    TrackMapperBase (const Image::Info& template_image) :
        upsampler (1),
        info      (template_image),
        transform (info),
        map_zero  (false) { }

    TrackMapperBase (const TrackMapperBase& that) :
        upsampler (1),
        info      (that.info),
        transform (info),
        map_zero  (that.map_zero) { }

    virtual ~TrackMapperBase() { }


    void set_upsample_ratio (const size_t i) { upsampler.set_ratio (i); }
    void set_map_zero (const bool i) { map_zero = i; }


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
    bool map_zero;


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
    TrackMapperDixel (const Image::Header& template_image, const DWI::Directions::FastLookupSet& directions) :
        TrackMapperBase (template_image),
        dirs (directions) { }

  protected:
    const DWI::Directions::FastLookupSet& dirs;

  private:
    void voxelise (const std::vector< Point<float> >&, SetDixel&) const;

};




class TrackMapperTWI : public TrackMapperBase
{
  public:
    TrackMapperTWI (const Image::Header& template_image, const contrast_t c, const tck_stat_t s) :
        TrackMapperBase       (template_image),
        contrast              (c),
        track_statistic       (s),
        gaussian_denominator  (0.0),
        image_plugin          (NULL) { }

    TrackMapperTWI (const TrackMapperTWI& that) :
        TrackMapperBase       (that),
        contrast              (that.contrast),
        track_statistic       (that.track_statistic),
        gaussian_denominator  (that.gaussian_denominator),
        image_plugin          (NULL)
    {
      if (that.image_plugin) {
        if (contrast == SCALAR_MAP || contrast == SCALAR_MAP_COUNT)
          image_plugin = new TWIScalarImagePlugin (*dynamic_cast<TWIScalarImagePlugin*> (that.image_plugin));
        else if (contrast == FOD_AMP)
          image_plugin = new TWIFODImagePlugin    (*dynamic_cast<TWIFODImagePlugin*>    (that.image_plugin));
        else
          throw Exception ("Copy-constructing TrackMapperTWI with unknown image plugin");
      }
    }

    virtual ~TrackMapperTWI()
    {
      if (image_plugin) {
        delete image_plugin;
        image_plugin = NULL;
      }
    }


    void set_gaussian_FWHM   (const float FWHM)
    {
      if (track_statistic != GAUSSIAN)
        throw Exception ("Cannot set Gaussian FWHM unless the track statistic is Gaussian");
      const float theta = FWHM / (2.0 * sqrt (2.0 * log (2.0)));
      gaussian_denominator = 2.0 * Math::pow2 (theta);
    }

    void add_scalar_image (const std::string&);
    void add_fod_image    (const std::string&);



  protected:
    virtual void load_factors (const std::vector< Point<float> >&) const;
    const contrast_t contrast;
    const tck_stat_t track_statistic;

    // Members required for when Gaussian smoothing is performed along the track
    float gaussian_denominator; // Only for Gaussian-smoothed
    void gaussian_smooth_factors (const std::vector< Point<float> >&) const;

    // Member for when the contribution of a track is not constant along its length
    mutable std::vector<float> factors;

    // Member for incorporating additional information from an external image into the TWI process
    TWIImagePluginBase* image_plugin;


  private:

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






}
}
}
}

#endif



