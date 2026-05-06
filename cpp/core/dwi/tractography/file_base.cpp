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

#include "dwi/tractography/file_base.h"
#include "file/path.h"
#include <fmt/format.h>

namespace MR::DWI::Tractography {

void __ReaderBase__::open(std::string_view file, std::string_view type, Properties &properties) {
  properties.clear();
  dtype = DataType::Undefined;

  const std::string firstline(fmt::format("mrtrix {}", type));
  File::KeyValue::Reader kv(file, firstline.c_str());
  std::string data_file;

  while (kv.next()) {
    const std::string key = lowercase(kv.key());
    if (key == "roi" || key == "prior_roi") {
      try {
        std::vector<std::string> V(split(kv.value(), " \t", true, 2));
        if (V.size() != 2)
          throw 1;
        properties.prior_rois.insert(std::pair<std::string, std::string>(V[0], V[1]));
      } catch (...) {
        WARN(fmt::format("invalid ROI specification in {} file \"{}\" - ignored", type, file));
      }
    } else if (key == "comment")
      properties.comments.emplace_back(std::string(kv.value()));
    else if (key == "file")
      data_file = kv.value();
    else if (key == "datatype")
      dtype = DataType::parse(kv.value());
    else
      add_line(properties[std::string(kv.key())], kv.value());
  }

  if (dtype == DataType::Undefined)
    throw Exception(fmt::format("no datatype specified for tracks file \"{}\"", file));
  if (dtype != DataType::Float32LE && dtype != DataType::Float32BE && dtype != DataType::Float64LE &&
      dtype != DataType::Float64BE)
    throw Exception(fmt::format("only supported datatype for tracks file are \"\n                    \"Float32LE, "
                                "Float32BE, Float64LE & Float64BE (in {} file \"{}\")",
                                type,
                                file));

  if (data_file.empty())
    throw Exception(fmt::format("missing \"files\" specification for {} file \"{}\"", type, file));

  std::istringstream files_stream(data_file);
  std::string fname;
  files_stream >> fname;
  int64_t offset = 0;
  if (files_stream.good()) {
    try {
      files_stream >> offset;
    } catch (...) {
      throw Exception(fmt::format("invalid offset specified for file \"{}\" in {} file \"{}\"", fname, type, file));
    }
  }

  if (fname != ".")
    fname = Path::join(Path::dirname(file), fname);
  else
    fname = file;

  in.open(fname.c_str(), std::ios::in | std::ios::binary);
  if (!in)
    throw Exception(fmt::format("error opening {} data file \"{}\": {}", type, fname, strerror(errno)));
  in.seekg(offset);
}

} // namespace MR::DWI::Tractography
