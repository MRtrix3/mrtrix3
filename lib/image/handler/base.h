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


#ifndef __image_handler_base_h__
#define __image_handler_base_h__

#include <vector>
#include <stdint.h>
#include <unistd.h>
#include <cassert>

#include "ptr.h"
#include "datatype.h"
#include "file/entry.h"

#define MAX_FILES_PER_IMAGE 256U

namespace MR
{
  namespace Image
  {

    class Header;

    //! Classes responsible for actual image loading & writing
    /*! These classes are designed to provide a consistent interface for image
     * loading & writing, so that various non-trivial types of image storage
     * can be accommodated. These include compressed files, and images stored
     * as mosaic (e.g. Siemens DICOM mosaics). */
    namespace Handler
    {

      class Base
      {
        public:
          Base (const Image::Header& header);
          virtual ~Base ();

          void open ();
          void close ();

          void set_readwrite (bool readwrite) {
            writable = readwrite;
          }
          void set_image_is_new (bool image_is_new) {
            is_new = image_is_new;
          }

          uint8_t* segment (size_t n) const {
            assert (n < addresses.size());
            return addresses[n];
          }
          size_t nsegments () const {
            return addresses.size();
          }
          size_t segment_size () const {
            check();
            return segsize;
          }

          std::vector<File::Entry> files;

          void set_name (const std::string& image_name) {
            name = image_name;
          }

          void merge (const Base& B) {
            assert (addresses.empty());
            assert (datatype == B.datatype);
            for (size_t n = 0; n < B.files.size(); ++n) 
              files.push_back (B.files[n]);
            segsize += B.segsize;
          }

          friend std::ostream& operator<< (std::ostream& stream, const Base& B) {
            stream << "\"" << B.name << ", data type " << B.datatype << ", " << str (B.files.size()) << " files, segsize " << str(B.segsize)
                   << ", is " << (B.is_new ? "" : "NOT ") << "new, " << (B.writable ? "read/write" : "read-only");
            return stream;
          }

        protected:
          std::string name;
          const DataType datatype;
          size_t segsize;
          VecPtr<uint8_t,true> addresses;
          bool is_new, writable;

          void check () const {
            assert (addresses.size());
          }
          virtual void load () = 0;
          virtual void unload ();
      };

    }
  }
}

#endif

