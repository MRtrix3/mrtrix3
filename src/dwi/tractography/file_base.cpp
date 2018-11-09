/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "dwi/tractography/file_base.h"
#include "file/path.h"

namespace MR {
  namespace DWI {
    namespace Tractography {


      void __ReaderBase__::open (const std::string& file, const std::string& type, Properties& properties)
      {
        properties.clear();
        dtype = DataType::Undefined;

        const std::string firstline ("mrtrix " + type);
        File::KeyValue kv (file, firstline.c_str());
        std::string data_file;

        while (kv.next()) {
          const std::string key = lowercase (kv.key());
          if (key == "roi") {
            try {
              vector<std::string> V (split (kv.value(), " \t", true, 2));
              properties.roi.insert (std::pair<std::string,std::string> (V[0], V[1]));
            }
            catch (...) {
              WARN ("invalid ROI specification in " + type  + " file \"" + file + "\" - ignored");
            }
          }
          else if (key == "comment") properties.comments.push_back (kv.value());
          else if (key == "file") data_file = kv.value();
          else if (key == "datatype") dtype = DataType::parse (kv.value());
          else properties[kv.key()] = kv.value();
        }

        if (dtype == DataType::Undefined)
          throw Exception ("no datatype specified for tracks file \"" + file + "\"");
        if (dtype != DataType::Float32LE && dtype != DataType::Float32BE &&
            dtype != DataType::Float64LE && dtype != DataType::Float64BE)
          throw Exception ("only supported datatype for tracks file are "
              "Float32LE, Float32BE, Float64LE & Float64BE (in " + type  + " file \"" + file + "\")");

        if (data_file.empty())
          throw Exception ("missing \"files\" specification for " + type  + " file \"" + file + "\"");

        std::istringstream files_stream (data_file);
        std::string fname;
        files_stream >> fname;
        int64_t offset = 0;
        if (files_stream.good()) {
          try { files_stream >> offset; }
          catch (...) {
            throw Exception ("invalid offset specified for file \""
                + fname + "\" in " + type  + " file \"" + file + "\"");
          }
        }

        if (fname != ".")
          fname = Path::join (Path::dirname (file), fname);
        else
          fname = file;

        in.open (fname.c_str(), std::ios::in | std::ios::binary);
        if (!in)
          throw Exception ("error opening " + type  + " data file \"" + fname + "\": " + strerror(errno));
        in.seekg (offset);
      }

    }
  }
}


