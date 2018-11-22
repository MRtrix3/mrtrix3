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
 * See the Mozila Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __stats_tfce_h__
#define __stats_tfce_h__

#include "thread_queue.h"
#include "filter/connected_components.h"
#include "math/stats/permutation.h"
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
      using vector_type = Math::Stats::vector_type;



      class EnhancerBase : public Stats::EnhancerBase
      { MEMALIGN (EnhancerBase)
        public:
          // Alternative functor that also takes the threshold value;
          //   makes TFCE integration cleaner
          virtual value_type operator() (const vector_type& /*input_statistics*/, const value_type /*threshold*/, vector_type& /*enhanced_statistics*/) const = 0;

      };




      class Wrapper : public Stats::EnhancerBase
      { MEMALIGN (Wrapper)
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

          value_type operator() (const vector_type&, vector_type&) const override;

        private:
          std::shared_ptr<Stats::TFCE::EnhancerBase> enhancer;
          value_type dH, E, H;
      };



    }
  }
}

#endif
