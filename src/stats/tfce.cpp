/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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

#include "stats/tfce.h"

namespace MR
{
  namespace Stats
  {
    namespace TFCE
    {


      using namespace App;
      const OptionGroup Options (const default_type default_dh, const default_type default_e, const default_type default_h)
      {
        OptionGroup result = OptionGroup ("Options for controlling TFCE behaviour")

        + Option ("tfce_dh", "the height increment used in the tfce integration (default: " + str(default_dh, 2) + ")")
        + Argument ("value").type_float (1e-6)

        + Option ("tfce_e", "tfce extent exponent (default: " + str(default_e, 2) + ")")
        + Argument ("value").type_float (0.0)

        + Option ("tfce_h", "tfce height exponent (default: " + str(default_h, 2) + ")")
        + Argument ("value").type_float (0.0);

        return result;
      }



      value_type Wrapper::operator() (const vector_type& in, vector_type& out) const
      {
        out = vector_type::Zero (in.size());
        const value_type max_input_value = in.maxCoeff();
        for (value_type h = dH; (h-dH) < max_input_value; h += dH) {
          vector_type temp;
          const value_type max = (*enhancer) (in, h, temp);
          if (max) {
            const value_type h_multiplier = std::pow (h, H);
            for (size_t index = 0; index != size_t(in.size()); ++index)
              out[index] += (std::pow (temp[index], E) * h_multiplier);
          }
        }
        return out.maxCoeff();
      }



    }
  }
}
