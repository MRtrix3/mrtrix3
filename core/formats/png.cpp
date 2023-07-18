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

#include "formats/list.h"

#include "header.h"
#include "file/name_parser.h"
#include "file/path.h"
#include "file/png.h"
#include "image_io/base.h"
#include "image_io/png.h"

namespace MR
{
  namespace Formats
  {



    std::unique_ptr<ImageIO::Base> PNG::read (Header& H) const
    {
      if (!(Path::has_suffix (H.name(), ".png") ||
            Path::has_suffix (H.name(), ".PNG")))
        return std::unique_ptr<ImageIO::Base>();

      File::PNG::Reader png (H.name());

      switch (png.get_colortype()) {
        case PNG_COLOR_TYPE_GRAY:
          H.ndim() = 3;
          break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
          H.ndim() = 4;
          H.size(3) = 2;
          break;
        case PNG_COLOR_TYPE_PALETTE:
        case PNG_COLOR_TYPE_RGB:
          H.ndim() = 4;
          H.size(3) = 3;
          break;
        case PNG_COLOR_TYPE_RGB_ALPHA:
          H.ndim() = 4;
          H.size(3) = 4;
          break;
        default:
          throw Exception ("Unsupported color type in PNG image \"" + H.name() + "\" (" + str(png.get_colortype()) + ")");
      }
      if (png.has_transparency()) {
        if (H.ndim() == 3) {
          H.ndim() = 4;
          H.size(3) = 2;
        } else {
          ++H.size(3);
        }
      }

      H.size(0) = png.get_width();
      H.stride(0) = -3;

      H.size(1) = png.get_height();
      H.stride(1) = -4;

      H.size(2) = 1;
      H.stride(2) = 1;

      if (H.ndim() == 4)
        H.stride(3) = 2;

      H.spacing (0) = H.spacing (1) = H.spacing (2) = 1.0;
      H.transform().setIdentity();

      H.datatype() = DataType::Undefined;
      switch (png.get_bitdepth()) {
        case 1:
          if (png.get_colortype() == PNG_COLOR_TYPE_PALETTE) {
            H.datatype() = DataType::UInt8;
          } else if (png.get_width() % 8) {
            WARN ("Bitwise PNG being read with width not a factor of 8; will be converted to UInt8 datatype");
            H.datatype() = DataType::UInt8;
          } else {
            H.datatype() = DataType::Bit;
          }
          break;
        case 2:
          H.datatype() = DataType::UInt8;
          break;
        case 4:
          H.datatype() = DataType::UInt8;
          break;
        case 8:
          H.datatype() = DataType::UInt8;
          break;
        case 16:
          H.datatype() = DataType::UInt16BE;
          break;
        default:
          throw Exception ("Unexpected bit depth (" + str(png.get_bitdepth()) + ") in PNG image \"" + H.name() + "\"");
      }

      std::unique_ptr<ImageIO::Base> io_handler (new ImageIO::PNG (H));
      io_handler->files.push_back (File::Entry (H.name(), 0));

      return io_handler;
    }



    bool PNG::check (Header& H, size_t num_axes) const
    {
      if (!(Path::has_suffix (H.name(), ".png") ||
            Path::has_suffix (H.name(), ".PNG")))
        return false;

      if (H.datatype().is_complex())
        throw Exception ("PNG format does not support complex data");

      if (H.ndim() == 4 && H.size(3) > 4)
        throw Exception ("A 4D image written to PNG must have between one and four volumes (requested: " + str(H.size(3)) + ")");

      // TODO After looping over axes via square-bracket notation,
      //   there needs to be at least two axes with size greater than one
      size_t unity_axes = 0;
      for (size_t axis = 0; axis != H.ndim(); ++axis) {
        if (H.size (axis) == 1)
          ++unity_axes;
      }
      if (unity_axes - (H.ndim() - num_axes) < 2)
        throw Exception ("Too few (non-unity) image axes to support PNG export");

      // For 4D images, can support:
      // - 1 volume (just greyscale)
      // - 2 volumes (save as greyscale & alpha)
      // - 3 volumes (save as RGB)
      // - 4 volumes (save as RGBA)
      // This needs to be compatible with NameParser used in Header::create():
      //   "num_axes" subtracts from H.ndim() however many instances of [] there are
      size_t width_axis = 0, axis_to_zero = 3;
      if (H.ndim() - num_axes > 1)
        throw Exception ("Cannot nominate more than one axis using square-bracket notation for PNG format");
      switch (num_axes) {
        case 1:
          throw Exception ("Cannot generate PNG image with only 1 axis");
        case 2:
          if (H.ndim() == 3) {
            // Which axis do we remove?
            // If axis 0 or 1 is size 1, then let's leave the header as 3D
            // If however all 3 axes are greater than 1, we're going to remove
            //   the final axis, which will then be looped over via the NameParser
            if (H.size(0) > 1 && H.size(1) > 1)
              H.ndim() = 2;
          }
          break;
        case 3:
          if (H.size(1) == 1) {
            axis_to_zero = 1;
          } else if (H.size(0) == 1) {
            axis_to_zero = 0;
            width_axis = 1;
          } else {
            // If image is 3D, and all three axes have size greater than one, and we
            //   haven't used the square-bracket notation, we can't export genuine 3D data
            if (H.ndim() == 3 && H.size(2) > 1)
              throw Exception ("Cannot export 3D image to PNG format if all three axes have size greater than 1 and square-bracket notation is not used");
            // By default generate one image per slice (axis #2) if image is 4D but
            //   square bracket notation has been used
            axis_to_zero = 2;
          }
          break;
        case 4:
          // Can only export a 4D image if one of the three spatial axes has size 1
          ssize_t axis;
          for (axis = 2; axis >= 0; --axis) {
            if (H.size(axis) == 1) {
              axis_to_zero = axis;
              break;
            }
          }
          if (axis < 0)
            throw Exception ("Cannot export 4D image to PNG format if all three spatial axes have size greater than 1 and square-bracket notation is not used");
          if (!axis_to_zero)
            width_axis = 1;
          break;
        default:
          throw Exception ("Cannot generate PNG file(s) from image with more than 4 axes");
      }

      // Set strides:
      // - If there is more than one volume (= channel in PNG), these need to be contiguous
      // - Data should run across width of image, then height
      // - Strides should be reversed if necessary in order for image orientations to be
      //     as expected compared to e.g. mrview
      H.stride(0) = -2;
      H.spacing(0) = 1.0;
      H.stride(1) = -3;
      H.spacing(1) = 1.0;
      if (H.ndim() > 2) {
        H.stride(2) = 4;
        H.spacing(2) = 1.0;
      }
      if (H.ndim() > 3) {
        H.stride(3) = 1;
        H.spacing(3) = NaN;
      }

      // Set axis that will be consumed by NameParser
      if (axis_to_zero != 3)
        H.stride (axis_to_zero) = 0;

      H.transform().setIdentity();

      if (H.datatype() == DataType::Bit && H.size (width_axis) % 8) {
        WARN ("Cannot write bitwise PNG image with width not a factor of 8; will instead write with 8-bit depth");
        H.datatype() = DataType::UInt8;
      }

      return true;
    }



    std::unique_ptr<ImageIO::Base> PNG::create (Header& H) const
    {
      std::unique_ptr<ImageIO::Base> io_handler (new ImageIO::PNG (H));
      io_handler->files.push_back (File::Entry (H.name(), 0));
      return io_handler;
    }



  }
}

#endif

