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

#include "dwi/tractography/formats/mat.h"

#include <array>
#include <optional>

#include "exception.h"
#include "raw.h"

namespace MR::DWI::Tractography::Formats::Mat {

namespace {

// MATLAB Level-5 data-type tag codes used when *parsing* element tags. These
//   overlap with the public Mat::Class enumerators for the numeric/char types,
//   but also include the container/aggregate codes that only appear as element
//   tags (not as a stored array element type).
constexpr uint32_t miMATRIX = 14;
constexpr uint32_t miCOMPRESSED = 15;

// MATLAB array classes (the byte inside the "array flags" subelement that names
//   the high-level class of an miMATRIX, distinct from the element data type).
constexpr uint8_t mxCHAR_CLASS = 4;
constexpr uint8_t mxDOUBLE_CLASS = 6;
constexpr uint8_t mxSINGLE_CLASS = 7;
constexpr uint8_t mxINT8_CLASS = 8;
constexpr uint8_t mxUINT8_CLASS = 9;
constexpr uint8_t mxINT16_CLASS = 10;
constexpr uint8_t mxUINT16_CLASS = 11;
constexpr uint8_t mxINT32_CLASS = 12;
constexpr uint8_t mxUINT32_CLASS = 13;
constexpr uint8_t mxINT64_CLASS = 14;
constexpr uint8_t mxUINT64_CLASS = 15;

//! \brief sequential little-endian reader over the decompressed ".mat" bytes.
class Cursor {
public:
  Cursor(const std::byte *base, size_t size) : base(base), size(size), pos(0) {}

  size_t remaining() const { return size - pos; }
  size_t offset() const { return pos; }
  bool at_end() const { return pos >= size; }

  void require(size_t n) const {
    if (pos + n > size)
      throw Exception("MATLAB \".mat\" container truncated (needed " + str(n) + " bytes at offset " + str(pos) + ")");
  }

  template <typename T> T read_scalar() {
    require(sizeof(T));
    const T value = Raw::fetch_LE<T>(base + pos);
    pos += sizeof(T);
    return value;
  }

  const std::byte *read_block(size_t n) {
    require(n);
    const std::byte *const p = base + pos;
    pos += n;
    return p;
  }

  void skip(size_t n) {
    require(n);
    pos += n;
  }

  //! \brief advance to the next 8-byte boundary (MATLAB element padding).
  void align8() {
    const size_t rem = pos % 8;
    if (rem != 0)
      skip(8 - rem);
  }

private:
  const std::byte *base;
  size_t size;
  size_t pos;
};

//! \brief A single Level-5 data element: its type tag and payload bytes.
struct Element {
  uint32_t type;
  const std::byte *data;
  size_t length;
};

//! \brief read one Level-5 data element, honouring the small-element format.
/*! In the small-element-data format the byte count is packed into the high
 * 16 bits of the first 32-bit word and up to 4 payload bytes immediately
 * follow; otherwise the tag is two 32-bit words (type, byte-count) followed by
 * the payload padded to an 8-byte boundary. */
Element read_element(Cursor &cursor) {
  cursor.require(4);
  const uint32_t first = cursor.read_scalar<uint32_t>();
  const uint16_t small_bytes = static_cast<uint16_t>(first >> 16);
  Element element{};
  if (small_bytes != 0) {
    // Small-element format: type in low 16 bits, byte count in high 16 bits.
    element.type = first & 0x0000FFFFU;
    element.length = small_bytes;
    element.data = cursor.read_block(4); // payload occupies the 4 bytes after the tag word
  } else {
    element.type = first;
    element.length = cursor.read_scalar<uint32_t>();
    element.data = cursor.read_block(element.length);
    cursor.align8();
  }
  return element;
}

//! \brief map a stored element data-type tag to the public numeric Class.
Class element_class(uint32_t type) {
  switch (type) {
  case 1:
    return Class::Int8;
  case 2:
    return Class::UInt8;
  case 3:
    return Class::Int16;
  case 4:
    return Class::UInt16;
  case 5:
    return Class::Int32;
  case 6:
    return Class::UInt32;
  case 7:
    return Class::Single;
  case 9:
    return Class::Double;
  case 12:
    return Class::Int64;
  case 13:
    return Class::UInt64;
  case 16: // miUTF8
  case 17: // miUTF16
  case 18: // miUTF32
    return Class::Char;
  default:
    throw Exception("unsupported MATLAB element data type code " + str(type) + " in \".mat\" container");
  }
}

//! \brief parse the body of one miMATRIX element into a named Array.
/*! Returns false (skipping the member) for array classes that ".tt" never uses
 * but that are still structurally valid (cell/struct/sparse), so an unrelated
 * member does not abort the parse. */
bool parse_matrix(const Element &matrix, std::string &name, Array &array) {
  Cursor cursor(matrix.data, matrix.length);

  // Subelement 1: array flags (8 payload bytes: class byte + flags, then undefined).
  const Element flags = read_element(cursor);
  if (flags.length < 8)
    throw Exception("MATLAB miMATRIX array-flags subelement is malformed");
  const uint8_t mx_class = static_cast<uint8_t>(flags.data[0]);

  // Subelement 2: dimensions (int32 array).
  const Element dims = read_element(cursor);
  const size_t ndims = dims.length / sizeof(int32_t);
  std::vector<int32_t> dimensions(ndims);
  for (size_t i = 0; i != ndims; ++i)
    dimensions[i] = Raw::fetch_LE<int32_t>(dims.data, i);

  // Subelement 3: array name (int8 character array).
  const Element name_element = read_element(cursor);
  name.assign(reinterpret_cast<const char *>(name_element.data), name_element.length);

  // Aggregate classes carry sub-structure rather than a flat numeric payload;
  //   ".tt" never uses these, so report the member as not-of-interest.
  if (mx_class != mxCHAR_CLASS &&   //
      mx_class != mxDOUBLE_CLASS && //
      mx_class != mxSINGLE_CLASS && //
      mx_class != mxINT8_CLASS &&   //
      mx_class != mxUINT8_CLASS &&  //
      mx_class != mxINT16_CLASS &&  //
      mx_class != mxUINT16_CLASS && //
      mx_class != mxINT32_CLASS &&  //
      mx_class != mxUINT32_CLASS && //
      mx_class != mxINT64_CLASS &&  //
      mx_class != mxUINT64_CLASS) { //
    return false;
  }

  // Subelement 4: the real part (the numeric / character payload).
  const Element payload = read_element(cursor);
  array.cls = element_class(payload.type);
  array.rows = ndims > 0 ? static_cast<size_t>(dimensions[0]) : 0;
  array.cols = ndims > 1 ? static_cast<size_t>(dimensions[1]) : (array.rows > 0 ? 1 : 0);
  for (size_t i = 2; i < ndims; ++i)
    array.cols *= static_cast<size_t>(dimensions[i]);

  // Character payloads stored as UTF-16/UTF-32 are narrowed to a byte string;
  //   for the numeric payloads the on-disk bytes are copied verbatim.
  array.data.assign(payload.data, payload.data + payload.length);
  return true;
}

//! \brief the Level-4 MOPT type word for a numeric / character Array class.
/*! Little-endian (M=0), full matrix (T=0); the precision code P is the second
 * digit. The Level-4 precision set is a subset of Class: 8-/16-/32-bit characters
 * and signed 8-bit collapse onto the unsigned-8 precision, matching how DSI Studio
 * stores text members and the int8 track-delta buffer. */
uint32_t level4_mopt_for_class(Class cls) {
  switch (cls) {
  case Class::Double:
    return 0; // P=0, f64
  case Class::Single:
    return 10; // P=1, f32
  case Class::Int32:
  case Class::UInt32:
    return 20; // P=2, i32
  case Class::Int16:
    return 30; // P=3, i16
  case Class::UInt16:
    return 40; // P=4, u16
  case Class::Int8:
  case Class::UInt8:
  case Class::Char:
    return 50; // P=5, u8
  default:
    return 0;
  }
}

//! \brief append \a value to \a out as a little-endian uint32.
void append_u32(std::vector<std::byte> &out, uint32_t value) {
  std::array<std::byte, sizeof(uint32_t)> bytes{};
  Raw::store_LE<uint32_t>(value, bytes.data(), 0);
  out.insert(out.end(), bytes.begin(), bytes.end());
}

//! \brief serialise one named Array as a Level-4 matrix record.
void append_level4_matrix(std::vector<std::byte> &out, std::string_view name, const Array &array) {
  append_u32(out, level4_mopt_for_class(array.cls));
  append_u32(out, static_cast<uint32_t>(array.rows));
  append_u32(out, static_cast<uint32_t>(array.cols));
  append_u32(out, 0U);                                     // imagf: real data only
  append_u32(out, static_cast<uint32_t>(name.size() + 1)); // namelen includes the NUL terminator
  out.insert(out.end(),
             reinterpret_cast<const std::byte *>(name.data()),
             reinterpret_cast<const std::byte *>(name.data()) + name.size());
  out.push_back(std::byte{0});
  out.insert(out.end(), array.data.begin(), array.data.end());
}

} // namespace

/* ************************************************************************ */
/*                   MATLAB Level-4 ("v4") container                      */
/* ************************************************************************ */

// DSI Studio writes ".tt" as a gzipped MATLAB Level-4 file, not Level-5. A Level-4
//   file has no 128-byte header: it is a bare sequence of matrix records, each
//     type:uint32 | mrows:uint32 | ncols:uint32 | imagf:uint32 | namelen:uint32
//     | name[namelen] (NUL-terminated) | real data | [imaginary data]
//   with column-major data. The "type" is an MOPT code = M*1000 + O*100 + P*10 + T:
//   M (machine): 0 = IEEE little-endian (the only one supported here); O: always 0;
//   P (precision): 0=f64 1=f32 2=i32 3=i16 4=u16 5=u8; T: 0=full 1=text 2=sparse.
//   DSI Studio stores text members (report, parameter_id) as u8 full matrices.

namespace {

//! \brief decode a Level-4 MOPT word into an element class + width, or nullopt.
/*! Returns nullopt for any word that is not a well-formed little-endian Level-4
 * type — which is also how read() distinguishes a Level-4 file (first word is a
 * valid MOPT) from a Level-5 file (first word is ASCII header text). */
std::optional<std::pair<Class, size_t>> decode_level4_type(uint32_t mopt) {
  const uint32_t machine = mopt / 1000;
  const uint32_t order = (mopt / 100) % 10;
  const uint32_t precision = (mopt / 10) % 10;
  const uint32_t matrix_type = mopt % 10;
  if (machine != 0 || order != 0 || matrix_type > 2)
    return std::nullopt;
  switch (precision) {
  case 0:
    return std::make_pair(Class::Double, size_t(8));
  case 1:
    return std::make_pair(Class::Single, size_t(4));
  case 2:
    return std::make_pair(Class::Int32, size_t(4));
  case 3:
    return std::make_pair(Class::Int16, size_t(2));
  case 4:
    return std::make_pair(Class::UInt16, size_t(2));
  case 5:
    return std::make_pair(Class::UInt8, size_t(1));
  default:
    return std::nullopt;
  }
}

//! \brief parse a bare Level-4 record stream into the same File abstraction.
File read_level4(const std::byte *bytes, size_t size) {
  File file;
  Cursor cursor(bytes, size);
  // Each record needs five uint32 header fields; a shorter trailing remnant is
  //   treated as end-of-file padding rather than an error.
  while (cursor.remaining() >= 5 * sizeof(uint32_t)) {
    const uint32_t mopt = cursor.read_scalar<uint32_t>();
    const uint32_t mrows = cursor.read_scalar<uint32_t>();
    const uint32_t ncols = cursor.read_scalar<uint32_t>();
    const uint32_t imagf = cursor.read_scalar<uint32_t>();
    const uint32_t namelen = cursor.read_scalar<uint32_t>();

    const std::optional<std::pair<Class, size_t>> element = decode_level4_type(mopt);
    if (!element.has_value())
      throw Exception("unsupported MATLAB Level-4 matrix type " + str(mopt) + " at offset " +
                      str(cursor.offset() - 5 * sizeof(uint32_t)));
    if (namelen == 0)
      throw Exception("malformed MATLAB Level-4 \".mat\": zero-length member name");

    const std::byte *const name_bytes = cursor.read_block(namelen);
    std::string name(reinterpret_cast<const char *>(name_bytes), namelen);
    const size_t terminator = name.find('\0');
    if (terminator != std::string::npos)
      name.resize(terminator);

    Array array;
    array.cls = element->first;
    array.rows = mrows;
    array.cols = ncols;
    const size_t data_bytes = static_cast<size_t>(mrows) * static_cast<size_t>(ncols) * element->second;
    const std::byte *const data = cursor.read_block(data_bytes);
    array.data.assign(data, data + data_bytes);
    // ".tt" data is always real; skip any imaginary half rather than store it.
    if (imagf != 0)
      cursor.skip(data_bytes);

    file.add(name, std::move(array));
  }
  return file;
}

} // namespace

size_t class_size(Class cls) {
  switch (cls) {
  case Class::Int8:
  case Class::UInt8:
  case Class::Char:
    return 1;
  case Class::Int16:
  case Class::UInt16:
    return 2;
  case Class::Int32:
  case Class::UInt32:
  case Class::Single:
    return 4;
  case Class::Int64:
  case Class::UInt64:
  case Class::Double:
    return 8;
  default:
    return 1;
  }
}

template <typename T> std::vector<T> Array::as() const {
  if (sizeof(T) != class_size(cls))
    throw Exception("MATLAB array element-width mismatch on reinterpretation");
  const size_t n = data.size() / sizeof(T);
  std::vector<T> result(n);
  for (size_t i = 0; i != n; ++i)
    result[i] = Raw::fetch_LE<T>(data.data(), i);
  return result;
}

std::string Array::as_string() const {
  // ".tt" character members are plain ASCII / UTF-8; narrow any wider encoding
  //   by dropping the high-order zero bytes of each code unit.
  const size_t unit = class_size(cls);
  std::string result;
  result.reserve(data.size() / unit);
  for (size_t i = 0; i + unit <= data.size(); i += unit)
    result += static_cast<char>(data[i]);
  return result;
}

const Array *File::find(std::string_view name) const {
  for (const auto &member : members) {
    if (member.first == name)
      return &member.second;
  }
  return nullptr;
}

namespace {

File read_level5(const std::byte *bytes, size_t size) {
  if (size < 128)
    throw Exception("MATLAB \".mat\" container too small to hold a Level-5 header");

  // The 128-byte header: a 116-byte text description, 8 bytes subsystem offset,
  //   then a 2-byte version and a 2-byte endian-indicator ("MI"/"IM").
  const char endian0 = static_cast<char>(bytes[126]);
  const char endian1 = static_cast<char>(bytes[127]);
  if (!((endian0 == 'I' && endian1 == 'M') || (endian0 == 'M' && endian1 == 'I')))
    throw Exception("file is not a valid MATLAB Level-5 \".mat\" container (bad endian indicator)");
  if (endian0 == 'M' && endian1 == 'I')
    throw Exception("big-endian MATLAB \".mat\" containers are not supported");

  File file;
  Cursor cursor(bytes, size);
  cursor.skip(128);

  while (!cursor.at_end()) {
    // A trailing run of padding zeros shorter than a tag is tolerated.
    if (cursor.remaining() < 8)
      break;
    const Element element = read_element(cursor);
    if (element.type == miCOMPRESSED)
      throw Exception("zlib-compressed elements inside a MATLAB \".mat\" container are not supported"
                      " (the \".tt\" container itself is gzip-compressed, not its members)");
    if (element.type != miMATRIX)
      continue; // skip any non-array top-level element
    std::string name;
    Array array;
    if (parse_matrix(element, name, array))
      file.add(name, std::move(array));
  }

  return file;
}

} // namespace

File read(const std::byte *bytes, size_t size) {
  // Distinguish the container generation. A Level-4 file (as DSI Studio writes for
  //   ".tt") begins directly with a matrix record whose first word is a small MOPT
  //   type code; a Level-5 file begins with a 128-byte ASCII "MATLAB 5.0 ..." header
  //   whose first word is therefore never a valid MOPT.
  if (size >= sizeof(uint32_t) && decode_level4_type(Raw::fetch_LE<uint32_t>(bytes)).has_value())
    return read_level4(bytes, size);
  return read_level5(bytes, size);
}

std::vector<std::byte> write(const File &file) {
  // Emit the MATLAB Level-4 ("v4") layout that DSI Studio reads and writes for
  //   ".tt": a bare, header-less sequence of little-endian matrix records. (MRtrix
  //   previously wrote Level-5, which DSI Studio does not produce for ".tt".)
  std::vector<std::byte> out;
  for (const auto &member : file.all())
    append_level4_matrix(out, member.first, member.second);
  return out;
}

// Explicit instantiations of Array::as<> for the element widths ".tt" requires.
template std::vector<int8_t> Array::as<int8_t>() const;
template std::vector<uint8_t> Array::as<uint8_t>() const;
template std::vector<int16_t> Array::as<int16_t>() const;
template std::vector<uint16_t> Array::as<uint16_t>() const;
template std::vector<int32_t> Array::as<int32_t>() const;
template std::vector<uint32_t> Array::as<uint32_t>() const;
template std::vector<float> Array::as<float>() const;
template std::vector<double> Array::as<double>() const;
template std::vector<int64_t> Array::as<int64_t>() const;
template std::vector<uint64_t> Array::as<uint64_t>() const;

} // namespace MR::DWI::Tractography::Formats::Mat
