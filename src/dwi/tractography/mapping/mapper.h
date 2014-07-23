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
        map_zero  (false),
        precise   (false) { }

    TrackMapperBase (const Image::Info& template_image, const DWI::Directions::FastLookupSet& dirs) :
        upsampler    (1),
        dixel_plugin (new DixelMappingPlugin (dirs)),
        info         (template_image),
        transform    (info),
        map_zero     (false),
        precise      (false) { }

    TrackMapperBase (const TrackMapperBase& that) :
        upsampler    (1),
        dixel_plugin (that.dixel_plugin),
        info         (that.info),
        transform    (info),
        map_zero     (that.map_zero),
        precise      (that.precise) { }

    virtual ~TrackMapperBase() { }


    void set_upsample_ratio (const size_t i) { upsampler.set_ratio (i); }
    void set_map_zero (const bool i) { map_zero = i; }
    void set_use_precise_mapping (const bool i) { precise = i; }


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
        else
          voxelise (in, out);
        postprocess (in, out);
      }
      return true;
    }


  private:
    Upsampler<float> upsampler;

    RefPtr<DixelMappingPlugin> dixel_plugin;

    template <class Cont>
    void voxelise_precise (const Streamline<>&, Cont&) const;


  protected:
    const Image::Info info;
    Image::Transform transform;
    bool map_zero;
    bool precise;


    size_t get_upsample_factor() const { return upsampler.get_ratio(); }


    virtual void voxelise (const std::vector< Point<float> >&, SetVoxel&)    const;
    virtual void voxelise (const std::vector< Point<float> >&, SetVoxelDEC&) const;
    //virtual void voxelise (const std::vector< Point<float> >&, SetVoxelFactor&)    const { throw Exception ("Running empty virtual function TrackMapperBase::voxelise() (SetVoxelFactor)"); }
    //virtual void voxelise (const std::vector< Point<float> >&, SetVoxelDECFactor&) const { throw Exception ("Running empty virtual function TrackMapperBase::voxelise() (SetVoxelDECFactor)"); }
    virtual void voxelise (const std::vector< Point<float> >&, SetDixel&)          const { throw Exception ("Running empty virtual function TrackMapperBase::voxelise() (SetDixel)"); }

    virtual bool preprocess  (const std::vector< Point<float> >& tck, SetVoxelExtras& out) const { out.factor = 1.0; return true; }
    virtual void postprocess (const std::vector< Point<float> >& tck, SetVoxelExtras& out) const { }

    // Used by voxelise_precise to increment the relevant set
    void add_to_set (SetVoxel&   , const Voxel&, const Point<float>&, const float) const;
    void add_to_set (SetVoxelDEC&, const Voxel&, const Point<float>&, const float) const;
    void add_to_set (SetDixel&   , const Voxel&, const Point<float>&, const float) const;

};







template <class Cont>
void TrackMapperBase::voxelise_precise (const Streamline<>& tck, Cont& out) const
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
  Voxel next_voxel (round (transform.scanner2voxel (tck.front())));

  do {

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

      const PointF* p_one  = (p == 1)              ? &tck_proj_front : &tck[p - 2];
      const PointF* p_four = (p == tck.size() - 1) ? &tck_proj_back  : &tck[p + 1];

      PointF p_min = p_prev;
      PointF p_max = tck[p];

      while (dist2 (p_min, p_max) > accuracy) {

        mu = 0.5 * (mu_min + mu_max);
        hermite.set (mu);
        const PointF p_mu = hermite.value (*p_one, tck[p - 1], tck[p], *p_four);
        const Voxel mu_voxel = round (transform.scanner2voxel (p_mu));

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
    if (traversal_vector.valid() && check (this_voxel, info))
      add_to_set (out, this_voxel, traversal_vector, length);

  } while (!end_track);

}




class TrackMapperTWI : public TrackMapperBase
{
  public:
    TrackMapperTWI (const Image::Header& template_image, const contrast_t c, const tck_stat_t s) :
        TrackMapperBase       (template_image),
        contrast              (c),
        track_statistic       (s),
        //gaussian_denominator  (0.0),
        image_plugin          (NULL) { }

    TrackMapperTWI (const TrackMapperTWI& that) :
        TrackMapperBase       (that),
        contrast              (that.contrast),
        track_statistic       (that.track_statistic),
        //gaussian_denominator  (that.gaussian_denominator),
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

/*
    void set_gaussian_FWHM   (const float FWHM)
    {
      if (track_statistic != GAUSSIAN)
        throw Exception ("Cannot set Gaussian FWHM unless the track statistic is Gaussian");
      const float theta = FWHM / (2.0 * sqrt (2.0 * log (2.0)));
      gaussian_denominator = 2.0 * Math::pow2 (theta);
    }
*/

    void add_scalar_image (const std::string&);
    void add_fod_image    (const std::string&);



  protected:
    virtual void load_factors (const std::vector< Point<float> >&) const;
    const contrast_t contrast;
    const tck_stat_t track_statistic;

    // Members required for when Gaussian smoothing is performed along the track
    //float gaussian_denominator;
    //void gaussian_smooth_factors (const std::vector< Point<float> >&) const;

    // Member for when the contribution of a track is not constant along its length
    mutable std::vector<float> factors;

    // Member for incorporating additional information from an external image into the TWI process
    TWIImagePluginBase* image_plugin;


  private:

    void set_factor (const std::vector< Point<float> >&, SetVoxelExtras&) const;


    void voxelise (const std::vector< Point<float> >& tck, SetVoxel&          voxels) const;
    void voxelise (const std::vector< Point<float> >& tck, SetVoxelDEC&       voxels) const;
    //void voxelise (const std::vector< Point<float> >& tck, SetVoxelFactor&    voxels) const;
    //void voxelise (const std::vector< Point<float> >& tck, SetVoxelDECFactor& voxels) const;
    void voxelise (const std::vector< Point<float> >& tck, SetDixel&          voxels) const { throw Exception ("Running empty virtual function TrackMapperTWI::voxelise() (SetDixel)"); }

    // Overload virtual function
    bool preprocess (const std::vector< Point<float> >& tck, SetVoxelExtras& out) const { set_factor (tck, out); return out.factor; }



};






}
}
}
}

#endif



