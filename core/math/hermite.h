/* Copyright (c) 2008-2017 the MRtrix3 contributors
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


#ifndef __math_hermite_h__
#define __math_hermite_h__

#include <limits>

#include "types.h"

namespace MR
{
  namespace Math
  {

    template <typename T> class Hermite
    {
      public:
        typedef T value_type;

        Hermite (value_type tension = 0.0) : t (T (0.5) *tension) { }

        void set (value_type position) {
          value_type p2 = position*position;
          value_type p3 = position*p2;
          w[0] = (T (0.5)-t) * (T (2.0) *p2  - p3 - position);
          w[1] = T (1.0) + (T (1.5) +t) *p3 - (T (2.5) +t) *p2;
          w[2] = (T (2.0) +T (2.0) *t) *p2 + (T (0.5)-t) *position - (T (1.5) +t) *p3;
          w[3] = (T (0.5)-t) * (p3 - p2);
        }

        value_type coef (size_t i) const {
          return (w[i]);
        }

        template <class S>
        S value (const S* vals) const {
          return (value (vals[0], vals[1], vals[2], vals[3]));
        }

        template <class S>
        S value (const S& a, const S& b, const S& c, const S& d) const {
          return (w[0]*a + w[1]*b + w[2]*c + w[3]*d);
        }

      private:
        value_type w[4];
        value_type t;



    };

  }
}

#endif
