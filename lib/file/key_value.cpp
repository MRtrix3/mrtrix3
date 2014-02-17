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


#include <fstream>
#include "file/key_value.h"


namespace MR
{
  namespace File
  {

    void KeyValue::open (const std::string& file, const char* first_line)
    {
      filename.clear();
      DEBUG ("reading key/value file \"" + file + "\"...");

      in.open (file.c_str(), std::ios::in | std::ios::binary);
      if (!in) 
        throw Exception ("failed to open key/value file \"" + file + "\": " + strerror (errno));
      if (first_line) {
        std::string sbuf;
        getline (in, sbuf);
        if (sbuf.compare (0, strlen (first_line), first_line)) {
          in.close();
          throw Exception ("invalid first line for key/value file \"" + file + "\" (expected \"" + first_line + "\")");
        }
      }
      filename = file;
    }





    bool KeyValue::next ()
    {
      while (in.good()) {
        std::string sbuf;
        getline (in, sbuf);
        if (in.bad()) 
          throw Exception ("error reading key/value file \"" + filename + "\": " + strerror (errno));

        sbuf = strip (sbuf.substr (0, sbuf.find_first_of ('#')));
        if (sbuf == "END") {
          in.setstate (std::ios::eofbit);
          return false;
        }

        if (sbuf.size()) {
          size_t colon = sbuf.find_first_of (':');
          if (colon == std::string::npos) {
            INFO ("malformed key/value entry (\"" + sbuf + "\") in file \"" + filename + "\" - ignored");
          }
          else {
            K = strip (sbuf.substr (0, colon));
            V = strip (sbuf.substr (colon+1));
            if (K.empty()) {
              INFO ("malformed key/value entry (\"" + sbuf + "\") in file \"" + filename + "\" - ignored");
            }
            else 
              return true;
          }

        }
      }
      return false;
    }



  }
}


