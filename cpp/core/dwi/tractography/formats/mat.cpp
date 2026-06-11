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
#include <cstring>

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

//! \brief append a Level-5 element tag + payload (padded to 8 bytes) to a buffer.
void append_element(std::vector<std::byte> &out, uint32_t type, const std::byte *data, size_t length) {
  std::array<std::byte, 8> tag{};
  Raw::store_LE<uint32_t>(type, tag.data(), 0);
  Raw::store_LE<uint32_t>(static_cast<uint32_t>(length), tag.data(), 1);
  out.insert(out.end(), tag.begin(), tag.end());
  out.insert(out.end(), data, data + length);
  const size_t padding = (8 - (length % 8)) % 8;
  out.insert(out.end(), padding, std::byte{0});
}

//! \brief map a numeric Class to the MATLAB array-class byte for the flags subelement.
uint8_t array_class_byte(Class cls) {
  switch (cls) {
  case Class::Int8:
    return mxINT8_CLASS;
  case Class::UInt8:
    return mxUINT8_CLASS;
  case Class::Int16:
    return mxINT16_CLASS;
  case Class::UInt16:
    return mxUINT16_CLASS;
  case Class::Int32:
    return mxINT32_CLASS;
  case Class::UInt32:
    return mxUINT32_CLASS;
  case Class::Single:
    return mxSINGLE_CLASS;
  case Class::Double:
    return mxDOUBLE_CLASS;
  case Class::Int64:
    return mxINT64_CLASS;
  case Class::UInt64:
    return mxUINT64_CLASS;
  case Class::Char:
    return mxCHAR_CLASS;
  default:
    return mxDOUBLE_CLASS;
  }
}

//! \brief serialise one named Array as a complete miMATRIX element.
void append_matrix(std::vector<std::byte> &out, std::string_view name, const Array &array) {
  // Build the miMATRIX body (the four subelements) into a scratch buffer first,
  //   so its total byte count can be written into the outer element tag.
  std::vector<std::byte> body;

  // Subelement 1: array flags (8 bytes of payload).
  std::array<std::byte, 8> flags{};
  flags[0] = static_cast<std::byte>(array_class_byte(array.cls));
  append_element(body, static_cast<uint32_t>(Class::UInt8), flags.data(), flags.size());

  // Subelement 2: dimensions (int32[2]).
  std::array<std::byte, 8> dims{};
  Raw::store_LE<int32_t>(static_cast<int32_t>(array.rows), dims.data(), 0);
  Raw::store_LE<int32_t>(static_cast<int32_t>(array.cols), dims.data(), 1);
  append_element(body, static_cast<uint32_t>(Class::Int32), dims.data(), dims.size());

  // Subelement 3: array name (int8 characters).
  append_element(
      body, static_cast<uint32_t>(Class::Int8), reinterpret_cast<const std::byte *>(name.data()), name.size());

  // Subelement 4: the numeric / character payload.
  append_element(body, static_cast<uint32_t>(array.cls), array.data.data(), array.data.size());

  // Outer miMATRIX tag wrapping the assembled body.
  std::array<std::byte, 8> tag{};
  Raw::store_LE<uint32_t>(miMATRIX, tag.data(), 0);
  Raw::store_LE<uint32_t>(static_cast<uint32_t>(body.size()), tag.data(), 1);
  out.insert(out.end(), tag.begin(), tag.end());
  out.insert(out.end(), body.begin(), body.end());
  // miMATRIX bodies are already a multiple of 8 bytes (every subelement is
  //   padded), so no further outer padding is required.
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

File read(const std::byte *bytes, size_t size) {
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

std::vector<std::byte> write(const File &file) {
  std::vector<std::byte> out;

  // 128-byte header: descriptive text, subsystem-offset zeros, version 0x0100,
  //   little-endian indicator "IM".
  const std::string description = "MATLAB 5.0 MAT-file, written by MRtrix3";
  std::vector<std::byte> header(128, std::byte{0});
  std::memcpy(header.data(), description.data(), std::min<size_t>(description.size(), 116));
  Raw::store_LE<uint16_t>(0x0100, header.data() + 124, 0);
  header[126] = static_cast<std::byte>('I');
  header[127] = static_cast<std::byte>('M');
  out.insert(out.end(), header.begin(), header.end());

  for (const auto &member : file.all())
    append_matrix(out, member.first, member.second);

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
