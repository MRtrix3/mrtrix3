/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt 19/11/12

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

#ifndef __registration_transform_warp_composer_h__
#define __registration_transform_warp_composer_h__

#include "transform.h"
#include "interp/cubic.h"
#include "algo/iterator.h"


namespace MR
{
  namespace Registration
  {
    namespace Transform
    {

      /** \addtogroup Registration
      @{ */

      /*! a thread kernel to compose two deformation fields
       *
       * Typical usage:
       * \code
       * WarpComposer <Image<float> > warpcomposer (warp1, warp2, composed_warp)
       * ThreadedLoop (warp1, 0, 3).run (warpcomposer, warp1, composed_warp);
       * \endcode
       */
      template <class WarpType>
      class WarpComposer
      {
        public:
          typedef typename WarpType::value_type value_type;

          WarpComposer (WarpType& first_warp,
                        WarpType& second_warp,
                        WarpType& output_warp) :
            first_warp (first_warp),
            output_warp (output_warp),
            second_warp_interp (second_warp) { }


          void operator () (const Iterator& pos) {
            second_warp_interp.scanner (first_warp.row(3));
            output_warp.row(3) = second_warp_interp.row(3);
          }

        protected:
          WarpType first_warp;
          WarpType output_warp;
          Interp::Cubic<WarpType> second_warp_interp;

      };
      //! @}
    }
  }
}


#endif
