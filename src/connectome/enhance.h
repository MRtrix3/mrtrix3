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




#ifndef __connectome_enhance_h__
#define __connectome_enhance_h__

#include <memory>
#include <stdint.h>
#include <vector>

#include "bitset.h"

#include "connectome/mat2vec.h"
#include "stats/enhance.h"
#include "stats/tfce.h"



namespace MR {
  namespace Connectome {
    namespace Enhance {



      typedef Math::Stats::value_type  value_type;
      typedef Math::Stats::vector_type vector_type;



      // This should be possible to use for any domain of inference
      class PassThrough : public Stats::EnhancerBase
      { MEMALIGN (PassThrough)
        public:
          PassThrough() { }
          ~PassThrough() { }

        private:
          value_type operator() (const vector_type&, vector_type&) const override;

      };



      class NBS : public Stats::TFCE::EnhancerBase
      { MEMALIGN (NBS)
        public:

          NBS () = delete;
          NBS (const node_t i) : threshold (0.0) { initialise (i); }
          NBS (const node_t i, const value_type t) : threshold (t) { initialise (i); }
          NBS (const NBS& that) = default;
          virtual ~NBS() { }

          void set_threshold (const value_type t) { threshold = t; }

          value_type operator() (const vector_type& in, vector_type& out) const override {
            return (*this) (in, threshold, out);
          }

          value_type operator() (const vector_type&, const value_type, vector_type&) const override;

        protected:
          std::shared_ptr< vector< vector<size_t> > > adjacency;
          value_type threshold;

        private:
          void initialise (const node_t);

      };



    }
  }
}


#endif

