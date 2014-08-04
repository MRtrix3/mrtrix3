/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 23/07/09.

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

#ifndef __file_ofstream_h__
#define __file_ofstream_h__


#include <fstream>
#include <string>

#include "file/utils.h"


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
    class OFStream : public std::ofstream
    {
      public:
        OFStream() { }
        OFStream (const std::string& path, const std::ios_base::openmode mode = std::ios_base::out) {
          open (path, mode);
        }
        OFStream (const char* path, const std::ios_base::openmode mode = std::ios_base::out) {
          const std::string temp (path);
          open (path, mode);
        };

        void open (const std::string& path, const std::ios_base::openmode mode = std::ios_base::out)
        {
          if (!(mode & std::ios_base::app) && !(mode & std::ios_base::ate) && !(mode & std::ios_base::in))
            output_file_check (path);
          std::ofstream::open (path.c_str(), mode);
          if (!*this)
            throw Exception ("error opening output file \"" + path + "\": " + strerror (errno));
        }

        void open (const char* path, const std::ios_base::openmode mode = std::ios_base::out) {
          const std::string temp (path);
          open (path, mode);
        }

    };


  }
}

#endif


