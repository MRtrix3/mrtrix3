/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#include "image_io/png.h"

#include "file/png.h"
#include "header.h"
#include "image_helpers.h"

namespace MR::ImageIO {

void PNG::load(const Header &header, size_t) {
  DEBUG(std::string("loading PNG ")               //
        + (files.size() > 1 ? "images" : "image") //
        + " \"" + header.name() + "\"");          //
  segsize = (header.datatype().bits() * voxel_count(header) + 7) / 8;
  addresses.resize(1);
  addresses[0].reset(new uint8_t[segsize]);
  if (is_new) {
    memset(addresses[0].get(), 0x00, segsize);
  } else {
    const size_t slice_bytes = (header.datatype().bits() *                    //
                                    header.size(0) *                          //
                                    header.size(1) *                          //
                                    (header.ndim() == 4 ? header.size(3) : 1) //
                                + 7)                                          //
                               / 8;                                           //
    for (size_t i = 0; i != files.size(); ++i) {
      File::PNG::Reader png(files[i].name);
      if (png.get_width() != header.size(0) || png.get_height() != header.size(1) ||
          png.get_output_bitdepth() != static_cast<int>(header.datatype().bits()) ||
          ((header.ndim() > 3 && png.get_channels() != header.size(3)) ||
           (header.ndim() <= 3 && png.get_channels() > 1))) {
        Exception e("Inconsistent image properties within series \"" + header.name() + "\"");
        e.push_back("Series: " + str(header.size(0)) + "x" + str(header.size(1)) + " x " +
                    str(header.datatype().bits()) + " bits, " + (header.ndim() > 3 ? str(header.size(3)) : "1") +
                    " volumes");
        e.push_back("File \"" + files[i].name + ": " + str(png.get_width()) + "x" + str(png.get_height()) + " x " +
                    str(png.get_bitdepth()) + "(->" + str(png.get_output_bitdepth()) + ") bits, " +
                    str(png.get_channels()) + " channels");
        throw e;
      }
      png.load(addresses[0].get() + (i * slice_bytes));
    }
  }
}

void PNG::unload(const Header &header) {
  assert(addresses.size() == 1);
  if (writable) {
    const size_t slice_bytes = (header.datatype().bits() *                    //
                                    header.size(0) *                          //
                                    header.size(1) *                          //
                                    (header.ndim() == 4 ? header.size(3) : 1) //
                                + 7)                                          //
                               / 8;                                           //
    for (size_t i = 0; i != files.size(); i++) {
      File::PNG::Writer png(header, files[i].name);
      png.save(addresses[0].get() + (i * slice_bytes));
    }
  }
  delete[] addresses[0].release();
}

} // namespace MR::ImageIO

#endif
