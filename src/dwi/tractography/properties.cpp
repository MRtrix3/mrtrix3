#include "dwi/tractography/properties.h"


namespace MR {
  namespace DWI {
    namespace Tractography {

      void Properties::load_ROIs ()
      {
        typedef std::multimap<std::string,std::string>::const_iterator iter;

        std::pair<iter,iter> range = roi.equal_range ("include");
        for (iter it = range.first; it != range.second; ++it) include.add (it->second);
        range = roi.equal_range ("exclude");
        for (iter it = range.first; it != range.second; ++it) exclude.add (it->second);
        range = roi.equal_range ("mask");
        for (iter it = range.first; it != range.second; ++it) mask.add (it->second);

        roi.clear();
      }

    }
  }
}

