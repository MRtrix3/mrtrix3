/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __dwi_tractography_mapping_mapper_plugins_h__
#define __dwi_tractography_mapping_mapper_plugins_h__


#include "image.h"
#include "types.h"
#include "interp/linear.h"
#include "math/SH.h"

#include "dwi/directions/set.h"

#include "dwi/tractography/streamline.h"
#include "dwi/tractography/mapping/twi_stats.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Mapping {





        class DixelMappingPlugin
        { MEMALIGN(DixelMappingPlugin)
          public:
            DixelMappingPlugin (const DWI::Directions::FastLookupSet& directions) :
              dirs (directions) { }
            DixelMappingPlugin (const DixelMappingPlugin& that) :
              dirs (that.dirs) { }
            DWI::Directions::index_type operator() (const Eigen::Vector3& d) const { return dirs.select_direction (d); }
          private:
            const DWI::Directions::FastLookupSet& dirs;
        };



        class TODMappingPlugin
        { MEMALIGN(TODMappingPlugin)
          public:
            TODMappingPlugin (const size_t N) :
              generator (new Math::SH::aPSF<float> (Math::SH::LforN (N))) { }
            template <class VectorType, class UnitVectorType>
              void operator() (VectorType& sh, const UnitVectorType& d) const { (*generator) (sh, d); }
          private:
            std::shared_ptr<Math::SH::aPSF<float>> generator;
        };





        class TWIImagePluginBase
        { MEMALIGN(TWIImagePluginBase)

          public:
            TWIImagePluginBase (const std::string& input_image) :
              interp (Image<float>::open (input_image).with_direct_io()) { }

            virtual ~TWIImagePluginBase() { }

            virtual void load_factors (const Streamline<>&, vector<default_type>&) = 0;

          protected:
            //Image<float> voxel;
            // Each instance of the class has its own interpolator for obtaining values
            //   in a thread-safe fashion
            mutable Interp::Linear<Image<float>> interp;

            // New helper function; find the last point on the streamline from which valid image information can be read
            const Streamline<>::point_type get_last_point_in_fov (const Streamline<>&, const bool) const;

        };





        class TWIScalarImagePlugin : public TWIImagePluginBase
        { MEMALIGN(TWIScalarImagePlugin)
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


            void load_factors (const Streamline<>&, vector<default_type>&);

          private:
            const tck_stat_t statistic;

        };





        class TWIFODImagePlugin : public TWIImagePluginBase
        { MEMALIGN(TWIFODImagePlugin)
          public:
            TWIFODImagePlugin (const std::string& input_image) :
              TWIImagePluginBase (input_image),
              sh_coeffs (interp.size(3)),
              precomputer (new Math::SH::PrecomputedAL<default_type> ()) {
                Math::SH::check (Header (interp));
                precomputer->init (Math::SH::LforN (sh_coeffs.size()));
              }

            void load_factors (const Streamline<>&, vector<default_type>&);

          private:
            Eigen::Matrix<default_type, Eigen::Dynamic, 1> sh_coeffs;
            std::shared_ptr<Math::SH::PrecomputedAL<default_type>> precomputer;

        };








      }
    }
  }
}

#endif



