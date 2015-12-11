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

            Receiver (const std::string& path, const Properties& properties, const size_t n, const size_t s, const bool e) :
              writer (path, properties),
              number (n),
              skip (s),
              ends_only (e),
              // Need to use local counts instead of writer class members due to track cropping
              count (0),
              total_count (0),
              progress ("       0 read,        0 written") { }

            ~Receiver()
            {
              // Use set_text() rather than update() here to force update of the text before progress goes out of scope
              progress.set_text (printf ("%8" PRIu64 " read, %8" PRIu64 " written", total_count, count));
              if (number && (count != number))
                WARN ("User requested " + str(number) + " streamlines, but only " + str(count) + " were written to file");
            }


            bool operator() (const Streamline<>&);


          private:

            Writer<> writer;
            const uint64_t number;
            uint64_t skip;
            const bool ends_only;
            uint64_t count, total_count;
            ProgressBar progress;

            void output (const Streamline<>&);

        };



      }
    }
  }
}

#endif
