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

#ifdef MRTRIX_PNG_SUPPORT

#include "file/png.h"

#include <chrono>
#include <zlib.h>

#include "app.h"
#include "datatype.h"
#include "exception.h"
#include "mrtrix.h"
#include "file/path.h"


namespace MR
{
  namespace File
  {
    namespace PNG
    {



      Reader::Reader (const std::string& filename) :
          png_ptr (NULL),
          info_ptr (NULL),
          width (0),
          height (0),
          bit_depth (0),
          color_type (0),
          channels (0)
      {
        FILE* infile = fopen (filename.c_str(), "rb");
        unsigned char sig[8];
        if (fread (sig, 1, 8, infile) < 8)
          throw Exception ("error reading from PNG file \"" + filename + "\"");
        const int sigcmp = png_sig_cmp(sig, 0, 8);
        if (sigcmp) {
          fclose (infile);
          std::stringstream s;
          for (size_t i = 0; i != 8; ++i) {
            s << str(int(sig[i])) << " ";
          }
          Exception e ("Bad PNG signature in file \"" + filename + "\"");
          e.push_back ("File signature: " + s.str());
          throw e;
        }
        if (!(png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL))) {
          fclose (infile);
          throw Exception ("Unable to allocate memory for PNG read structure for image \"" + filename + "\"");
        }
        if (!(info_ptr = png_create_info_struct (png_ptr))) {
          fclose (infile);
          png_destroy_read_struct (&png_ptr, NULL, NULL);
          throw Exception ("Unable to allocate memory for PNG info structure for image \"" + filename + "\"");
        }
        if (setjmp(png_jmpbuf(png_ptr))) {
          fclose (infile);
          png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
          throw Exception ("Fatal error reading PNG image \"" + filename + "\"");
        }
        png_init_io (png_ptr, infile);
        png_set_sig_bytes (png_ptr, 8);
        png_read_info (png_ptr, info_ptr);
        png_get_IHDR (png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

        switch (color_type) {
          case PNG_COLOR_TYPE_GRAY:
          case PNG_COLOR_TYPE_GRAY_ALPHA:
          case PNG_COLOR_TYPE_PALETTE:
          case PNG_COLOR_TYPE_RGB:
          case PNG_COLOR_TYPE_RGB_ALPHA:
            break;
          default:
            throw Exception ("Invalid color type (" + str(color_type) + ") in PNG file \"" + filename + "\"");
        }
        switch (bit_depth) {
          case 1:
          case 2:
          case 4:
          case 8:
          case 16:
            break;
          default:
            throw Exception ("Invalid bit depth (" + str<int>(bit_depth) + ") in PNG file \"" + filename + "\"");
        }

        output_bitdepth = bit_depth;

        if ((color_type == PNG_COLOR_TYPE_PALETTE) ||
            (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8 && !(bit_depth == 1 && !(width%8))) ||
            (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))) {
          set_expand();
        }
        png_set_interlace_handling (png_ptr);
        png_read_update_info (png_ptr, info_ptr);
        channels = png_get_channels (png_ptr, info_ptr);
        DEBUG ("PNG image \"" + filename + "\":  " +
               str(width) + "x" + str(height) + "; " +
               "bitdepth = " + str(bit_depth) + "; " +
               "colortype: " + str(color_type) + "; " +
               "channels = " + str<int>(channels) + "; " +
               "bytes per row = " + str(png_get_rowbytes(png_ptr, info_ptr)) + "; " +
               "output bitdepth = " + str(output_bitdepth) + "; " +
               "total bytes = " = str(get_size()));
      }



      Reader::~Reader()
      {
        if (png_ptr && info_ptr) {
          png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
          png_ptr = NULL;
          info_ptr = NULL;
        }
      }



      void Reader::set_expand()
      {
        png_set_expand (png_ptr);
        output_bitdepth = std::max (8, bit_depth);
      }



      void Reader::load (uint8_t* image_data)
      {
        if (setjmp(png_jmpbuf(png_ptr))) {
          png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
          throw Exception ("Fatal error reading PNG image");
        }
        const int row_bytes = png_get_rowbytes (png_ptr, info_ptr);
        png_bytepp row_pointers = new png_bytep[height];
        for (png_uint_32 i = 0; i != height; ++i)
          row_pointers[i] = image_data + i*row_bytes;
        png_read_image (png_ptr, row_pointers);
        delete[] row_pointers;
        row_pointers = NULL;
      }
















      jmp_buf Writer::jmpbuf;



      Writer::Writer (const Header& H, const std::string& filename) :
          png_ptr (NULL),
          info_ptr (NULL),
          color_type (0),
          bit_depth (0),
          filename (filename),
          data_type (H.datatype()),
          outfile (NULL)
      {
        if (Path::exists (filename) && !App::overwrite_files)
          throw Exception ("output file \"" + filename + "\" already exists (use -force option to force overwrite)");
        if (!(png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, this, &error_handler, NULL)))
          throw Exception ("Unable to create PNG write structure for image \"" + filename + "\"");
        if (!(info_ptr = png_create_info_struct (png_ptr)))
          throw Exception ("Unable to create PNG info structure for image \"" + filename + "\"");
        if (setjmp (jmpbuf)) {
          png_destroy_write_struct (&png_ptr, &info_ptr);
          throw Exception ("Unable to set jump buffer for PNG structure for image \"" + filename + "\"");
        }
        outfile = fopen (filename.c_str(), "wb");
        png_init_io (png_ptr, outfile);
        png_set_compression_level (png_ptr, Z_DEFAULT_COMPRESSION);
        switch (H.ndim()) {
          case 2:
          case 3:
            color_type = PNG_COLOR_TYPE_GRAY;
            break;
          case 4:
            switch (H.size(3)) {
              case 1: color_type = PNG_COLOR_TYPE_GRAY; break;
              case 2: color_type = PNG_COLOR_TYPE_GRAY_ALPHA; break;
              case 3: color_type = PNG_COLOR_TYPE_RGB; break;
              case 4: color_type = PNG_COLOR_TYPE_RGB_ALPHA; break;
              default:
                png_destroy_write_struct (&png_ptr, &info_ptr);
                throw Exception ("Unsupported number of volumes (" + str(H.size(3)) + ") in image \"" + filename + "\" for PNG writer");
            }
            break;
          default:
            png_destroy_write_struct (&png_ptr, &info_ptr);
            throw Exception ("Unsupported image dimensionality (" + str(H.ndim()) + ") in image \"" + filename + "\" for PNG writer");
        }
        if (data_type.is_complex()) {
          png_destroy_write_struct (&png_ptr, &info_ptr);
          throw Exception ("Complex datatype from image \"" + H.name() + "\" not supported by PNG format");
        }
        if (data_type.is_floating_point()) {
          INFO ("Data to be converted to PNG is floating-point; "
                "image will be scaled to integer representation assuming input data is in the range 0.0 - 1.0");
        }
        switch (data_type() & DataType::Type) {
          case DataType::Undefined:
            png_destroy_write_struct (&png_ptr, &info_ptr);
            throw Exception ("Undefined data type in image \"" + H.name() + "\" for PNG writer");
          case DataType::Bit:
            bit_depth = 1;
            break;
          case DataType::UInt8:
          case DataType::Float32:
            bit_depth = 8;
            break;
          case DataType::UInt16:
          case DataType::UInt32:
          case DataType::UInt64:
          case DataType::Float64:
            bit_depth = 16;
            break;
        }
        // Detect cases where one axis has a size of 1, and hence represents the image plane
        //   being set via selection of a single slice rather than axis permutation
        // Note that H may be 3D or 4D, all with non-unity size, in which case
        //   axis 2 forms the image plane
        size_t width_axis = 0, height_axis = 1;
        if (H.ndim() > 2 && H.size(2) != 1) {
          if (H.size(0) == 1 && H.size(1) != 1) {
            width_axis = 1;
            height_axis = 2;
          } else if (H.size(1) == 1 && H.size(0) != 1) {
            width_axis = 0;
            height_axis = 2;
          }
        }
        width = H.size (width_axis);
        height = H.size (height_axis);
        png_set_IHDR (png_ptr, info_ptr, width, height, bit_depth, color_type, PNG_INTERLACE_NONE,
                      PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
        png_set_gAMA (png_ptr, info_ptr, 1.0);
        png_time modtime;
        png_convert_from_time_t (&modtime, std::chrono::system_clock::to_time_t (std::chrono::system_clock::now()));
        png_set_tIME (png_ptr, info_ptr, &modtime);
        png_text text[4];
        const std::string title_key ("Title");
        const std::string title_text (filename);
        const std::string software_key ("Software");
        const std::string software_text ("MRtrix3");
        const std::string source_key ("Source");
        const std::string source_text (MR::App::mrtrix_version);
        const std::string url_key ("URL");
        const std::string url_text ("www.mrtrix.org");
        text[0].compression = PNG_TEXT_COMPRESSION_NONE;
        text[0].key = const_cast<char*> (title_key.c_str());
        text[0].text = const_cast<char*> (title_text.c_str());
        text[1].compression = PNG_TEXT_COMPRESSION_NONE;
        text[1].key = const_cast<char*> (software_key.c_str());
        text[1].text = const_cast<char*> (software_text.c_str());
        text[2].compression = PNG_TEXT_COMPRESSION_NONE;
        text[2].key = const_cast<char*> (source_key.c_str());
        text[2].text = const_cast<char*> (source_text.c_str());
        text[3].compression = PNG_TEXT_COMPRESSION_NONE;
        text[3].key = const_cast<char*> (url_key.c_str());
        text[3].text = const_cast<char*> (url_text.c_str());
        png_set_text (png_ptr, info_ptr, text, 4);
        png_write_info (png_ptr, info_ptr);
        // Note: png_set_packing NOT being called; this will result in 8 pixels per byte for mask images
        //   (if this function _were_ called, it would instead be spread over 1 byte per pixel)
      }



      Writer::~Writer()
      {
        if (png_ptr && info_ptr) {
          png_destroy_write_struct (&png_ptr, &info_ptr);
          png_ptr = NULL;
          info_ptr = NULL;
        }
      }



      void Writer::save (uint8_t* data)
      {
        if (setjmp (jmpbuf)) {
          png_destroy_write_struct (&png_ptr, &info_ptr);
          png_ptr = NULL;
          info_ptr = NULL;
          throw Exception ("Unable to set jump buffer for PNG structure for image \"" + filename + "\"");
        }
        const size_t row_bytes = png_get_rowbytes (png_ptr, info_ptr);

        auto finish = [&] (uint8_t* to_write)
        {
          png_bytepp row_pointers = new png_bytep[height];
          for (size_t row = 0; row != height; ++row)
            row_pointers[row] = to_write + row * row_bytes;
          png_write_image (png_ptr, row_pointers);
          png_write_end (png_ptr, info_ptr);
        };


        if (bit_depth == 1 || data_type == DataType::UInt8 || data_type == DataType::UInt16BE) {
          finish (data);
        } else {
          uint8_t scratch[row_bytes * height];
          // Convert from "data" into "scratch"
          // This may include changes to fundamental type, changes to bit depth, changes to endianness
          uint8_t* in_ptr = data, *out_ptr = scratch;
          const uint8_t channels = png_get_channels (png_ptr, info_ptr);
          const size_t num_elements = channels * width * height;
          switch (bit_depth) {
            case 8:  fill<uint8_t>  (in_ptr, out_ptr, data_type, num_elements); break;
            case 16: fill<uint16_t> (in_ptr, out_ptr, data_type, num_elements); break;
            default: assert (0);
          }
          finish (scratch);
        }
      }



      void Writer::error_handler (png_struct_def* data, const char* msg)
      {
        Exception e ("Encountered critical error during PNG write: ");
        e.push_back (msg);
        throw e;
      }



    }
  }
}

#endif

