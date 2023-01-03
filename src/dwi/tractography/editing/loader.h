/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __dwi_tractography_editing_loader_h__
#define __dwi_tractography_editing_loader_h__


#include <string>

#include "memory.h"
#include "types.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Editing {




        class Loader
        { MEMALIGN(Loader)

          public:
            Loader (const vector<std::string>& files) :
              file_list (files),
              dummy_properties (),
              reader (new Reader<> (file_list[0], dummy_properties)),
              file_index (0) { }

            bool operator() (Streamline<>&);


          private:
            const vector<std::string>& file_list;
            Properties dummy_properties;
            std::unique_ptr<Reader<> > reader;
            size_t file_index;

        };



        bool Loader::operator() (Streamline<>& out)
        {
          out.clear();

          if ((*reader) (out))
            return true;

          while (++file_index != file_list.size()) {
            dummy_properties.clear();
            reader.reset (new Reader<> (file_list[file_index], dummy_properties));
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
