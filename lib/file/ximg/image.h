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


#ifndef __file_ximg_image_h__
#define __file_ximg_image_h__

#include <time.h>

#include "debug.h"
#include "point.h"
#include "get_set.h"
#include "datatype.h"
#include "file/path.h"
#include "file/mmap.h"
#include "file/entry.h"
#include "image/header.h"


namespace MR
{
  namespace File
  {
    namespace ImageSlice 
    {

      class Base : public Entry {
        public:
          Base (const std::string& filename, int64_t offset) : 
            Entry (filename, offset) { }
          virtual ~Base() { }
          virtual bool operator< (const Base& x) const {
            return name.compare (x.name) < 0;
          }
          virtual void complete (Image::Header& H) = 0;
      };


      class XIMG : public Base
      {
        public:
          static Base* read (const std::string& filename, std::string& key)  
            {
              if (!check_filename (filename)) 
                throw Exception ("file \"" + filename + " is not in IMGF format: invalid filename");

              File::MMap mmap (filename);
              const char* bof = reinterpret_cast<const char*> (mmap.address());

              if (std::string ((const char*) bof, 4) != "IMGF") 
                throw Exception ("file \"" + filename + " is not in IMGF format: invalid magic number");

              int nseries = series_num (bof);
              key = patient(bof) + ";" + study_name(bof) + ";" + study_datetime(bof) + ";" + series_name(bof) + "[" + str(int(nseries/20)) + "]";
              return new XIMG (filename, getBE<int32_t> (bof + 0x4), getBE<int16_t> (image (bof) + 0xc) + 1000 * int (nseries / 20));
            }

          XIMG (const std::string& filename, int64_t offset, int num) :
            Base (filename, offset), instance (num) { }

          virtual bool operator< (const XIMG& x) const {
            return instance < x.instance;
          }

          virtual void complete (Image::Header& H) {/*
                                                       int dim[2], depth, instance, echo;
                                                       Point<> position, row, column, normal;
                                                       float pixel_size[2], slice_thickness, slice_spacing, distance;
                                                       float TR, TI, TE;
                                                       dim[0] = getBE<int32_t> (bof() + 0x8);
                                                       dim[1] = getBE<int32_t> (bof() + 0xc);
                                                       depth = getBE<int32_t> (bof() + 0x10);
                                                       int nseries = getBE<int16_t> (series() + 0xa);
                                                       echo = getBE<int16_t> (image() + 0xd4);
                                                       TR = 1.0e-3 * getBE<int32_t> (image() + 0xc2);
                                                       TI = 1.0e-3 * getBE<int32_t> (image() + 0xc6);
                                                       TE = 1.0e-3 * getBE<int32_t> (image() + 0xca);
                                                       slice_thickness = getBE<float32> (image() + 0x1a);

                                                       pixel_size[0] = getBE<float32> (image() + 0x32);
                                                       pixel_size[1] = getBE<float32> (image() + 0x36);

                                                       distance = getBE<float32> (image() + 0x7e);
                                                       position = Point<> (getBE<float32> (image() + 0x9a), getBE<float32> (image() + 0x9e), getBE<float32> (image() + 0xa2));
                                                       Point<> topright (getBE<float32> (image() + 0xa6), getBE<float32> (image() + 0xaa), getBE<float32> (image() + 0xae));
                                                       Point<> bottomright (getBE<float32> (image() + 0xb2), getBE<float32> (image() + 0xb6), getBE<float32> (image() + 0xba));

                                                       row = topright - position;
                                                       column = bottomright - topright;
                                                       normal = Point<> (getBE<float32> (image() + 0x8e), getBE<float32> (image() + 0x92), getBE<float32> (image() + 0x96));

                                                       row.normalise();
                                                       column.normalise();
                                                       normal.normalise();
                                                     */
            /*
               VAR (distance);
               VAR (position);
               VAR (row);
               VAR (column);
               VAR (normal);
             */
          }

        private:
          int instance;
          static const char* user (const char* bof) {
            return bof + getBE<int32_t> (bof + 0x74);
          }
          static const char* suite (const char* bof) {
            return bof + getBE<int32_t> (bof + 0x7c);
          }
          static const char* exam (const char* bof) {
            return bof + getBE<int32_t> (bof + 0x84);
          }
          static const char* series (const char* bof) {
            return bof + getBE<int32_t> (bof + 0x8c);
          }
          static const char* image (const char* bof) {
            return bof + getBE<int32_t> (bof + 0x94);
          }

          static bool check_filename (const std::string& filename) {
            std::string basename (Path::basename (filename));
            if (basename.size() != 5) 
              return false;
            if (basename.substr (0, 2) != "I.") 
              return false;
            for (size_t n = 2; n < 5; ++n)
              if (!isdigit (basename[n]))
                return false;

            return true; 
          }

          static std::string cleanup (const std::string& field) {
            std::string s (field);
            for (size_t n = s.size()-1; n > 0; --n) {
              if (!isalnum(s[n]) && s[n] != '-')
                s[n] = '\0';
              else 
                return s;
            }
            return s;
          }

          static std::string patient (const char* bof) {
            return cleanup (std::string (exam (bof) + 0x61, 25) + " " + std::string (exam (bof) + 0x54, 13));
          }

          static std::string study_datetime (const char* bof) {
            time_t tbuf = getBE<int32_t> (exam (bof) + 0xd0);
            struct tm* T = gmtime (&tbuf);
            return MR::printf ("%04d-%02d-%02d %02d:%02d", 1900 + T->tm_year, 1+T->tm_mon, T->tm_mday, T->tm_hour, T->tm_min);
          }

          static std::string study_name (const char* bof) {
            return cleanup (std::string (exam (bof) + 0x11a, 23));
          }
          static std::string series_name (const char* bof) {
            return cleanup (std::string (series (bof) + 0x14, 30));
          }
          static int series_num (const char* bof) {
            return getBE<int16_t> (series (bof) + 0xa) % 20;
          }
      };


    }
  }
}

#endif




