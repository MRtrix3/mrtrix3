/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "file/ofstream.h"

#include "file/utils.h"


namespace MR
{
  namespace File
  {


  void OFStream::open (const std::string& path, const std::ios_base::openmode mode)
  {
    if (!(mode & std::ios_base::app) && !(mode & std::ios_base::ate) && !(mode & std::ios_base::in)) {
      if (!File::is_tempfile (path))
        File::create (path);
    }

    std::ofstream::open (path.c_str(), mode);
    if (std::ofstream::operator!())
      throw Exception ("error opening output file \"" + path + "\": " + std::strerror (errno));
  }


  }
}



