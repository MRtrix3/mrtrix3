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


#ifndef __file_key_value_h__
#define __file_key_value_h__

#include <fstream>
#include "mrtrix.h"

namespace MR
{
  namespace File
  {

    class KeyValue
    {
      public:
        KeyValue () { }
        KeyValue (const std::string& file, const char* first_line = NULL) {
          open (file, first_line);
        }

        void  open (const std::string& file, const char* first_line = NULL);
        bool  next ();
        void  close () {
          in.close();
        }

        const std::string& key () const throw ()   {
          return (K);
        }
        const std::string& value () const throw () {
          return (V);
        }
        const std::string& name () const throw ()  {
          return (filename);
        }

      protected:
        std::string K, V, filename;
        std::ifstream in;
    };

  }
}

#endif

