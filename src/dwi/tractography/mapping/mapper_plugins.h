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
#define __dwi_tractography_mapping_mapper_h__


#include <vector>

#include "point.h"
#include "ptr.h"
#include "image/buffer_preload.h"
#include "image/voxel.h"
#include "image/interp/linear.h"
#include "math/SH.h"

#include "dwi/tractography/mapping/twi_stats.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Mapping {




class TWIImagePluginBase
{

    typedef Image::BufferPreload<float>::voxel_type input_voxel_type;

  public:
    TWIImagePluginBase (const std::string& input_image) :
        data (new Image::BufferPreload<float> (input_image)),
        voxel (*data),
        interp (voxel) { }

    TWIImagePluginBase (const TWIImagePluginBase& that) :
        data (that.data),
        voxel (that.voxel),
        interp (voxel) { }

    virtual ~TWIImagePluginBase() { }


    virtual void load_factors (const std::vector< Point<float> >&, std::vector<float>&) const = 0;


  protected:
    RefPtr< Image::BufferPreload<float> > data;
    const input_voxel_type voxel;
    // Each instance of the class has its own interpolator for obtaining values
    //   in a thread-safe fashion
    mutable Image::Interp::Linear< input_voxel_type > interp;

    // New helper function; find the last point on the streamline from which valid image information can be read
    const Point<float> get_last_point_in_fov (const std::vector< Point<float> >&, const bool) const;

};





class TWIScalarImagePlugin : public TWIImagePluginBase
{
  public:
    TWIScalarImagePlugin (const std::string& input_image, const tck_stat_t track_statistic) :
        TWIImagePluginBase (input_image),
        statistic (track_statistic)
    {
      if (data->ndim() != 3)
        throw Exception ("Scalar image used for TWI must be a 3D image");
    }

    TWIScalarImagePlugin (const TWIScalarImagePlugin& that) :
        TWIImagePluginBase (that),
        statistic (that.statistic) { }

    ~TWIScalarImagePlugin() { }


    void load_factors (const std::vector< Point<float> >&, std::vector<float>&) const;

  private:
    const tck_stat_t statistic;

};





class TWIFODImagePlugin : public TWIImagePluginBase
{
  public:
    TWIFODImagePlugin (const std::string& input_image) :
        TWIImagePluginBase (input_image),
        sh_coeffs (NULL),
        precomputer (new Math::SH::PrecomputedAL<float> ())
    {
      if (data->ndim() != 4)
        throw Exception ("Image used for TWI with FOD_AMP contrast be a 4D image");
      lmax = Math::SH::LforN (data->dim (3));
      N = Math::SH::NforL (lmax);
      if (int(N) != data->dim (3))
        throw Exception ("Image used for TWI with FOD_AMP contrast be an SH image");
      sh_coeffs = new float[N];
      precomputer->init (lmax);
    }

    TWIFODImagePlugin (const TWIFODImagePlugin& that) :
        TWIImagePluginBase (that),
        lmax (that.lmax),
        N (that.N),
        sh_coeffs (new float[N]),
        precomputer (that.precomputer) { }

    ~TWIFODImagePlugin()
    {
      if (sh_coeffs) {
        delete[] sh_coeffs;
        sh_coeffs = NULL;
      }
    }


    void load_factors (const std::vector< Point<float> >&, std::vector<float>&) const;


  private:
    size_t lmax, N;
    float* sh_coeffs;
    RefPtr< Math::SH::PrecomputedAL<float> > precomputer;

};








}
}
}
}

#endif



