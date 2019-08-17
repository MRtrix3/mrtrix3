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


#include "fixel/filter/smooth.h"

#include "image_helpers.h"
#include "fixel/helpers.h"

namespace MR
{
  namespace Fixel
  {
    namespace Filter
    {



      void Smooth::operator() (Image<float>& input, Image<float>& output) const
      {
        Fixel::check_data_file (input);
        Fixel::check_data_file (output);

        check_dimensions (input, output);

        if (size_t (input.size(0)) != matrix.size())
          throw Exception ("Size of fixel data file \"" + input.name() + "\" (" + str(input.size(0)) +
                           ") does not match fixel connectivity matrix (" + str(matrix.size()) + ")");

        // Technically need to loop over axis 1; could be more than 1 parameter in the file
        for (auto l = Loop(1) (input, output); l; ++l) {
          for (size_t fixel = 0; fixel != matrix.size(); ++fixel) {
            input.index(0) = output.index(0) = fixel;
            if (std::isfinite (static_cast<float>(input.value()))) {
              default_type value = 0.0, sum_weights = 0.0;
              for (const auto& it : matrix[fixel]) {
                input.index(0) = it.index();
                if (std::isfinite (static_cast<float>(input.value()))) {
                  value += input.value() * it.value();
                  sum_weights += it.value();
                }
              }
              if (sum_weights)
                output.value() = value / sum_weights;
              else
                output.value() = NaN;
            } else {
              output.value() = NaN;
            }
          }
        }
        input.index(1) = output.index(1) = 0;
      }



    }
  }
}

