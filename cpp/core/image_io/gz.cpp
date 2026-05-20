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

#include <limits>

#include "app.h"
#include "file/gz.h"
#include "header.h"
#include "image_io/gz.h"
#include "progressbar.h"
#include <fmt/format.h>

namespace MR::ImageIO {

void GZ::load(const Header &header, size_t) {
  if (files.empty())
    throw Exception(fmt::format("no files specified in header for image \"{}\"", header.name()));

  segsize /= files.size();
  bytes_per_segment = (header.datatype().bits() * segsize + 7) / 8;
  if (files.size() * bytes_per_segment > std::numeric_limits<size_t>::max())
    throw Exception(fmt::format("image \"{}\" is larger than maximum accessible memory", header.name()));

  DEBUG(fmt::format("loading image \"{}\"...", header.name()));
  addresses.resize(header.datatype().bits() == 1 && files.size() > 1 ? files.size() : 1);
  addresses[0].reset(new std::byte[files.size() * bytes_per_segment]);
  if (!addresses[0])
    throw Exception(fmt::format("failed to allocate memory for image \"{}\"", header.name()));

  if (is_new)
    memset(addresses[0].get(), 0, files.size() * bytes_per_segment);
  else {
    ProgressBar progress(fmt::format("uncompressing image \"{}\"", header.name()),
                         files.size() * bytes_per_segment / bytes_per_zcall);
    for (size_t n = 0; n < files.size(); n++) {
      File::GZ zf(files[n].path, "rb");
      zf.seek(files[n].start);
      std::byte *address = addresses[0].get() + n * bytes_per_segment;
      std::byte *last = address + bytes_per_segment - bytes_per_zcall;
      while (address < last) {
        zf.read(reinterpret_cast<char *>(address), bytes_per_zcall);
        address += bytes_per_zcall;
        ++progress;
      }
      last += bytes_per_zcall;
      zf.read(reinterpret_cast<char *>(address), last - address);
    }
  }

  if (addresses.size() > 1)
    // TODO this looks like it needs to be handled explicitly in unload()...
    for (size_t n = 1; n < addresses.size(); n++)
      addresses[n].reset(addresses[0].get() + n * bytes_per_segment);
  else
    segsize = std::numeric_limits<size_t>::max();
}

void GZ::unload(const Header &header) {
  if (!addresses.empty()) {
    assert(addresses[0]);

    if (writable) {
      ProgressBar progress(fmt::format("compressing image \"{}\"", header.name()),
                           files.size() * bytes_per_segment / bytes_per_zcall);
      for (size_t n = 0; n < files.size(); n++) {
        assert(files[n].start == static_cast<int64_t>(lead_in_size));
        File::GZ zf(files[n].path, "wb");
        if (lead_in)
          zf.write(reinterpret_cast<const char *>(lead_in.get()), lead_in_size);
        std::byte *address = addresses[0].get() + n * bytes_per_segment;
        std::byte *last = address + bytes_per_segment - bytes_per_zcall;
        while (address < last) {
          zf.write(reinterpret_cast<const char *>(address), bytes_per_zcall);
          address += bytes_per_zcall;
          ++progress;
        }
        last += bytes_per_zcall;
        zf.write(reinterpret_cast<const char *>(address), last - address);
        if (lead_out)
          zf.write(reinterpret_cast<const char *>(lead_out.get()), lead_out_size);
      }
    }
  }
}

} // namespace MR::ImageIO
