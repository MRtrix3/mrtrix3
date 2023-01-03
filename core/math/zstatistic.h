/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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


#ifndef __math_zstatistic_h__
#define __math_zstatistic_h__

#include <map>
#include <mutex>

#include "math/stats/typedefs.h"

namespace MR
{
  namespace Math
  {



    default_type t2z (const default_type stat, const default_type dof);
    default_type F2z (const default_type stat, const size_t rank, const default_type dof);



    class Zstatistic
    { MEMALIGN(Zstatistic)
      public:
        Zstatistic() { }

        // Convert a t-statistic to a z-statistic
        default_type t2z (const default_type t,
                          const size_t dof);

        // Convert an F-statistic to a z-statistic
        default_type F2z (const default_type F,
                          const size_t rank,
                          const size_t dof);

        // Convert an Aspin-Welch v to a z-statistic
        default_type v2z (const default_type v,
                          const default_type dof);

        // Convert a G-statistic to a z-statistic
        default_type G2z (const default_type G,
                          const size_t rank,
                          const default_type dof);

      protected:

        class LookupBase
        { MEMALIGN(LookupBase)
          public:
            virtual ~LookupBase() { }
            using array_type = Eigen::Array<default_type, Eigen::Dynamic, 1>;
            virtual default_type operator() (const default_type) const = 0;
          protected:
            // Function that will determine an interpolated value using
            //   the lookup table if the value lies within the pre-calculated
            //   range, or perform an explicit calculation if that is not
            //   the case
            default_type interp (const default_type stat,
                                 const default_type offset,
                                 const default_type scale,
                                 const array_type& data,
                                 std::function<default_type(default_type)> func) const;
        };

        class Lookup_t2z : public LookupBase
        { MEMALIGN(Lookup_t2z)
          public:
            Lookup_t2z (const size_t dof);
            default_type operator() (const default_type) const override;
          private:
            const size_t dof;
            // Transform an input statistic value to an index location in the lookup table
            const default_type offset, scale;
            array_type data;
        };

        class Lookup_F2z : public LookupBase
        { MEMALIGN(Lookup_F2z)
          public:
            Lookup_F2z (const size_t rank, const size_t dof);
            default_type operator() (const default_type) const override;
          private:
            const size_t rank, dof;
            const default_type offset_upper, scale_upper;
            array_type data_upper;
            const default_type offset_lower, scale_lower;
            array_type data_lower;
        };

        std::map<size_t, Lookup_t2z> t2z_data;
        std::map<std::pair<size_t, size_t>, Lookup_F2z> F2z_data;
        std::mutex mutex;

    };




  }
}


#endif
