/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#include "file/path.h"
#include "dwi/tractography/file.h"


namespace MR {
  namespace DWI {
    namespace Tractography {

      void __ReaderBase__::open (const std::string& file, Properties& properties)
      {
        properties.clear();
        dtype = DataType::Undefined;

        File::KeyValue kv (file, "mrtrix tracks");
        std::string data_file;

        while (kv.next()) {
          std::string key = lowercase (kv.key());
          if (key == "roi") {
            try {
              std::vector<std::string> V (split (kv.value(), " \t", true, 2));
              properties.roi.insert (std::pair<std::string,std::string> (V[0], V[1]));
            }
            catch (...) {
              warning ("invalid ROI specification in tracks file \"" + file + "\" - ignored");
            }
          }
          else if (key == "comment") properties.comments.push_back (kv.value());
          else if (key == "file") data_file = kv.value();
          else if (key == "datatype") dtype = DataType::parse (kv.value());
          else properties[key] = kv.value();
        }

        if (dtype == DataType::Undefined) 
          throw Exception ("no datatype specified for tracks file \"" + file + "\"");
        if (dtype != DataType::Float32LE && dtype != DataType::Float32BE && 
            dtype != DataType::Float64LE && dtype != DataType::Float64BE)
          throw Exception ("only supported datatype for tracks file are "
              "Float32LE, Float32BE, Float64LE & Float64BE (in tracks file \"" + file + "\")");

        if (data_file.empty()) 
          throw Exception ("missing \"files\" specification for tracks file \"" + file + "\"");

        std::istringstream files_stream (data_file);
        std::string fname;
        files_stream >> fname;
        int64_t offset = 0;
        if (files_stream.good()) {
          try { files_stream >> offset; }
          catch (...) { 
            throw Exception ("invalid offset specified for file \""
                + fname + "\" in tracks file \"" + file + "\"");
          }
        }

        if (fname != ".") 
          fname = Path::join (Path::dirname (file), fname);
        else 
          fname = file;

        in.open (fname.c_str(), std::ios::in | std::ios::binary);
        if (!in) 
          throw Exception ("error opening tracks data file \"" + fname + "\": " + strerror(errno));
        in.seekg (offset);
      }



    }
  }
}


