/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */
#ifdef MRTRIX_TIFF_SUPPORT

#include "file/config.h"
#include "file/path.h"
#include "formats/list.h"
#include "header.h"
#include "file/tiff.h"
#include "image_io/tiff.h"


namespace MR
{
  namespace Formats
  {


    std::unique_ptr<ImageIO::Base> TIFF::read (Header& H) const
    {
      if (! (Path::has_suffix (H.name(), ".tiff") ||
            Path::has_suffix (H.name(), ".tif") ||
            Path::has_suffix (H.name(), ".TIFF") ||
            Path::has_suffix (H.name(), ".TIF"))) 
        return std::unique_ptr<ImageIO::Base>();

      File::TIFF tif (H.name());

      uint32 width(0), height(0);
      uint16 bpp(0), sampleformat(0), samplesperpixel(0), config (0);
      size_t ndir = 0;

      do {
        tif.read_and_check (TIFFTAG_IMAGEWIDTH, width);
        tif.read_and_check (TIFFTAG_IMAGELENGTH, height);
        tif.read_and_check (TIFFTAG_BITSPERSAMPLE, bpp);
        tif.read_and_check (TIFFTAG_SAMPLEFORMAT, sampleformat);
        tif.read_and_check (TIFFTAG_SAMPLESPERPIXEL, samplesperpixel);
        tif.read_and_check (TIFFTAG_PLANARCONFIG, config);

        ++ndir;
      } while (tif.read_directory() != 0);

      H.ndim() = samplesperpixel > 1 ? 4 : 3;

      H.size(0) = width; 
      H.stride(0) = 2;

      H.size(1) = height;
      H.stride(1) = 3;

      H.size(2) = ndir;
      H.stride(2) = 4;

      if (samplesperpixel > 1) {
        H.size(3) = samplesperpixel;
        H.stride(3) = (config == PLANARCONFIG_CONTIG ? 1 : 5);
      }

      H.datatype() = DataType::Undefined;
      switch (bpp) {
        case 8: 
          switch (sampleformat) {
            case 1: H.datatype() = DataType::UInt8; break;
            case 2: H.datatype() = DataType::Int8; break;
          }
          break;
        case 16:
          switch (sampleformat) {
            case 1: H.datatype() = DataType::UInt16; break;
            case 2: H.datatype() = DataType::Int16; break;
          }
          break;
        case 32:
          switch (sampleformat) {
            case 1: H.datatype() = DataType::UInt32; break;
            case 2: H.datatype() = DataType::Int32; break;
            case 3: H.datatype() = DataType::Float32; break;
          }
          break;
      }

      if (H.datatype() == DataType::Undefined)
        throw Exception ("unrecognised or unsupported data type in TIFF file \"" + H.name() + "\"");

      H.datatype().set_byte_order_native();

      std::unique_ptr<ImageIO::Base> io_handler (new ImageIO::TIFF (H));
      io_handler->files.push_back (File::Entry (H.name(), 0));

      return io_handler;
    }





    bool TIFF::check (Header& H, size_t num_axes) const
    {
      if (Path::has_suffix (H.name(), ".tiff") ||
          Path::has_suffix (H.name(), ".tif") ||
          Path::has_suffix (H.name(), ".TIFF") ||
          Path::has_suffix (H.name(), ".TIF")) 
        throw Exception ("TIFF format not supported for output");

      return false;
    }







    std::unique_ptr<ImageIO::Base> TIFF::create (Header& H) const
    {
      return std::unique_ptr<ImageIO::Base>();
    }

  }
}

#endif

