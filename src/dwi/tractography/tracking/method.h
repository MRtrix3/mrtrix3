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

#ifndef __dwi_tractography_tracking_method_h__
#define __dwi_tractography_tracking_method_h__

#include "memory.h"
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



    class MethodBase {

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
          uniform_rng (that.uniform_rng),
          values (that.values.size()) { }



        bool check_seed()
        {
          if (!std::isfinite (pos[0]))
            return false;

          if ((S.properties.mask.size() && !S.properties.mask.contains (pos))
              || (S.properties.exclude.contains (pos))
              || (S.is_act() && !act().check_seed (pos))) {
            pos = { NaN, NaN, NaN };
            return false;
          }

          return true;
        }


        template <class InterpolatorType>
          inline bool get_data (InterpolatorType& source, const Eigen::Vector3f& position)
          {
            source.scanner (position);
            if (!source) return false;
            for (auto l = Loop (3) (source); l; ++l)
              values[source[3]] = source.value();
            return !std::isnan (values[0]);
          }

        template <class InterpolatorType>
          inline bool get_data (InterpolatorType& source) {
            return get_data (source, pos);
          }


        void reverse_track() { }
        bool init() { return false; }
        term_t next() { return term_t(); }
        float get_metric() { return NaN; }


        void truncate_track (std::vector< Eigen::Vector3f >& tck, const size_t revert_step)
        {
          for (size_t i = revert_step; i && tck.size(); --i)
            tck.pop_back();
          if (S.is_act())
            act().sgm_depth = MAX (0, act().sgm_depth - int(revert_step));
        }


        ACT::ACT_Method_additions& act() const { return *act_method_additions; }

        Eigen::Vector3f pos, dir;


      private:
        const SharedBase& S;
        std::unique_ptr<ACT::ACT_Method_additions> act_method_additions;


      protected:
        Math::RNG::Uniform<float> uniform_rng;
        std::vector<float> values;

        Eigen::Vector3f random_direction ();
        Eigen::Vector3f random_direction (float max_angle, float sin_max_angle);
        Eigen::Vector3f random_direction (const Eigen::Vector3f& d, float max_angle, float sin_max_angle);
        Eigen::Vector3f rotate_direction (const Eigen::Vector3f& reference, const Eigen::Vector3f& direction);

    };






    Eigen::Vector3f MethodBase::random_direction ()
    {
      Eigen::Vector3f d;
      do {
        d[0] = 2.0 * uniform_rng() - 1.0;
        d[1] = 2.0 * uniform_rng() - 1.0;
        d[2] = 2.0 * uniform_rng() - 1.0;
      } while (d.squaredNorm() > 1.0);
      d.normalize();
      return d;
    }


    Eigen::Vector3f MethodBase::random_direction (float max_angle, float sin_max_angle)
    {
      float phi = 2.0 * Math::pi * uniform_rng();
      float theta;
      do {
        theta = max_angle * uniform_rng();
      } while (sin_max_angle * uniform_rng() > sin (theta));
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



