/* Copyright (c) 2008-2019 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __dwi_tractography_tracking_method_h__
#define __dwi_tractography_tracking_method_h__

#include "memory.h"
#include "types.h"
#include "dwi/tractography/rng.h"
#include "dwi/tractography/tracking/shared.h"
#include "dwi/tractography/ACT/method.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Tracking
      {



        class MethodBase { MEMALIGN(MethodBase)

          public:

            MethodBase (const SharedBase& shared) :
              pos (0.0, 0.0, 0.0),
              dir (0.0, 0.0, 1.0),
              S (shared),
              act_method_additions (S.is_act() ? new ACT::ACT_Method_additions (S) : nullptr),
              values (shared.source.size(3)) { }

            MethodBase (const MethodBase& that) :
              pos (0.0, 0.0, 0.0),
              dir (0.0, 0.0, 1.0),
              S (that.S),
              act_method_additions (S.is_act() ? new ACT::ACT_Method_additions (S) : nullptr),
              uniform (that.uniform),
              values (that.values.size()) { }


            template <class InterpolatorType>
            FORCE_INLINE bool get_data (InterpolatorType& source, const Eigen::Vector3f& position)
            {
              if (!source.scanner (position))
                return false;
              for (auto l = Loop (3) (source); l; ++l)
                values[source.index(3)] = source.value();
              return !std::isnan (values[0]);
            }

            template <class InterpolatorType>
            FORCE_INLINE bool get_data (InterpolatorType& source)
            {
              return get_data (source, pos);
            }


            virtual bool init() = 0;
            virtual term_t next() = 0;
            virtual float get_metric() = 0;

            virtual void reverse_track() { if (act_method_additions) act().reverse_track(); }
            virtual void truncate_track (GeneratedTrack& tck, const size_t length_to_revert_from, const size_t revert_step);

            bool check_seed();

            ACT::ACT_Method_additions& act() const { return *act_method_additions; }

            Eigen::Vector3f pos, dir;


          private:
            const SharedBase& S;
            std::unique_ptr<ACT::ACT_Method_additions> act_method_additions;


          protected:
            std::uniform_real_distribution<float> uniform;
            Eigen::VectorXf values;

            Eigen::Vector3f random_direction ();
            Eigen::Vector3f random_direction (const float max_angle, const float sin_max_angle);
            Eigen::Vector3f rotate_direction (const Eigen::Vector3f& reference, const Eigen::Vector3f& direction);

            FORCE_INLINE Eigen::Vector3f random_direction (const Eigen::Vector3f& d, const float max_angle, const float sin_max_angle)
            {
              return rotate_direction (d, random_direction (max_angle, sin_max_angle));
            }

        };



      }
    }
  }
}

#endif



