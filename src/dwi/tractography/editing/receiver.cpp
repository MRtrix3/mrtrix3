/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */


#include "dwi/tractography/editing/receiver.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Editing {





        bool Receiver::operator() (const Streamline<>& in)
        {
          auto display_func = [&](){ return printf ("%8" PRIu64 " read, %8" PRIu64 " written", total_count, count); };

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
            output (in);

          } else {

            // Explicitly handle case where the streamline has been cropped into multiple components
            // Worker class separates track segments using invalid points as delimiters
            Streamline<> temp;
            temp.index = in.index;
            temp.weight = in.weight;
            for (const auto& p : in) {
              if (p.allFinite()) {
                temp.push_back (p);
              } else if (temp.size()) {
                output (temp);
                temp.clear();
              }
            }

          }

          ++count;
          progress.update (display_func);
          return (!(number && (count == number)));

        }



        void Receiver::output (const Streamline<>& in)
        {
          if (ends_only) {
            Streamline<> temp;
            temp.push_back (in.front());
            temp.push_back (in.back());
            writer (temp);
          } else {
            writer (in);
          }
        }



      }
    }
  }
}

