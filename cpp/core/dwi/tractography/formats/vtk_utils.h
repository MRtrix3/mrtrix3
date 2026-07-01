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

#include "datatype.h"
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

/* ************************************************************************ */
/*               VTK dataset-attribute (sidecar) data types               */
/* ************************************************************************ */

//! \brief Map a legacy-VTK \c dataType token to an MR::DataType (§ Stage 13).
/*! Implements the spec's "dataType is one of bit, unsigned_char, char,
 * unsigned_short, short, unsigned_int, int, unsigned_long, long, float, double"
 * (vtk.md). The returned DataType carries native byte-order for the multi-byte
 * types so that a sidecar field round-trips in its on-disk precision (D7). The
 * "bit" token maps to UInt8 (the in-memory sidecar bit representation, §2.2).
 * Throws if the token is not a recognised / sidecar-supportable VTK type. */
DataType datatype_from_vtk_token(std::string_view token, const std::filesystem::path &path);

//! \brief The legacy-VTK \c dataType token for an MR::DataType (§ Stage 13).
/*! The inverse of datatype_from_vtk_token(): emits the VTK keyword written into
 * the SCALARS / FIELD attribute header for a sidecar field of the given native
 * datatype. Throws if the datatype has no legacy-VTK token. */
std::string vtk_token_from_datatype(DataType dtype);

//! \brief Append one M-component sidecar tuple to a dataset-attribute WriteBuffer.
/*! Encodes the \a M values pointed to by \a values (native element type \c T)
 * for one tuple (one streamline for dps / CELL_DATA, one vertex for dpv /
 * POINT_DATA), terminated by a newline in ASCII. BINARY values are stored
 * big-endian (the legacy-VTK convention); ASCII values are printed with enough
 * significant digits to round-trip the native type. Integer types are printed
 * as integers (char/unsigned_char promoted so they are not emitted as
 * characters). Shared by the ".vtk"/".vtx" writers. */
template <class T> void write_sidecar_tuple(WriteBuffer &buffer, Encoding encoding, const T *values, size_t M);

//! \brief Fetch one M-component sidecar tuple of native type \c T from a memory-map.
/*! Reads \a M big-endian values of type \c T starting at byte \a offset within
 * the map \a base into \a out. The binary counterpart of the per-tuple read used
 * while streaming a POINT_DATA / CELL_DATA block (§ Stage 13). */
template <class T> void fetch_sidecar_tuple_BE(const std::byte *base, int64_t offset, T *out, size_t M);

} // namespace MR::DWI::Tractography::Formats::VTKUtils
