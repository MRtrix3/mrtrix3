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

#ifndef __stats_tfce_h__
#define __stats_tfce_h__

#include "thread_queue.h"
#include "filter/connected_components.h"
#include "math/stats/typedefs.h"

#include "stats/enhance.h"

namespace MR
{
  namespace Stats
  {
    namespace TFCE
    {



      const App::OptionGroup Options (const default_type, const default_type, const default_type);



      using value_type = Math::Stats::value_type;
      using matrix_type = Math::Stats::matrix_type;



      class EnhancerBase : public Stats::EnhancerBase
      {
        public:
          virtual ~EnhancerBase() { }
        protected:
          // Alternative functor that also takes the threshold value;
          //   makes TFCE integration cleaner
          virtual void operator() (in_column_type /*input_statistics*/, const value_type /*threshold*/, out_column_type /*enhanced_statistics*/) const = 0;
          // While we don't use this function, it reassures the compiler that we are not accidentally
          //   hiding the virutal function of the base class using the function above
          using Stats::EnhancerBase::operator();
          friend class Wrapper;
      };




      class Wrapper : public Stats::EnhancerBase
      {
        public:
          Wrapper (const std::shared_ptr<TFCE::EnhancerBase> base) : enhancer (base), dH (NaN), E (NaN), H (NaN) { }
          Wrapper (const std::shared_ptr<TFCE::EnhancerBase> base, const default_type dh, const default_type e, const default_type h) : enhancer (base), dH (dh), E (e), H (h) { }
          Wrapper (const Wrapper& that) = default;
          virtual ~Wrapper() { }

          void set_tfce_parameters (const value_type d_height, const value_type extent, const value_type height)
          {
            dH = d_height;
            E = extent;
            H = height;
          }

        private:
          std::shared_ptr<Stats::TFCE::EnhancerBase> enhancer;
          value_type dH, E, H;

          void operator() (in_column_type, out_column_type) const override;
      };



    }
  }
}

#endif
