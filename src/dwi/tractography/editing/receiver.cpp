/*
   Copyright 2011 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 2014.

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


#include "dwi/tractography/editing/receiver.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Editing {





        bool Receiver::operator() (const Streamline& in)
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

          if (std::isfinite (in[0][0])) {

            if (skip) {
              --skip;
              progress.update (display_func);
              return true;
            }
            output (in);

          } else {

            // Explicitly handle case where the streamline has been cropped into multiple components
            // Worker class separates track segments using invalid points as delimiters
            Streamline temp;
            temp.index = in.index;
            temp.weight = in.weight;
            for (const auto& p : in) {
              if (std::isfinite (p[0])) {
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



        void Receiver::output (const Streamline& in)
        {
          if (ends_only) {
            Streamline temp;
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

