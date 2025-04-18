/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#ifndef __image_adapter_jacobian_h__
#define __image_adapter_jacobian_h__

#include "adapter/base.h"
#include "adapter/gradient1D.h"
#include "transform.h"

namespace MR
{
  namespace Adapter
  {

    template <class WarpType>
      class Jacobian : 
        public Base<Jacobian<WarpType>,WarpType> 
    { MEMALIGN (Jacobian<WarpType>)
        public:

          using base_type = Base<Jacobian<WarpType>, WarpType>;
          using value_type = Eigen::Matrix<typename WarpType::value_type, 3, 3>;

          using base_type::name;
          using base_type::size;
          using base_type::spacing;
          using base_type::index;

          Jacobian (const WarpType& parent, bool wrt_scanner = true) :
            base_type (parent),
            gradient1D (parent, 0, wrt_scanner),
            transform (parent),
            wrt_scanner (wrt_scanner) { }

          value_type value ()
          {
            for (size_t dim = 0; dim < 3; ++dim)
              gradient1D.index(dim) = index(dim);
            for (size_t i = 0; i < 3; ++i) {
              gradient1D.index(3) = i;
              for (size_t j = 0; j < 3; ++j) {
                gradient1D.set_axis(j);
                jacobian (i, j) = gradient1D.value();
              }
            }

            if (wrt_scanner)
              jacobian = jacobian * transform.scanner2image.linear().template cast<typename WarpType::value_type>();
            return jacobian;
          }


        protected:
          value_type jacobian;
          Gradient1D<WarpType> gradient1D;
          Transform transform;
          const bool wrt_scanner;
      };
  }
}


#endif

