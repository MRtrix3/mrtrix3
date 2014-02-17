#ifndef __dwi_tractography_algorithms_seedtest_h__
#define __dwi_tractography_algorithms_seedtest_h__

#include "point.h"
#include "dwi/tractography/tracking/method.h"
#include "dwi/tractography/tracking/shared.h"
#include "dwi/tractography/tracking/types.h"



namespace MR {
namespace DWI {
namespace Tractography {
namespace Algorithms {

using namespace MR::DWI::Tractography::Tracking;

class Seedtest : public MethodBase {

  public:

  class Shared : public SharedBase {
    public:
    Shared (const std::string& diff_path, DWI::Tractography::Properties& property_set) :
        SharedBase (diff_path, property_set)
    {
      set_step_size (1.0);
      min_num_points = 1;
      max_num_points = 2;
      unidirectional = true;
      properties["method"] = "Seedtest";
    }
  };

  Seedtest (const Shared& shared) :
    MethodBase (shared),
    S (shared) { }


  bool init () { return true; }
  term_t next () { return EXIT_IMAGE; }


  protected:
    const Shared& S;

};

}
}
}
}

#endif


