/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#ifndef __dwi_tractography_tracking_method_h__
#define __dwi_tractography_tracking_method_h__


#include "dwi/tractography/tracking/shared.h"



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
              values             (shared.source_buffer.dim(3)) { } 

            MethodBase (const MethodBase& that) :
              pos                 (0.0, 0.0, 0.0),
              dir                 (0.0, 0.0, 1.0),
              S                   (that.S),
              rng                 (that.rng),
              values              (that.values.size()) { } 


            bool check_seed()
            {
              if (!pos.valid())
                return false;

              if ((S.properties.mask.size() && !S.properties.mask.contains (pos))
                  || (S.properties.exclude.contains (pos))) {
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
                return (!isnan (values[0]));
              }

            template <class InterpolatorType>
              inline bool get_data (InterpolatorType& source) {
                return (get_data (source, pos));
              }


            virtual void reverse_track() { }
            virtual bool init() = 0;
            virtual term_t next() = 0;


            virtual void truncate_track (std::vector< Point<value_type> >& tck, const size_t revert_step)
            {
              for (size_t i = revert_step; i && tck.size(); --i)
                tck.pop_back();
            }


            Point<value_type> pos, dir;


          private:
            const SharedBase& S;


          protected:
            Math::RNG rng;
            std::vector<value_type> values;

            Point<value_type> random_direction (value_type max_angle, value_type sin_max_angle);
            Point<value_type> random_direction (const Point<value_type>& d, value_type max_angle, value_type sin_max_angle);
            Point<value_type> rotate_direction (const Point<value_type>& reference, const Point<value_type>& direction);

        };






        Point<value_type> MethodBase::random_direction (value_type max_angle, value_type sin_max_angle)
        {
          value_type phi = 2.0 * M_PI * rng.uniform();
          value_type theta;
          do {
            theta = max_angle * rng.uniform();
          } while (sin_max_angle * rng.uniform() > sin (theta));
          return (Point<value_type> (sin(theta)*cos(phi), sin(theta)*sin(phi), cos(theta)));
        }


        Point<value_type> MethodBase::random_direction (const Point<value_type>& d, value_type max_angle, value_type sin_max_angle)
        {
          return (rotate_direction (d, random_direction (max_angle, sin_max_angle)));
        }


        Point<value_type> MethodBase::rotate_direction (const Point<value_type>& reference, const Point<value_type>& direction)
        {
          using namespace Math;

          value_type n = sqrt (pow2(reference[0]) + pow2(reference[1]));
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



