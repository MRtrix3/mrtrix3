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

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <istream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "raw.h"

#include "dwi/tractography/formats/write_buffer.h"
#include "dwi/tractography/properties.h"

namespace MR::File {
class MMap;
}

namespace MR::DWI::Tractography::Formats::VTKUtils {

//! \brief Maximum length of the VTK header line (part 2 of the legacy spec).
constexpr size_t header_max_length = 256;

//! \brief The on-disk encoding of a VTK / VTX payload.
enum class Encoding { ASCII, Binary };

//! \brief The element datatype of a VTK / VTX POINTS coordinate array.
enum class PointDataType { Float32, Float64 };

//! \brief Append the entire contents of \a source to the open stream \a dest.
/*! Used when assembling a VTK-derived output from its concatenated temporary
 * POINTS / connectivity files. */
void append_file(std::ofstream &dest, const std::filesystem::path &source);

//! \brief Append \a size bytes of \a data to the file at \a path (binary append).
/*! The filesystem-flush primitive behind the temporary-file WriteBuffers used
 * by the VTK-derived writers. */
void append_bytes(const std::filesystem::path &path, const std::byte *data, size_t size);

//! \brief Build the leading lines of a VTK-derived header, up to the DATASET line.
/*! Emits the version line, the (command-history-derived) description line bounded
 * to \a header_max_length, the ASCII/BINARY encoding line, and the
 * "DATASET <dataset_type>" line for \a dataset_type (e.g. "POLYDATA" or
 * "STREAMLINES"). */
std::string dataset_header(Encoding encoding, std::string_view dataset_type);

//! \brief Parse the VTK preamble (version + encoding + free-text comments).
/*! Reads the version/identifier line, then consumes lines until the ASCII or
 * BINARY format keyword is found, pushing any intervening free-text description
 * lines onto \a properties.comments. Leaves \a in positioned at the first
 * dataset-structure line. Throws a hierarchical Exception (rooted at \a path) on
 * a missing or malformed preamble. */
Encoding parse_preamble(std::istream &in, const std::filesystem::path &path, Properties &properties);

//! \brief Streaming accessor over a VTK-derived POINTS coordinate block.
/*! Encapsulates the two representations a POINTS block may take: an ASCII array
 * parsed up-front into RAM, or a binary (big-endian) array accessed through a
 * memory-map. get_point() fetches the i-th 3-vector irrespective of encoding,
 * converting to the requested processing precision \a ValueType. This is the
 * shared POINTS-handling code reused by both the ".vtk" and ".vtx" readers.
 *
 * Explicitly instantiated for float and double in formats/vtk_utils.cpp. */
template <class ValueType> class PointReader {
public:
  //! \brief Construct an ASCII POINTS accessor from RAM-resident coordinates.
  PointReader(std::vector<ValueType> &&ascii_coordinates, size_t num_points)
      : encoding(Encoding::ASCII),
        point_type(PointDataType::Float32),
        ascii(std::move(ascii_coordinates)),
        binary_offset(0),
        n_points(num_points) {}

  //! \brief Construct a binary POINTS accessor over a memory-map.
  /*! \a map is the memory-map of the whole file; \a offset is the byte offset of
   * the first coordinate; \a type is the on-disk element datatype. */
  PointReader(std::shared_ptr<File::MMap> map, int64_t offset, PointDataType type, size_t num_points)
      : encoding(Encoding::Binary),
        point_type(type),
        mmap(std::move(map)),
        binary_offset(offset),
        n_points(num_points) {}

  //! \brief The number of vertices in the POINTS block.
  size_t size() const { return n_points; }

  //! \brief Fetch the \a i-th point coordinates in the processing precision.
  Eigen::Matrix<ValueType, 3, 1> get_point(size_t i) const;

private:
  Encoding encoding;
  PointDataType point_type;
  std::shared_ptr<File::MMap> mmap;
  std::vector<ValueType> ascii;
  int64_t binary_offset;
  size_t n_points;
};

//! \brief Append one streamline vertex to a POINTS WriteBuffer.
/*! Shared big-endian / ASCII point-encoding used by the VTK-derived writers.
 * Binary coordinates are stored as big-endian float32; ASCII coordinates are
 * printed with enough significant digits to round-trip float32 losslessly. */
template <class ValueType>
void write_point(WriteBuffer &buffer, Encoding encoding, const Eigen::Matrix<ValueType, 3, 1> &p);

} // namespace MR::DWI::Tractography::Formats::VTKUtils
