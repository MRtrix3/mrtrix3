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
          std::string key = lowercase (kv.key());
          if (key == "roi") {
            try {
              std::vector<std::string> V (split (kv.value(), " \t", true, 2));
              properties.roi.insert (std::pair<std::string,std::string> (V[0], V[1]));
            }
            catch (...) {
              WARN ("invalid ROI specification in " + type  + " file \"" + file + "\" - ignored");
            }
          }
          else if (key == "comment") properties.comments.push_back (kv.value());
          else if (key == "file") data_file = kv.value();
          else if (key == "datatype") dtype = DataType::parse (kv.value());
          else if (key == "timestamp") properties.timestamp = atof(kv.value().c_str());
          else properties[key] = kv.value();
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


