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
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace MR::DWI::Tractography::Formats::Mat {

//! \brief The numeric element classes a Level-5 MATLAB array may carry.
/*! Only the small subset required by the DSI Studio ".tt" format is modelled
 * (numeric vectors/matrices plus character strings); the full MATLAB type
 * system (cell/struct/sparse/complex) is deliberately out of scope. The
 * enumerator values are the on-disk MATLAB "miXXX" data-type codes so that they
 * can be written verbatim into an element tag. */
enum class Class : uint32_t {
  Int8 = 1,    //!< miINT8
  UInt8 = 2,   //!< miUINT8
  Int16 = 3,   //!< miINT16
  UInt16 = 4,  //!< miUINT16
  Int32 = 5,   //!< miINT32
  UInt32 = 6,  //!< miUINT32
  Single = 7,  //!< miSINGLE
  Double = 9,  //!< miDOUBLE
  Int64 = 12,  //!< miINT64
  UInt64 = 13, //!< miUINT64
  Char = 16    //!< miUTF8 / character data
};

//! \brief size in bytes of one element of the given numeric class.
size_t class_size(Class cls);

//! \brief One named array parsed from (or to be written to) a Level-5 ".mat".
/*! The payload is held as raw little-endian bytes alongside its element class
 * and 2-D shape; the caller reinterprets the bytes against \c cls. Storage is
 * column-major (the MATLAB convention), matching how the values are laid out on
 * disk. For the ".tt" use case every member is either a 1×N numeric row or a
 * character string, so the 2-D (rows, cols) shape suffices. */
struct Array {
  Class cls{Class::Double};
  size_t rows{0};
  size_t cols{0};
  std::vector<std::byte> data; //!< column-major little-endian element bytes

  //! \brief number of elements (rows * cols).
  size_t size() const { return rows * cols; }

  //! \brief reinterpret the payload as a vector of T (T must match the on-disk class width).
  /*! No type conversion is performed: T's width must equal class_size(cls).
   * Little-endian on-disk order is assumed (the ".tt" platform convention). */
  template <typename T> std::vector<T> as() const;

  //! \brief decode a character array (Class::Char) to a std::string.
  std::string as_string() const;
};

//! \brief A parsed Level-5 ".mat" file: its named top-level arrays, in order.
/*! Insertion order is preserved (a std::vector of name/array pairs) so that a
 * round-trip re-emits members in the order MATLAB / DSI Studio expects. */
class File {
public:
  //! \brief append a named array (write path).
  void add(std::string_view name, Array &&array) { members.emplace_back(std::string(name), std::move(array)); }

  //! \brief retrieve a member by name, or nullptr if absent (read path).
  const Array *find(std::string_view name) const;

  const std::vector<std::pair<std::string, Array>> &all() const { return members; }

private:
  std::vector<std::pair<std::string, Array>> members;
};

//! \brief parse a Level-5 ".mat" container from an in-memory byte buffer.
/*! \a bytes is the already-decompressed ".mat" content (the ".tt" handler
 * gzip-inflates the file first). Only the numeric/character members needed by
 * ".tt" are retained; cell/struct/sparse/complex members raise a clean error.
 * A hierarchical MR::Exception is thrown on any structural inconsistency. */
File read(const std::byte *bytes, size_t size);

//! \brief serialise a Level-5 ".mat" container to a byte buffer.
/*! Produces the 128-byte header followed by one miMATRIX element per member.
 * The result is suitable for gzip-compression into a ".tt" file. */
std::vector<std::byte> write(const File &file);

} // namespace MR::DWI::Tractography::Formats::Mat
