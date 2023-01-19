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

#ifndef __stats_enhance_h__
#define __stats_enhance_h__

#include "math/stats/typedefs.h"

namespace MR
{
  namespace Stats
  {



    // This class defines the standardised interface by which statistical enhancement
    //   is performed.
    class EnhancerBase
    { NOMEMALIGN
      public:
        virtual ~EnhancerBase() { }

        // Perform statistical enhancement once for each column in the matrix
        //   (correspond to different contrasts)
        void operator() (const Math::Stats::matrix_type& input_statistics,
                         Math::Stats::matrix_type& enhanced_statistics) const
        {
          for (ssize_t col = 0; col != input_statistics.cols(); ++col)
            (*this) (input_statistics.col (col), enhanced_statistics.col (col));
        }

      protected:
        typedef Math::Stats::matrix_type::ConstColXpr in_column_type;
        typedef Math::Stats::matrix_type::ColXpr out_column_type;
        // Derived classes should override this function
        virtual void operator() (in_column_type, out_column_type) const = 0;

    };



  }
}

#endif
