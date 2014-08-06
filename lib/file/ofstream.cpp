/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 04/08/14.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "file/ofstream.h"

#include "file/utils.h"


namespace MR
{
  namespace File
  {


  void OFStream::open (const std::string& path, const std::ios_base::openmode mode)
  {
    if (!(mode & std::ios_base::app) && !(mode & std::ios_base::ate) && !(mode & std::ios_base::in))
      File::output_file_check (path);
    std::ofstream::open (path.c_str(), mode);
    if (std::ofstream::operator!())
      throw Exception ("error opening output file \"" + path + "\": " + std::strerror (errno));
  }


  }
}



