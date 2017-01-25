/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */



#include "dwi/tractography/editing/receiver.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Editing {





        bool Receiver::operator() (const Streamline<>& in)
        {
          auto display_func = [&]() {
            return (printf ("%8" PRIu64 " read, %8" PRIu64 " written", total_count, count)
                    + (crop ? printf(", %8" PRIu64 " segments", segments) : ""));
          };

          if (number && (count == number))
            return false;

          ++total_count;

          if (in.empty()) {
            writer (in);
            progress.update (display_func);
            return true;
          }

          if (in[0].allFinite()) {

            if (skip) {
              --skip;
              progress.update (display_func);
              return true;
            }
            writer (in);
            ++segments;

          } else {

            // Explicitly handle case where the streamline has been cropped into multiple components
            // Worker class separates track segments using invalid points as delimiters
            Streamline<> temp;
            for (const auto& p : in) {
              if (p.allFinite()) {
                temp.push_back (p);
              } else if (temp.size()) {
                temp.index = in.index;
                temp.weight = in.weight;
                writer (temp);
                ++segments;
                temp.clear();
              }
            }
            assert (temp.empty());

          }

          ++count;
          progress.update (display_func);
          return (!(number && (count == number)));

        }



      }
    }
  }
}

