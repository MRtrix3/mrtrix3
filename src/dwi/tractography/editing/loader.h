#ifndef __dwi_tractography_editing_loader_h__
#define __dwi_tractography_editing_loader_h__


#include <string>
#include <vector>

#include "ptr.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Editing {




        class Loader
        {

          public:
            Loader (const std::vector<std::string>& files) :
              file_list (files),
              dummy_properties (),
              reader (new Tractography::Reader<> (file_list[0], dummy_properties)),
              file_index (0) { }

            bool operator() (Tractography::Streamline<>&);


          private:
            const std::vector<std::string>& file_list;
            Tractography::Properties dummy_properties;
            Ptr< Tractography::Reader<> > reader;
            size_t file_index;

        };



        bool Loader::operator() (Tractography::Streamline<>& out)
        {

          out.clear();

          if ((*reader) (out))
            return true;

          while (++file_index != file_list.size()) {
            dummy_properties.clear();
            reader = new Tractography::Reader<> (file_list[file_index], dummy_properties);
            if ((*reader) (out))
              return true;
          }

          return false;

        }




      }
    }
  }
}

#endif
