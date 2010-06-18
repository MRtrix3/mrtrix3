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
};


EXECUTE {
  Image::Header source = Image::Header::open ("CSD10.mif");
  Properties props;
  props.seed.add (ROI (std::string ("mask.mif")));

  iFOD1::Shared shared (source, props);
  TimingTest test (shared);
}

