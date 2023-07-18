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
            writer.skip();
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
                temp.set_index (in.get_index());
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

