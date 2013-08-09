/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 31/07/13.

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


#include <fstream>
#include <vector>

#include "file/key_value.h"
#include "file/path.h"
#include "image/header.h"
#include "image/stride.h"



namespace MR
{
  namespace Image
  {
    namespace Format
    {


      // Read generic image header information - common between conventional and sparse formats
      void read_mrtrix_header (Header&, File::KeyValue&);

      // Get the path to a file - use same function for image data and sparse data
      // Note that the 'file' and 'sparse_file' fields are read in as entries in the map<string, string>
      //   by read_mrtrix_header(), and are therefore erased by this function so that they do not propagate
      //   into new images created using this header
      void get_mrtrix_file_path (Header&, const std::string&, std::string&, size_t&);

      // Write generic image header information to file
      void write_mrtrix_header (const Header&, std::ofstream&);



    }
  }
}
