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

#ifndef __dwi_tractography_mapping_mapper_plugins_h__
#define __dwi_tractography_mapping_mapper_plugins_h__


#include <vector>

#include "point.h"
#include "image/buffer_preload.h"
#include "image/buffer_scratch.h"
#include "image/voxel.h"
#include "image/interp/linear.h"
#include "math/SH.h"
#include "math/vector.h"

#include "dwi/directions/set.h"

#include "dwi/tractography/mapping/twi_stats.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {





class DixelMappingPlugin
{
  public:
    DixelMappingPlugin (const DWI::Directions::FastLookupSet& directions) :
        dirs (directions) { }
    DixelMappingPlugin (const DixelMappingPlugin& that) :
        dirs (that.dirs) { }
    size_t operator() (const Point<float>& d) const { return dirs.select_direction (d); }
  private:
    const DWI::Directions::FastLookupSet& dirs;
};



class TODMappingPlugin
{
  public:
    TODMappingPlugin (const size_t N) :
        generator (new Math::SH::aPSF<float> (Math::SH::LforN (N))) { }
    TODMappingPlugin (const TODMappingPlugin& that) :
        generator (that.generator) { }
    void operator() (Math::Vector<float>& sh, const Point<float>& d) const { (*generator) (sh, d); }
  private:
    std::shared_ptr< Math::SH::aPSF<float> > generator;
};





class TWIImagePluginBase
{

  public:
    TWIImagePluginBase (const std::string& input_image, const tck_stat_t track_statistic) :
        statistic (track_statistic),
        data (new Image::BufferPreload<float> (input_image)),
        voxel (*data),
        interp (voxel),
        backtrack (false) { }

    TWIImagePluginBase (const TWIImagePluginBase& that) :
        statistic (that.statistic),
        data (that.data),
        voxel (that.voxel),
        interp (voxel),
        backtrack (that.backtrack),
        backtrack_mask (that.backtrack_mask) { }

    virtual ~TWIImagePluginBase() { }


    void set_backtrack();

    virtual void load_factors (const std::vector< Point<float> >&, std::vector<float>&) const = 0;


  protected:
    typedef Image::BufferPreload<float>::voxel_type input_voxel_type;

    const tck_stat_t statistic;

    std::shared_ptr< Image::BufferPreload<float> > data;
    const input_voxel_type voxel;
    // Each instance of the class has its own interpolator for obtaining values
    //   in a thread-safe fashion
    mutable Image::Interp::Linear<input_voxel_type> interp;

    // If the streamline endpoint doesn't contain a valid value, but we want a valid value
    //   at all endpoints, should we backtrack to the first streamline point that _does_ have
    //   a valid value, or should it provide a value of zero?
    // If using ENDS_CORR, pre-calculate a mask of voxels that have valid time series
    bool backtrack;
    std::shared_ptr<Image::BufferScratch<bool>> backtrack_mask;

    // New helper function; find the last point on the streamline from which valid image information can be read
    const ssize_t get_end_index (const std::vector< Point<float> >&, const bool) const;
    const Point<float> get_end_point (const std::vector< Point<float> >&, const bool) const;

};





class TWIScalarImagePlugin : public TWIImagePluginBase
{
  public:
    TWIScalarImagePlugin (const std::string& input_image, const tck_stat_t track_statistic) :
        TWIImagePluginBase (input_image, track_statistic)
    {
      if (track_statistic == ENDS_CORR) {
        if (data->ndim() != 4)
          throw Exception ("Image provided for ends_corr statistic must be a 4D image");
      } else {
        if (!((data->ndim() == 3) || (data->ndim() == 4 && data->dim(3) == 1)))
          throw Exception ("Scalar image used for TWI must be a 3D image");
        if (data->ndim() == 4)
          interp[3] = 0;
      }
    }

    TWIScalarImagePlugin (const TWIScalarImagePlugin& that) :
        TWIImagePluginBase (that)
    {
      if (data->ndim() == 4)
        interp[3] = 0;
    }

    ~TWIScalarImagePlugin() { }

    void load_factors (const std::vector< Point<float> >&, std::vector<float>&) const;

  private:


};





class TWIFODImagePlugin : public TWIImagePluginBase
{
  public:
    TWIFODImagePlugin (const std::string& input_image, const tck_stat_t track_statistic) :
        TWIImagePluginBase (input_image, track_statistic),
        N (data->dim(3)),
        sh_coeffs (new float[N]),
        precomputer (new Math::SH::PrecomputedAL<float> ())
    {
      if (track_statistic == ENDS_CORR)
        throw Exception ("Cannot use ends_corr track statistic with an FOD image");
      Math::SH::check (*data);
      precomputer->init (Math::SH::LforN (N));
    }

    TWIFODImagePlugin (const TWIFODImagePlugin& that) :
        TWIImagePluginBase (that),
        N (that.N),
        sh_coeffs (new float[N]),
        precomputer (that.precomputer) { }

    ~TWIFODImagePlugin()
    {
      delete[] sh_coeffs;
      sh_coeffs = NULL;
    }


    void load_factors (const std::vector< Point<float> >&, std::vector<float>&) const;


  private:
    const size_t N;
    float* sh_coeffs;
    std::shared_ptr< Math::SH::PrecomputedAL<float> > precomputer;

};








}
}
}
}

#endif



