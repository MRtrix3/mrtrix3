/* Copyright (c) 2008-2021 the MRtrix3 contributors.
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

#ifndef __fixel_filter_cfe_h__
#define __fixel_filter_cfe_h__

#include "types.h"
#include "math/stats/typedefs.h"
#include "stats/enhance.h"

#include "fixel/matrix.h"
#include "fixel/filter/base.h"

namespace MR
{
  namespace Fixel
  {
    namespace Filter
    {



      constexpr float cfe_default_dh = 0.1;
      constexpr float cfe_default_e = 2.0;
      constexpr float cfe_default_h = 3.0;
      constexpr float cfe_default_c = 0.5;

      extern const App::OptionGroup cfe_options;

    
      class CFE : public Stats::EnhancerBase, public Fixel::Filter::Base
      { MEMALIGN (CFE)
        public:

          using value_type = Math::Stats::value_type;
          using direction_type = Eigen::Matrix<value_type, 3, 1>;

          CFE (const Fixel::Matrix::Reader& connectivity_matrix,
              const value_type dh = cfe_default_dh, 
              const value_type E = cfe_default_e, 
              const value_type H = cfe_default_h, 
              const value_type C = cfe_default_c,
              const bool norm = true);
          virtual ~CFE() { }

          // Override Fixel::Filter::Base functor
          void operator() (Image<float>& input, Image<float>& output) const override;

        protected:
          Fixel::Matrix::Reader matrix;
          const value_type dh, E, H, C;
          const bool normalise;

          mutable vector<value_type> h_pow_H;

          // Override Stats::EnhancerBase functor
          void operator() (in_column_type, out_column_type) const override;

          // TODO Templated implementation that is compatible with both fixel data files and matrix data
          template <class InputType, class OutputType>
          void run (InputType, OutputType) const;

      };



    }
  }
}

#endif
