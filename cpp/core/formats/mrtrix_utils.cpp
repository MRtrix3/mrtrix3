/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include "formats/mrtrix_utils.h"

#include "file/ofstream.h"
#include "file/path.h"
#include "file/utils.h"
#include "mrtrix.h"
#include "types.h"

namespace MR::Formats {

std::vector<ssize_t> parse_axes(size_t ndim, std::string_view specifier) {
  std::vector<ssize_t> parsed(ndim);

  size_t sub = 0;
  size_t lim = 0;
  size_t end = specifier.size();
  size_t current = 0;

  try {
    while (sub <= end) {
      bool pos = true;
      if (specifier[sub] == '+') {
        pos = true;
        sub++;
      } else if (specifier[sub] == '-') {
        pos = false;
        sub++;
      } else if (!isdigit(specifier[sub]))
        throw 0;

      lim = sub;

      while (isdigit(specifier[lim]))
        lim++;
      if (specifier[lim] != ',' && specifier[lim] != '\0')
        throw 0;
      parsed[current] = to<ssize_t>(specifier.substr(sub, lim - sub)) + 1;
      if (!pos)
        parsed[current] = -parsed[current];

      sub = lim + 1;
      current++;
    }
  } catch (int) {
    throw Exception("malformed axes specification \"" + specifier + "\"");
  }

  if (current != ndim)
    throw Exception("incorrect number of axes in axes specification \"" + specifier + "\"");

  if (parsed.size() != ndim)
    throw Exception("incorrect number of dimensions for axes specifier");
  for (size_t n = 0; n < parsed.size(); n++) {
    if (!parsed[n] || static_cast<size_t>(MR::abs(parsed[n])) > ndim)
      throw Exception("axis ordering " + str(parsed[n]) + " out of range");

    for (size_t i = 0; i < n; i++)
      if (MR::abs(parsed[i]) == MR::abs(parsed[n]))
        throw Exception("duplicate axis ordering (" + str(MR::abs(parsed[n])) + ")");
  }

  return parsed;
}

bool next_keyvalue(File::KeyValue::Reader &kv, std::string &key, std::string &value) {
  key.clear();
  value.clear();
  if (!kv.next())
    return false;
  key = kv.key();
  value = kv.value();
  return true;
}

bool next_keyvalue(File::GZ &gz, std::string &key, std::string &value) {
  key.clear();
  value.clear();
  std::string line = gz.getline();
  line = strip(line.substr(0, line.find_first_of('#')));
  if (line.empty() || line == "END")
    return false;

  size_t colon = line.find_first_of(':');
  if (colon == std::string::npos) {
    INFO("malformed key/value entry (\"" + line + "\") in file \"" + gz.name() + "\" - ignored");
  } else {
    key = strip(line.substr(0, colon));
    value = strip(line.substr(colon + 1));
    if (key.empty() || value.empty()) {
      INFO("malformed key/value entry (\"" + line + "\") in file \"" + gz.name() + "\" - ignored");
      key.clear();
      value.clear();
    }
  }
  return true;
}

void get_mrtrix_file_path(Header &H, std::string_view flag, std::string &fname, size_t &offset) {

  auto i = H.keyval().find(std::string(flag));
  if (i == H.keyval().end())
    throw Exception("missing \"" + std::string(flag) + "\" specification for MRtrix image \"" + H.name() + "\"");
  const std::string path = i->second;
  H.keyval().erase(i);

  std::istringstream file_stream(path);
  file_stream >> fname;
  offset = 0;
  if (file_stream.good()) {
    try {
      file_stream >> offset;
    } catch (...) {
      throw Exception("invalid offset specified for file \"" + fname + "\"" + //
                      " in MRtrix image header \"" + H.name() + "\"");        //
    }
  }

  if (fname == ".") {
    if (offset == 0)
      throw Exception("invalid offset specified for embedded MRtrix image \"" + H.name() + "\"");
    fname = H.name();
  } else {
    if (fname[0] != PATH_SEPARATORS[0]
#ifdef MRTRIX_WINDOWS
        && fname[0] != PATH_SEPARATORS[1]
#endif
    )
      fname = Path::join(Path::dirname(H.name()), fname);
  }
}

template <class SourceType> void read_mrtrix_header(Header &H, SourceType &kv) {
  std::string dtype, layout;
  std::vector<uint64_t> dim;
  std::vector<default_type> vox, scaling;
  std::vector<std::vector<default_type>> transform;

  std::string key, value;
  while (next_keyvalue(kv, key, value)) {
    const std::string lkey = lowercase(key);
    if (lkey == "dim")
      dim = parse_ints<uint64_t>(value);
    else if (lkey == "vox")
      vox = parse_floats(value);
    else if (lkey == "layout")
      layout = value;
    else if (lkey == "datatype")
      dtype = value;
    else if (lkey == "scaling")
      scaling = parse_floats(value);
    else if (lkey == "transform")
      transform.push_back(parse_floats(value));
    else if (!key.empty() && !value.empty())
      add_line(H.keyval()[key], value); // Preserve capitalization if not a compulsory key
  }

  if (dim.empty())
    throw Exception("missing \"dim\" specification for MRtrix image \"" + H.name() + "\"");
  H.ndim() = dim.size();
  for (size_t n = 0; n < dim.size(); n++) {
    if (dim[n] < 1)
      throw Exception("invalid dimensions for MRtrix image \"" + H.name() + "\"");
    H.size(n) = dim[n];
  }

  if (vox.empty())
    throw Exception("missing \"vox\" specification for MRtrix image \"" + H.name() + "\"");
  if (vox.size() < std::min(size_t(3), dim.size()))
    throw Exception("too few entries in \"vox\" specification for MRtrix image \"" + H.name() + "\"");
  for (size_t n = 0; n < std::min<size_t>(vox.size(), H.ndim()); n++) {
    if (vox[n] < 0.0)
      throw Exception("invalid voxel size for MRtrix image \"" + H.name() + "\"");
    H.spacing(n) = vox[n];
  }

  if (dtype.empty())
    throw Exception("missing \"datatype\" specification for MRtrix image \"" + H.name() + "\"");
  H.datatype() = DataType::parse(dtype);

  if (layout.empty())
    throw Exception("missing \"layout\" specification for MRtrix image \"" + H.name() + "\"");
  auto ax = parse_axes(H.ndim(), layout);
  for (size_t i = 0; i < ax.size(); ++i)
    H.stride(i) = ax[i];

  if (!transform.empty()) {

    auto check_transform = [&transform]() {
      if (transform.size() < 3)
        return false;
      for (auto row : transform)
        if (row.size() != 4)
          return false;
      return true;
    };
    if (!check_transform())
      throw Exception("invalid \"transform\" specification for MRtrix image \"" + H.name() + "\"");

    for (int row = 0; row < 3; ++row)
      for (int col = 0; col < 4; ++col)
        H.transform()(row, col) = transform[row][col];
  }

  if (!scaling.empty()) {
    if (scaling.size() != 2)
      throw Exception("invalid \"scaling\" specification for MRtrix image \"" + H.name() + "\"");
    H.set_intensity_scaling(scaling[1], scaling[0]);
  }
}
template void read_mrtrix_header<File::KeyValue::Reader>(Header &H, File::KeyValue::Reader &kv);
template void read_mrtrix_header<File::GZ>(Header &H, File::GZ &kv);

template <class StreamType> void write_mrtrix_header(const Header &H, StreamType &out) {
  out << "dim: " << H.size(0);
  for (size_t n = 1; n < H.ndim(); ++n)
    out << "," << H.size(n);

  out << "\nvox: " << H.spacing(0);
  for (size_t n = 1; n < H.ndim(); ++n)
    out << "," << H.spacing(n);

  auto stride = Stride::get(H);
  Stride::symbolise(stride);

  out << "\nlayout: " << (stride[0] > 0 ? "+" : "-") << MR::abs(stride[0]) - 1;
  for (size_t n = 1; n < H.ndim(); ++n)
    out << "," << (stride[n] > 0 ? "+" : "-") << MR::abs(stride[n]) - 1;

  DataType dt = H.datatype();
  dt.set_byte_order_native();
  out << "\ndatatype: " << dt.specifier();

  Eigen::IOFormat fmt(Eigen::FullPrecision, Eigen::DontAlignCols, ", ", "\ntransform: ", "", "", "\ntransform: ", "");
  out << H.transform().matrix().topLeftCorner(3, 4).format(fmt);

  if (H.intensity_offset() != 0.0 || H.intensity_scale() != 1.0)
    out << "\nscaling: " << H.intensity_offset() << "," << H.intensity_scale();

  for (const auto &i : H.keyval())
    for (const auto &line : split_lines(i.second))
      out << "\n" << i.first << ": " << line;

  out << "\n";
}
template void write_mrtrix_header<File::OFStream>(const Header &H, File::OFStream &out);
template void write_mrtrix_header<std::stringstream>(const Header &H, std::stringstream &out);

} // namespace MR::Formats
