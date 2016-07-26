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



#ifndef __connectome_enhance_h__
#define __connectome_enhance_h__

#include <memory>
#include <stdint.h>
#include <vector>

#include "bitset.h"

#include "connectome/mat2vec.h"



namespace MR {
  namespace Connectome {
    namespace Enhance {



      // TODO This should instead be handled using the new base enhancer class
      // These changes must not have been pushed...
      class Base
      {
        public:
          Base() { }
          Base (const Base& that) = default;
          virtual ~Base() { }

          value_type operator() (const value_type max, const vector_type& in, vector_type& out) const { return enhance (max, in, out); }
          virtual value_type operator() (const vector_type& in, vector_type& out, const value_type t) const { return enhance_at_t (in, out, t); }

        protected:
          virtual value_type enhance (const value_type, const vector_type&, vector_type&) const { assert (0); return NaN; }
          virtual value_type enhance_at_t (const vector_type&, vector_type&, const value_type) const { assert (0); return NaN; }

      };



      // This should be possible to use for either edge or node inference
      class PassThrough : public Base
      {
        public:
          PassThrough() { }
          ~PassThrough() { }

        private:
          value_type enhance (const value_type, const vector_type&, vector_type&) const override;

      };



      class NBS : public Base
      {
        public:

          NBS () = delete;
          NBS (const node_t i) : threshold (0.0) { initialise (i); }
          NBS (const node_t i, const value_type t) : threshold (t) { initialise (i); }
          NBS (const NBS& that) = default;
          virtual ~NBS() { }

          void set_threshold (const value_type t) { threshold = t; }

        protected:
          std::shared_ptr< std::vector< std::vector<size_t> > > adjacency;
          value_type threshold;

        private:
          void initialise (const node_t);

          value_type enhance (const value_type max_t, const vector_type& in, vector_type& out) const override {
            return enhance_at_t (in, out, threshold);
          }

          value_type enhance_at_t (const vector_type&, vector_type&, const value_type) const override;

      };



      class TFCEWrapper : public Base
      {
        public:
          TFCEWrapper (const std::shared_ptr<Base> base) : enhancer (base), E (NaN), H (NaN), dh (NaN) { }
          TFCEWrapper (const TFCEWrapper& that) = default;
          ~TFCEWrapper() { }

          // TODO Homogenise TFCE
          void set_tfce_parameters (const value_type extent, const value_type height, const value_type d_height)
          {
            E = extent;
            H = height;
            dh = d_height;
          }

        private:
          std::shared_ptr<Base> enhancer;
          value_type E, H, dh;

          value_type enhance (const value_type, const vector_type&, vector_type&) const override;

      };





    }
  }
}


#endif

