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

#include "dwi/tractography/formats/vtx.h"

#include <array>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>

#include "app.h"
#include "exception.h"
#include "file/config.h"
#include "file/mmap.h"
#include "file/ofstream.h"
#include "file/temp.h"
#include "raw.h"

namespace MR::DWI::Tractography {

namespace {

//! \brief Maximum length of the VTK header line (part 2 of the spec).
constexpr size_t vtk_header_max_length = 256;

//! \brief Append the contents of \a source to the already-open stream \a dest.
void append_file(std::ofstream &dest, const std::filesystem::path &source) {
  std::ifstream in(source, std::ios::binary);
  if (!in)
    throw Exception("unable to open temporary file \"" + source.string() + "\" for VTX assembly");
  dest << in.rdbuf();
  if (!dest.good() && !in.eof())
    throw Exception("error concatenating temporary file \"" + source.string() + "\" into VTX output");
}

//! \brief Append \a size bytes of \a data to the file at \a path (binary append).
void append_bytes(const std::filesystem::path &path, const std::byte *data, size_t size) {
  std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::app);
  if (!out)
    throw Exception("unable to open temporary file \"" + path.string() + "\" for writing");
  out.write(reinterpret_cast<const char *>(data), size);
  if (!out.good())
    throw Exception("error writing to temporary file \"" + path.string() + "\"");
}

} // namespace

/* ************************************************************************ */
/*                          VTXReader<ValueType>                           */
/* ************************************************************************ */

// NOTE: the ".vtx" read backend is implemented in the following enumerated
//   step of this stage; the writer (below) is what this step delivers.
template <class ValueType> VTXReader<ValueType>::VTXReader(const std::filesystem::path &path, Properties &) {
  throw Exception("reading of VTX file \"" + path.string() + "\" not yet supported");
}

template <class ValueType> VTXReader<ValueType>::~VTXReader() = default;

template <class ValueType> Eigen::Matrix<ValueType, 3, 1> VTXReader<ValueType>::get_point(size_t) const { return {}; }

template <class ValueType> int64_t VTXReader<ValueType>::get_offset_end(size_t) const { return 0; }

template <class ValueType> bool VTXReader<ValueType>::operator()(Streamline<ValueType> &) { return false; }

/* ************************************************************************ */
/*                          VTXWriter<ValueType>                           */
/* ************************************************************************ */

template <class ValueType>
VTXWriter<ValueType>::VTXWriter(const std::filesystem::path &path, const Properties &properties)
    : path(path),
      points_buffer(File::Config::get_int("TrackWriterBufferSize", 16777216), 1),
      offsets_buffer(File::Config::get_int("TrackWriterBufferSize", 16777216), 1),
      num_points(0),
      num_streamlines(0) {
  if (path.extension() != ".vtx")
    throw Exception("output VTX track files must use the .vtx suffix");

  // The "-ascii" option (declared by commands that write VTK-derived formats,
  //   e.g. tckconvert) selects the ASCII encoding; the BINARY (big-endian)
  //   encoding is the default.
  encoding = App::get_options("ascii").empty() ? Encoding::Binary : Encoding::ASCII;

  (void)properties;

  // Verify the output can be created, then truncate any pre-existing content.
  App::check_overwrite(path);
  {
    File::OFStream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!out.good())
      throw Exception("unable to create VTX output file \"" + path.string() + "\"");
  }

  points_tempfile = File::create_tempfile(0, ".vtxpoints");
  offsets_tempfile = File::create_tempfile(0, ".vtxoffsets");

  const std::filesystem::path points_path = points_tempfile;
  const std::filesystem::path offsets_path = offsets_tempfile;
  points_buffer.set_flush_callback(
      [points_path](const std::byte *data, size_t size, const Formats::WriteBuffer::Counts &) {
        append_bytes(points_path, data, size);
      });
  offsets_buffer.set_flush_callback(
      [offsets_path](const std::byte *data, size_t size, const Formats::WriteBuffer::Counts &) {
        append_bytes(offsets_path, data, size);
      });
}

template <class ValueType> void VTXWriter<ValueType>::add_offset(int64_t offset_end) {
  if (encoding == Encoding::ASCII) {
    std::ostringstream stream;
    stream << offset_end << "\n";
    const std::string text = stream.str();
    offsets_buffer.add(reinterpret_cast<const std::byte *>(text.data()), text.size());
  } else {
    std::array<int64_t, 1> raw{};
    Raw::store_BE<int64_t>(offset_end, raw.data(), 0);
    offsets_buffer.add(reinterpret_cast<const std::byte *>(raw.data()), sizeof(raw));
  }
}

template <class ValueType> bool VTXWriter<ValueType>::operator()(const Streamline<ValueType> &tck) {
  if (encoding == Encoding::ASCII) {
    std::ostringstream stream;
    // Coordinates are stored as float32 (matching the binary encoding); print
    //   enough significant digits to round-trip float32 losslessly.
    stream.precision(std::numeric_limits<float>::max_digits10);
    for (const auto &pos : tck)
      stream << static_cast<float>(pos[0]) << " " << static_cast<float>(pos[1]) << " " << static_cast<float>(pos[2])
             << "\n";
    const std::string text = stream.str();
    points_buffer.add(reinterpret_cast<const std::byte *>(text.data()), text.size());
  } else {
    for (const auto &pos : tck) {
      std::array<float, 3> raw{};
      for (size_t i = 0; i != 3; ++i)
        Raw::store_BE<float>(static_cast<float>(pos[i]), raw.data(), i);
      points_buffer.add(reinterpret_cast<const std::byte *>(raw.data()), sizeof(raw));
    }
  }
  num_points += tck.size();
  ++num_streamlines;
  // OFFSETS store the END vertex index of streamline j (offsetEnd[j]); with
  //   running vertex total num_points, the last index of this streamline is
  //   num_points - 1.
  add_offset(static_cast<int64_t>(num_points) - 1);
  return true;
}

template <class ValueType> std::string VTXWriter<ValueType>::finalise_header() const {
  std::string result = "# vtk DataFile Version 3.0\n";

  // Part 2: header line, maximum 256 characters (spec part 2). Prefer the full
  //   command string; fall back to the software name and version if it would not
  //   fit within the permitted header length.
  std::string description = App::command_history_string;
  if (description.empty() || description.size() > vtk_header_max_length)
    description = std::string("MRtrix ") + App::mrtrix_version;
  if (description.size() > vtk_header_max_length)
    description.resize(vtk_header_max_length);
  // The header line is terminated by a newline and may not itself contain one.
  for (char &c : description) {
    if (c == '\n' || c == '\r')
      c = ' ';
  }
  result += description;
  result += '\n';

  result += (encoding == Encoding::ASCII) ? "ASCII\n" : "BINARY\n";
  result += "DATASET STREAMLINES\n";
  return result;
}

template <class ValueType> void VTXWriter<ValueType>::finalise() {
  points_buffer.commit();
  offsets_buffer.commit();

  File::OFStream out(path, std::ios::out | std::ios::binary | std::ios::trunc);

  out << finalise_header();
  out << "POINTS " << num_points << " float\n";
  append_file(out, points_tempfile);
  // Binary payloads must be separated from the following ASCII keyword by a
  //   newline; ASCII payloads already end each record with one.
  if (encoding == Encoding::Binary)
    out << "\n";

  // The number of OFFSETS entries equals the number of streamlines.
  out << "OFFSETS " << num_streamlines << " vtktypeint64\n";
  append_file(out, offsets_tempfile);
  if (encoding == Encoding::Binary)
    out << "\n";

  out.close();

  std::error_code ec;
  std::filesystem::remove(points_tempfile, ec);
  std::filesystem::remove(offsets_tempfile, ec);
}

template <class ValueType> VTXWriter<ValueType>::~VTXWriter() {
  try {
    finalise();
  } catch (Exception &e) {
    Exception(e, "VTX tractography file not properly finalised").display();
  }
}

/* ************************************************************************ */
/*               Explicit instantiation for float and double              */
/* ************************************************************************ */

template class VTXReader<float>;
template class VTXReader<double>;
template class VTXWriter<float>;
template class VTXWriter<double>;

namespace Formats {

bool VTX::handles(const std::filesystem::path &path) const { return path.extension() == ".vtx"; }

std::unique_ptr<ReaderInterface<float>> VTX::read_float(const std::filesystem::path &path,
                                                        Properties &properties) const {
  return std::make_unique<VTXReader<float>>(path, properties);
}

std::unique_ptr<ReaderInterface<double>> VTX::read_double(const std::filesystem::path &path,
                                                          Properties &properties) const {
  return std::make_unique<VTXReader<double>>(path, properties);
}

std::unique_ptr<WriterInterface<float>> VTX::create_float(const std::filesystem::path &path,
                                                          const Properties &properties) const {
  return std::make_unique<VTXWriter<float>>(path, properties);
}

std::unique_ptr<WriterInterface<double>> VTX::create_double(const std::filesystem::path &path,
                                                            const Properties &properties) const {
  return std::make_unique<VTXWriter<double>>(path, properties);
}

} // namespace Formats

} // namespace MR::DWI::Tractography
