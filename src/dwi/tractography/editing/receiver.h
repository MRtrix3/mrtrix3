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

#ifndef __dwi_tractography_editing_receiver_h__
#define __dwi_tractography_editing_receiver_h__


#include <string>
#include <cinttypes>

#include "progressbar.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Editing {




        class Receiver
        {

          public:

            Receiver (const std::string& path, const Tractography::Properties& properties, const size_t c, const size_t n, const size_t s) :
              writer (path, properties),
              in_count (c),
              number (n),
              skip (s),
              // Need to use local counts instead of writer class members due to track cropping
              count (0),
              total_count (0),
              progress ("       0 read,        0 written") { }

            ~Receiver()
            {
              progress.set_text (printf ("%8" PRIu64 " read, %8" PRIu64 " written", total_count, count));
              if (number && (count != number))
                WARN ("User requested " + str(number) + " streamlines, but only " + str(count) + " were written to file");
            }


            bool operator() (const Tractography::Streamline<>&);


          private:

            Tractography::Writer<> writer;
            const uint64_t in_count, number;
            uint64_t skip;
            uint64_t count, total_count;
            ProgressBar progress;
        };



      }
    }
  }
}

#endif
