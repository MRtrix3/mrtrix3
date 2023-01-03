/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __file_png_h__
#define __file_png_h__

#ifdef MRTRIX_PNG_SUPPORT

#include <png.h>

#include "header.h"
#include "raw.h"
#include "types.h"
#include "image_io/fetch_store.h"

namespace MR
{
  namespace File
  {
    namespace PNG
    {

      class Reader
      { MEMALIGN(Reader)
        public:
          Reader (const std::string& filename);
          ~Reader();

          uint32_t get_width()  const { return width; }
          uint32_t get_height() const { return height; }
          int get_bitdepth() const { return bit_depth; }
          int get_colortype() const { return color_type; }
          int get_channels() const { return channels; }

          size_t get_size() const { return png_get_rowbytes(png_ptr, info_ptr) * height; }
          int get_output_bitdepth() const { return output_bitdepth; }
          bool has_transparency() const { return png_get_valid (png_ptr, info_ptr, PNG_INFO_tRNS); }

          void set_expand();

          void load (uint8_t*);

        private:
          png_structp png_ptr;
          png_infop info_ptr;
          png_uint_32 width, height;
          int bit_depth, color_type;
          uint8_t channels;

          int output_bitdepth;

      };



      class Writer
      { MEMALIGN(Writer)
        public:
          Writer (const Header&, const std::string&);
          ~Writer();

          size_t get_size() const { return png_get_rowbytes(png_ptr, info_ptr) * height;}

          void save (uint8_t*);

        private:
          png_structp png_ptr;
          png_infop info_ptr;
          uint32_t width, height;
          int color_type, bit_depth;
          std::string filename;
          DataType data_type;
          FILE* outfile;

          static void error_handler (png_struct_def*, const char*);
          static jmp_buf jmpbuf;

          template <typename T>
          void fill (uint8_t* in_ptr, uint8_t* out_ptr, const DataType data_type, const size_t num_elements);

      };


      template <typename T>
      void Writer::fill (uint8_t* in_ptr, uint8_t* out_ptr, const DataType data_type, const size_t num_elements)
      {
        std::function<default_type(const void*,size_t,default_type,default_type)> fetch_func;
        std::function<void(default_type,void*,size_t,default_type,default_type)> store_func;
        __set_fetch_store_functions<default_type> (fetch_func, store_func, data_type);
        default_type multiplier = 1.0;
        switch (data_type() & DataType::Type) {
          case DataType::Float32: multiplier = std::numeric_limits<uint8_t>::max(); break;
          case DataType::Float64: multiplier = std::numeric_limits<uint16_t>::max(); break;
        }
        for (size_t i = 0; i != num_elements; ++i) {
          Raw::store_BE<T> (std::min (default_type(std::numeric_limits<T>::max()), std::max (0.0, std::round(multiplier * fetch_func (in_ptr, 0, 0.0, 1.0)))), out_ptr);
          in_ptr += data_type.bytes();
          out_ptr += sizeof(T);
        }
      };



    }
  }
}

#endif
#endif



