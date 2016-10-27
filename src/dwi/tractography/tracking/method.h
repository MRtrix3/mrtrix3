/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __dwi_tractography_tracking_method_h__
#define __dwi_tractography_tracking_method_h__

#include "memory.h"
#include "dwi/tractography/rng.h"
#include "dwi/tractography/tracking/shared.h"
#include "dwi/tractography/ACT/method.h"
#include "dwi/tractography/MACT/method.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Tracking
      {



        class MethodBase {

          public:

            MethodBase (const SharedBase& shared) :
              pos (0.0, 0.0, 0.0),
              dir (0.0, 0.0, 1.0),
              S (shared),
              act_method_additions (S.is_act() ? new ACT::ACT_Method_additions (S) : nullptr),
              mact_method_additions (S.is_mact() ? new MACT::MACT_Method_additions (S) : nullptr),
              values (shared.source.size(3)) { }

            MethodBase (const MethodBase& that) :
              pos (0.0, 0.0, 0.0),
              dir (0.0, 0.0, 1.0),
              S (that.S),
              act_method_additions (S.is_act() ? new ACT::ACT_Method_additions (S) : nullptr),
              mact_method_additions (S.is_mact() ? new MACT::MACT_Method_additions (S) : nullptr),
              uniform (that.uniform),
              values (that.values.size()) { }



            bool check_seed()
            {
              if (!pos.allFinite())
                return false;

              if ((S.properties.mask.size() && !S.properties.mask.contains (pos))
                  || (S.properties.exclude.contains (pos))
                  || (S.is_act() && !act().check_seed (pos))
                  || (S.is_mact() && !mact().check_seed (pos))) {
                pos = { NaN, NaN, NaN };
                return false;
              }

              return true;
            }


            template <class InterpolatorType>
              inline bool get_data (InterpolatorType& source, const Eigen::Vector3f& position)
              {
                if (!source.scanner (position))
                  return false;
                for (auto l = Loop (3) (source); l; ++l)
                  values[source.index(3)] = source.value();
                return !std::isnan (values[0]);
              }

            template <class InterpolatorType>
              inline bool get_data (InterpolatorType& source) {
                return get_data (source, pos);
              }


            virtual void reverse_track()
            {
              if (act_method_additions) act().reverse_track();
              if (mact_method_additions) mact().reverse_track();
            }

            bool init() { return false; }
            term_t next() { return term_t(); }
            float get_metric() { return NaN; }


            void truncate_track (GeneratedTrack& tck, const size_t length_to_revert_from, const size_t revert_step)
            {
              if (tck.get_seed_index() + revert_step >= length_to_revert_from) {
                tck.clear();
                pos = { NaN, NaN, NaN };
                dir = { NaN, NaN, NaN };
                return;
              }
              const size_t new_size = length_to_revert_from - revert_step;
              if (tck.size() == 2 || new_size == 1)
                dir = (tck[1] - tck[0]).normalized();
              else
                dir = (tck[new_size] - tck[new_size - 2]).normalized();
              tck.resize (length_to_revert_from - revert_step);
              pos = tck.back();
              if (S.is_act())
                act().sgm_depth = (act().sgm_depth > revert_step) ? act().sgm_depth - revert_step : 0;
              if (S.is_mact())
                mact()._sgm_depth = (mact()._sgm_depth > revert_step) ? mact()._sgm_depth - revert_step : 0;
            }


            ACT::ACT_Method_additions& act() const { return *act_method_additions; }
            MACT::MACT_Method_additions& mact() const { return *mact_method_additions; }

            Eigen::Vector3f pos, dir;


          private:
            const SharedBase& S;
            std::unique_ptr<ACT::ACT_Method_additions> act_method_additions;
            std::unique_ptr<MACT::MACT_Method_additions> mact_method_additions;

          protected:
            std::uniform_real_distribution<float> uniform;
            Eigen::VectorXf values;

            Eigen::Vector3f random_direction ();
            Eigen::Vector3f random_direction (float max_angle, float sin_max_angle);
            Eigen::Vector3f random_direction (const Eigen::Vector3f& d, float max_angle, float sin_max_angle);
            Eigen::Vector3f rotate_direction (const Eigen::Vector3f& reference, const Eigen::Vector3f& direction);

        };






        Eigen::Vector3f MethodBase::random_direction ()
        {
          Eigen::Vector3f d;
          do {
            d[0] = 2.0 * uniform(*rng) - 1.0;
            d[1] = 2.0 * uniform(*rng) - 1.0;
            d[2] = 2.0 * uniform(*rng) - 1.0;
          } while (d.squaredNorm() > 1.0);
          d.normalize();
          return d;
        }


        Eigen::Vector3f MethodBase::random_direction (float max_angle, float sin_max_angle)
        {
          float phi = 2.0 * Math::pi * uniform(*rng);
          float theta;
          do {
            theta = max_angle * uniform(*rng);
          } while (sin_max_angle * uniform(*rng) > sin (theta));
          return Eigen::Vector3f (sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta));
        }


        Eigen::Vector3f MethodBase::random_direction (const Eigen::Vector3f& d, float max_angle, float sin_max_angle)
        {
          return rotate_direction (d, random_direction (max_angle, sin_max_angle));
        }


        Eigen::Vector3f MethodBase::rotate_direction (const Eigen::Vector3f& reference, const Eigen::Vector3f& direction)
        {
          float n = std::sqrt (Math::pow2(reference[0]) + Math::pow2(reference[1]));
          if (n == 0.0) {
            if (reference[2] < 0.0) 
              return -direction;
            else 
              return direction;
          }

          Eigen::Vector3f m (reference[0]/n, reference[1]/n, 0.0f);
          Eigen::Vector3f mp (reference[2]*m[0], reference[2]*m[1], -n);

          float alpha = direction[2];
          float beta = direction[0]*m[0] + direction[1]*m[1];

          return { 
            direction[0] + alpha * reference[0] + beta * (mp[0] - m[0]),
              direction[1] + alpha * reference[1] + beta * (mp[1] - m[1]),
              direction[2] + alpha * (reference[2]-1.0f) + beta * (mp[2] - m[2])
          };
        }


      }
    }
  }
}

#endif



