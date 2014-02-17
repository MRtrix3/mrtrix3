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


#include "image/format/mrtrix_utils.h"


namespace MR
{
  namespace Image
  {
    namespace Format
    {




      bool next_keyvalue (File::KeyValue& kv, std::string& key, std::string& value)
      {
        key.clear(); value.clear();
        if (!kv.next())
          return false;
        key = kv.key();
        value = kv.value();
        return true;
      }


      bool next_keyvalue (File::GZ& gz, std::string& key, std::string& value)
      {
        key.clear(); value.clear();
        std::string line = gz.getline();
        line = strip (line.substr (0, line.find_first_of ('#')));
        if (line.empty() || line == "END")
          return false;

        size_t colon = line.find_first_of (':');
        if (colon == std::string::npos) {
          INFO ("malformed key/value entry (\"" + line + "\") in file \"" + gz.name() + "\" - ignored");
        } else {
          key   = strip (line.substr (0, colon));
          value = strip (line.substr (colon+1));
          if (key.empty() || value.empty()) {
            INFO ("malformed key/value entry (\"" + line + "\") in file \"" + gz.name() + "\" - ignored");
            key.clear();
            value.clear();
          }
        }
        return true;
      }






      void get_mrtrix_file_path (Header& H, const std::string& flag, std::string& fname, size_t& offset)
      {

        Header::iterator i = H.find (flag);
        if (i == H.end())
          throw Exception ("missing \"" + flag + "\" specification for MRtrix image \"" + H.name() + "\"");
        const std::string path = i->second;
        H.erase (i);

        std::istringstream file_stream (path);
        file_stream >> fname;
        offset = 0;
        if (file_stream.good()) {
          try {
            file_stream >> offset;
          }
          catch (...) {
            throw Exception ("invalid offset specified for file \"" + fname
                             + "\" in MRtrix image header \"" + H.name() + "\"");
          }
        }

        if (fname == ".") {
          if (offset == 0)
            throw Exception ("invalid offset specified for embedded MRtrix image \"" + H.name() + "\"");
          fname = H.name();
        } else {
          fname = Path::join (Path::dirname (H.name()), fname);
        }

      }




    }
  }
}
