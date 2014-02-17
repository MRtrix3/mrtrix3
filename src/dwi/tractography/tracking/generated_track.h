#ifndef __dwi_tractography_tracking_generated_track_h__
#define __dwi_tractography_tracking_generated_track_h__


#include <vector>

#include "point.h"

#include "dwi/tractography/tracking/types.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Tracking
      {



        class GeneratedTrack : public std::vector< Point<Tracking::value_type> >
        {

          typedef std::vector< Point<Tracking::value_type> > BaseType;

          public:
          GeneratedTrack() : seed_index (0) { }
          void clear() { BaseType::clear(); seed_index = 0; }
          size_t get_seed_index() const { return seed_index; }
          void reverse() { std::reverse (begin(), end()); seed_index = size()-1; }
          void set_seed_index (const size_t i) { seed_index = i; }

          private:
          size_t seed_index;

        };


      }
    }
  }
}

#endif

