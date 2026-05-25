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

#include "file/gz.h"
#include "file/ofstream.h"
#include "file/path.h"
#include "formats/list.h"
#include "formats/mrtrix_utils.h"
#include "header.h"
#include "image_io/gz.h"
#include <fmt/format.h>

namespace MR::Formats {

std::unique_ptr<ImageIO::Base> MRtrix_GZ::read(Header &H) const {
  if (!Path::has_suffix(H.path(), ".mif.gz"))
    return std::unique_ptr<ImageIO::Base>();
  const std::filesystem::path &hpath = static_cast<const Header &>(H).path();
  File::GZ zf(hpath, "r");
  std::string first_line = zf.getline();
  if (first_line != "mrtrix image") {
    zf.close();
    throw Exception("invalid first line for compressed image \"{}\"" //
                    " (expected \"mrtrix image\", read \"{}\")",     //
                    H.name(),
                    first_line);
  }
  read_mrtrix_header(H, zf);
  zf.close();

  std::filesystem::path filepath;
  size_t offset, write_offset;
  get_mrtrix_file_path(H, "file", filepath, offset);
  write_offset = offset;
  if (filepath != hpath)
    throw Exception("GZip-compressed MRtrix format images must have image data within the same file as the header");

  std::stringstream header;
  header << "mrtrix image\n";
  write_mrtrix_header(H, header);
  write_offset = header.str().size() + size_t(24);
  write_offset += ((4 - (offset % 4)) % 4);
  header << "file: . " << write_offset << "\nEND\n";

  std::unique_ptr<ImageIO::GZ> io_handler(new ImageIO::GZ(H, write_offset));
  memcpy(io_handler.get()->header(), header.str().c_str(), header.str().size());
  memset(io_handler.get()->header() + header.str().size(), 0, write_offset - header.str().size());
  io_handler->files.push_back(File::Entry(hpath, offset));

  return io_handler;
}

bool MRtrix_GZ::check(Header &H, size_t num_axes) const {
  if (!Path::has_suffix(H.path(), ".mif.gz"))
    return false;

  H.ndim() = num_axes;
  for (size_t i = 0; i < H.ndim(); i++)
    if (H.size(i) < 1)
      H.size(i) = 1;

  return true;
}

std::unique_ptr<ImageIO::Base> MRtrix_GZ::create(Header &H) const {
  std::stringstream header;
  header << "mrtrix image\n";
  write_mrtrix_header(H, header);

  int64_t offset = static_cast<int64_t>(header.tellp()) + int64_t(24);
  offset += ((4 - (offset % 4)) % 4);
  header << "file: . " << offset << "\nEND\n";
  while (static_cast<int64_t>(header.tellp()) < offset)
    header << '\0';

  std::unique_ptr<ImageIO::GZ> io_handler(new ImageIO::GZ(H, offset));
  memcpy(io_handler->header(), header.str().c_str(), offset);

  const std::filesystem::path &hpath = const_cast<const Header &>(H).path();
  File::OFStream data_file(hpath);
  data_file.close();
  io_handler->files.push_back(File::Entry(hpath, offset));

  return io_handler;
}

} // namespace MR::Formats
