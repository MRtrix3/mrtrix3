/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#ifndef __dwi_tractography_editing_receiver_h__
#define __dwi_tractography_editing_receiver_h__


#include <string>

#include "timer.h"

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
              total_count (0) { }

            ~Receiver()
            {
              print();
              if (App::log_level > 0)
                fprintf (stderr, "\n");
              if (number && (count != number))
                WARN ("User requested " + str(number) + " streamlines, but only " + str(count) + " were written to file");
            }


            bool operator() (const Tractography::Streamline<>&);


          private:

            Tractography::Writer<> writer;
            const size_t in_count, number;
            size_t skip;
            size_t count, total_count;
            IntervalTimer timer;

            void update_cmdline();
            void print();

        };



      }
    }
  }
}

#endif
