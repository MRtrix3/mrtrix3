/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 22/02/12.

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
      class Jacobian : public Base<WarpType> {

        public:
          Jacobian (const WarpType& parent, bool wrt_scanner = true) :
            Base<WarpType> (parent),
            gradient1D (parent, 0, wrt_scanner),
            transform (parent),
            wrt_scanner (wrt_scanner) { }


          typedef typename WarpType::value_type value_type;


          Eigen::Matrix<value_type, 3, 3> value ()
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

//            std::cout << "jacobian" << std::endl;

//            std::cout << jacobian << std::endl;
//            std::cout << "image2scanner" << std::endl;

//            std::cout << transform.image2scanner.linear() << std::endl;
//        std::cout << "scanner2image" << std::endl;

//            std::cout << transform.scanner2image.linear() << std::endl;


            if (wrt_scanner)
              jacobian = jacobian * transform.scanner2image.linear().template cast<value_type>();
            return jacobian;
          }

          using Base<WarpType>::name;
          using Base<WarpType>::size;
          using Base<WarpType>::spacing;
          using Base<WarpType>::index;


        protected:
          Eigen::Matrix<value_type, 3, 3> jacobian;
          Gradient1D<WarpType> gradient1D;
          Transform transform;
          const bool wrt_scanner;
      };
  }
}


#endif

