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




      typedef Math::Stats::value_type value_type;
      typedef Math::Stats::vector_type vector_type;



      class EnhancerBase : public Stats::EnhancerBase
      {
        public:
          // Alternative functor that also takes the threshold value;
          //   makes TFCE integration cleaner
          virtual value_type operator() (const vector_type& /*input_statistics*/, const value_type /*threshold*/, vector_type& /*enhanced_statistics*/) const = 0;

      };




      class Wrapper : public Stats::EnhancerBase
      {
        public:
          Wrapper (const std::shared_ptr<TFCE::EnhancerBase> base) : enhancer (base), dH (NaN), E (NaN), H (NaN) { }
          Wrapper (const std::shared_ptr<TFCE::EnhancerBase> base, const default_type dh, const default_type e, const default_type h) : enhancer (base), dH (dh), E (e), H (h) { }
          Wrapper (const Wrapper& that) = default;
          ~Wrapper() { }

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
