#include "app.h"
#include "point.h"
#include "dwi/tractography/roi.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/shared.h"
#include "dwi/tractography/method.h"
#include "dwi/tractography/iFOD1.h"


using namespace std; 
using namespace MR; 
using namespace MR::DWI; 
using namespace MR::DWI::Tractography; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "perform timing tests for streamlines tracking.",
  NULL
};

ARGUMENTS = { Argument::End }; 
OPTIONS = { Option::End }; 


class TimingTest : public iFOD1
{
  public:
    TimingTest (const iFOD1::Shared& shared) :  iFOD1 (shared) {
      size_t count = 0;
      float sum = 0.0;
      const ROI& roi (S.properties.seed[0]);

      Timer stop_watch;



      /*
      {
        stop_watch.start();

        for (size_t n = 0; n < 1000000; ++n) {
          Point p = roi.sample (rng);
          count += roi.contains (p);
          Point dir (rng.normal(), rng.normal(), rng.normal());
          dir.normalise();
          get_data (p);
          sum += S.precomputer.value (values, dir);
        };

        std::cout << stop_watch.elapsed() << " s\n";
        VAR (count);
        VAR (sum);
      }
      */

      /*
      {
        sum = 0.0;
        Point p (0.0, 0.0, 0.0);
        get_data (p);

        stop_watch.start();

        for (size_t n = 0; n < 1000000; ++n) {
          Point dir (rng.normal(), rng.normal(), rng.normal());
          dir.normalise();
          sum += S.precomputer.value (values, dir);
        };

        std::cout << stop_watch.elapsed() << " s\n";
        VAR (sum);
      }
      */


      {
        Point p, mean_p (0.0, 0.0, 0.0);
        stop_watch.start();
        for (size_t n = 0; n < 100000; ++n) {
          size_t num_attempts = 0;
          do { 
            pos = S.properties.seed.sample (rng);
            num_attempts++;
            if (num_attempts++ > 10000) throw Exception ("failed to find suitable seed point after 10,000 attempts - aborting");
          } while (!init ());
          mean_p += pos;
        }
        std::cout << stop_watch.elapsed() << " s\n";
        VAR (mean_p*(1.0/100000.0));
      }

    }
};


EXECUTE {
  Image::Header source = Image::Header::open ("CSD10.mif");
  Properties props;
  props.seed.add (ROI (std::string ("mask.mif")));

  iFOD1::Shared shared (source, props);
  shared.init_threshold = 0.1;
  TimingTest test (shared);
}

