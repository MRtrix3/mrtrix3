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
