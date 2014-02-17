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
