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
          pos                (0.0, 0.0, 0.0),
          dir                (0.0, 0.0, 1.0),
          S                  (shared),
          values             (shared.source_buffer.dim(3))
        {
          if (S.is_act())
            act_method_additions.reset (new ACT::ACT_Method_additions (S));
        }

        MethodBase (const MethodBase& that) :
          pos                 (0.0, 0.0, 0.0),
          dir                 (0.0, 0.0, 1.0),
          S                   (that.S),
          uniform_rng         (that.uniform_rng),
          values              (that.values.size())
        {
          if (S.is_act())
            act_method_additions.reset (new ACT::ACT_Method_additions (S));
        }


        bool check_seed()
        {
          if (!pos.valid())
            return false;

          if ((S.properties.mask.size() && !S.properties.mask.contains (pos))
              || (S.properties.exclude.contains (pos))
              || (S.is_act() && !act().check_seed (pos))) {
            pos.invalidate();
            return false;
          }

          return true;
        }


        template <class InterpolatorType>
        inline bool get_data (InterpolatorType& source, const Point<value_type>& position)
        {
            source.scanner (position);
            if (!source) return (false);
            for (source[3] = 0; source[3] < source.dim(3); ++source[3])
              values[source[3]] = source.value();
            return (!std::isnan (values[0]));
        }

        template <class InterpolatorType>
        inline bool get_data (InterpolatorType& source) {
            return (get_data (source, pos));
        }


        virtual void reverse_track() { if (act_method_additions) act().reverse_track(); }
        bool init() { return false; }
        term_t next() { return term_t(); }
        float get_metric() { return NAN; }


        void truncate_track (GeneratedTrack& tck, const size_t length_to_revert_from, const size_t revert_step)
        {
          if (tck.get_seed_index() + revert_step >= length_to_revert_from) {
            tck.clear();
            pos.invalidate();
            dir.invalidate();
            return;
          }
          const size_t new_size = length_to_revert_from - revert_step;
          if (tck.size() == 2)
            dir = (tck[1] - tck[0]).normalise();
          else
            dir = (tck[new_size] - tck[new_size - 2]).normalise();
          tck.resize (length_to_revert_from - revert_step);
          pos = tck.back();
          if (S.is_act())
            act().sgm_depth = (act().sgm_depth > revert_step) ? act().sgm_depth - revert_step : 0;
        }


        ACT::ACT_Method_additions& act() const { return *act_method_additions; }

        Point<value_type> pos, dir;


      private:
        const SharedBase& S;
        copy_ptr<ACT::ACT_Method_additions> act_method_additions;


      protected:
        Math::RNG::Uniform<value_type> uniform_rng;
        std::vector<value_type> values;

        Point<value_type> random_direction ();
        Point<value_type> random_direction (value_type max_angle, value_type sin_max_angle);
        Point<value_type> random_direction (const Point<value_type>& d, value_type max_angle, value_type sin_max_angle);
        Point<value_type> rotate_direction (const Point<value_type>& reference, const Point<value_type>& direction);

    };






    Point<value_type> MethodBase::random_direction ()
    {
      Point<value_type> d;
      do {
        d[0] = 2.0 * uniform_rng() - 1.0;
        d[1] = 2.0 * uniform_rng() - 1.0;
        d[2] = 2.0 * uniform_rng() - 1.0;
      } while (d.norm2() > 1.0);
      d.normalise();
      return d;
    }


    Point<value_type> MethodBase::random_direction (value_type max_angle, value_type sin_max_angle)
    {
      value_type phi = 2.0 * Math::pi * uniform_rng();
      value_type theta;
      do {
        theta = max_angle * uniform_rng();
      } while (sin_max_angle * uniform_rng() > sin (theta));
      return (Point<value_type> (sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta)));
    }


    Point<value_type> MethodBase::random_direction (const Point<value_type>& d, value_type max_angle, value_type sin_max_angle)
    {
      return (rotate_direction (d, random_direction (max_angle, sin_max_angle)));
    }


    Point<value_type> MethodBase::rotate_direction (const Point<value_type>& reference, const Point<value_type>& direction)
    {
      value_type n = std::sqrt (Math::pow2(reference[0]) + Math::pow2(reference[1]));
      if (n == 0.0)
        return (reference[2] < 0.0 ? -direction : direction);

      Point<value_type> m (reference[0]/n, reference[1]/n, 0.0);
      Point<value_type> mp (reference[2]*m[0], reference[2]*m[1], -n);

      value_type alpha = direction[2];
      value_type beta = direction[0]*m[0] + direction[1]*m[1];

      return (Point<value_type> (
          direction[0] + alpha * reference[0] + beta * (mp[0] - m[0]),
          direction[1] + alpha * reference[1] + beta * (mp[1] - m[1]),
          direction[2] + alpha * (reference[2]-1.0) + beta * (mp[2] - m[2])
      ));
    }


      }
    }
  }
}

#endif



