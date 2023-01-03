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

#ifndef __file_ofstream_h__
#define __file_ofstream_h__


#include <fstream>
#include <string>

#include "types.h"


namespace MR
{
  namespace File
  {


    //! open output files for writing, checking for pre-existing file if necessary
    /*! This class is intended to be used as a substitute for std::ofstream.
     * It ensures that if the user has not given the command permission to overwrite
     * output files, the presence of an existing file is first checked. It also removes the
     * necessity to explicitly convert a path expressed as a std::string to a
     * c-style string. */
    class OFStream : public std::ofstream { NOMEMALIGN
      public:
        OFStream() { }
        OFStream (const std::string& path, const std::ios_base::openmode mode = std::ios_base::out | std::ios_base::binary) {
          open (path, mode);
        }

        void open (const std::string& path, const std::ios_base::openmode mode = std::ios_base::out | std::ios_base::binary);

    };


  }
}

#endif


