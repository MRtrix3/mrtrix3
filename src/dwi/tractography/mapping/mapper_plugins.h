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

#include "image.h"
#include "interp/linear.h"
#include "math/SH.h"

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
            size_t operator() (const Eigen::Vector3f& d) const { return dirs.select_direction (d); }
          private:
            const DWI::Directions::FastLookupSet& dirs;
        };



        class TODMappingPlugin
        {
          public:
            TODMappingPlugin (const size_t N) :
              generator (new Math::SH::aPSF<float> (Math::SH::LforN (N))) { }
            template <class VectorType, class UnitVectorType>
              void operator() (VectorType& sh, const UnitVectorType& d) const { (*generator) (sh, d); }
          private:
            std::shared_ptr<Math::SH::aPSF<float>> generator;
        };





        class TWIImagePluginBase
        {

          public:
            TWIImagePluginBase (const std::string& input_image) :
              interp (Image<float>::open (input_image).with_direct_io()) { }

            virtual ~TWIImagePluginBase() { }

            virtual void load_factors (const std::vector<Eigen::Vector3f>&, std::vector<float>&) = 0;

          protected:
            //Image<float> voxel;
            // Each instance of the class has its own interpolator for obtaining values
            //   in a thread-safe fashion
            mutable Interp::Linear<Image<float>> interp;

            // New helper function; find the last point on the streamline from which valid image information can be read
            const Eigen::Vector3f get_last_point_in_fov (const std::vector<Eigen::Vector3f>&, const bool) const;

        };





        class TWIScalarImagePlugin : public TWIImagePluginBase
        {
          public:
            TWIScalarImagePlugin (const std::string& input_image, const tck_stat_t track_statistic) :
              TWIImagePluginBase (input_image),
              statistic (track_statistic) {
                if (!((interp.ndim() == 3) || (interp.ndim() == 4 && interp.size(3) == 1)))
                  throw Exception ("Scalar image used for TWI must be a 3D image");
                if (interp.ndim() == 4)
                  interp.index(3) = 0;
              }

            TWIScalarImagePlugin (const TWIScalarImagePlugin& that) :
              TWIImagePluginBase (that),
              statistic (that.statistic) {
                if (interp.ndim() == 4)
                  interp.index(3) = 0;
              }

            ~TWIScalarImagePlugin() { }


            void load_factors (const std::vector<Eigen::Vector3f>&, std::vector<float>&);

          private:
            const tck_stat_t statistic;

        };





        class TWIFODImagePlugin : public TWIImagePluginBase
        {
          public:
            TWIFODImagePlugin (const std::string& input_image) :
              TWIImagePluginBase (input_image),
              sh_coeffs (interp.size(3)),
              precomputer (new Math::SH::PrecomputedAL<float> ()) {
                Math::SH::check (Header (interp));
                precomputer->init (Math::SH::LforN (sh_coeffs.size()));
              }

            void load_factors (const std::vector<Eigen::Vector3f>&, std::vector<float>&);

          private:
            Eigen::VectorXf sh_coeffs;
            std::shared_ptr<Math::SH::PrecomputedAL<float>> precomputer;

        };








      }
    }
  }
}

#endif



