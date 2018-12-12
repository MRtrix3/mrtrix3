/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __fixel_filter_smooth_h__
#define __fixel_filter_smooth_h__

#include "fixel/matrix.h"
#include "fixel/filter/base.h"

namespace MR
{
  namespace Fixel
  {
    namespace Filter
    {



      /** \addtogroup Filters
      @{ */

      /*! Smooth fixel data using a fixel-fixel connectivity matrix.
       *
       * Typical usage:
       * \code
       * auto input = Image<float>::open (argument[0]);
       * Fixel::Matrix::norm_matrix_type matrix;
       * Fixel::Matrix::load (argument[1], matrix);
       * Fixel::Filter::Smooth smooth_filter (matrix);
       * auto output = Image::create<float> (argument[2], input);
       * smooth_filter (input, output);
       *
       * \endcode
       */

      class Smooth : public Base
      { MEMALIGN (Smooth)

        public:
          Smooth (const Fixel::Matrix::norm_matrix_type& matrix) :
              matrix (matrix) { }

          void operator() (Image<float>& input, Image<float>& output) const;

        protected:
          const Fixel::Matrix::norm_matrix_type& matrix;
      };
    //! @}



    }
  }
}


#endif
