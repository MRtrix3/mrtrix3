/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#ifndef __stats_sse_h__
#define __stats_sse_h__

#include "types.h"
#include "math/stats/typedefs.h"
#include "stats/enhance.h"

#include "track/matrix.h"

namespace MR
{
  namespace Stats
  {

    using value_type = Math::Stats::value_type;
    // using direction_type = Eigen::Matrix<value_type, 3, 1>;



    class SSE : public Stats::EnhancerBase {
      public:
        SSE (const Track::Matrix::Reader& similarity_matrix, const Eigen::Matrix<float, Eigen::Dynamic, 1> weights,
             const value_type dh, const value_type E, const value_type H, const value_type M, const bool norm);
        virtual ~SSE() { }

      protected:
        const Track::Matrix::Reader matrix;
        const Eigen::Matrix<float, Eigen::Dynamic, 1> weights;
        const value_type dh, E, H, M;
        const bool normalise;

        mutable std::vector<value_type> h_pow_H;

        void operator() (in_column_type, out_column_type) const override;
    };



  }
}

#endif
