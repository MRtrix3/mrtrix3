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

#pragma once

#include <string>
#include <vector>

#include "file/gz.h"
#include "file/key_value.h"
#include "header.h"

namespace MR::Formats {

// Read generic image header information - common between conventional and compressed formats
template <class SourceType> void read_mrtrix_header(Header &, SourceType &);

// These are helper functiosn for reading key/value pairs from either a File::KeyValue construct,
//   or from a GZipped file (where the getline() function must be used explicitly)
bool next_keyvalue(File::KeyValue::Reader &, std::string &, std::string &);
bool next_keyvalue(File::GZ &, std::string &, std::string &);

// Get the path to a file - use same function for image data and sparse data
// Note that the 'file' field is read in as entries in the map<string, string>
//   by read_mrtrix_header(), and are therefore erased by this function so that they do not propagate
//   into new images created using this header
void get_mrtrix_file_path(Header &, std::string_view, std::string &, size_t &);

// Write generic image header information to a stream -
//   this could be an ofstream in the case of .mif, or a stringstream in the case of .mif.gz
template <class StreamType> void write_mrtrix_header(const Header &, StreamType &);

std::vector<ssize_t> parse_axes(size_t ndim, std::string_view specifier);

} // namespace MR::Formats
