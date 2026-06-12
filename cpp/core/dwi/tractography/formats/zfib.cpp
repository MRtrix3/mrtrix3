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

#include "dwi/tractography/formats/zfib.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <string>
#include <string_view>

#include "app.h"
#include "exception.h"
#include "file/ofstream.h"
#include "mrtrix.h"
#include "raw.h"

#include "dwi/tractography/compression/quantization.h"
#include "dwi/tractography/compression/ramer_douglas_peucker.h"

namespace MR::DWI::Tractography {

namespace {

//! header field values identifying the supported .zfib variant (little-endian int32).
constexpr int32_t fiber_type_mrtrix = 5; //!< FIBERTYPE_MRTRIX: scanner-space coordinates ≡ ".tck"
constexpr int32_t encoding_huffman = 0;  //!< HUFFMAN_ENCODING
constexpr int32_t transform_none = 6;    //!< NO_TRANSFORMATION

//! \brief Width of the size fields in the Huffman signal block.
/*! The reference encoder writes xEncodedSize / yEncodedSize / zEncodedSize /
 * dictSize with std::ofstream::write(sizeof(...)); on its 64-bit build these are
 * size_t (8 bytes). Centralised here as a single tunable so the choice can be
 * flipped should byte-exact interop against a reference file demand it; the
 * MRtrix-internal round-trip is self-consistent regardless. */
using EncodedSize = uint64_t;

//! \brief A bounds-checked little-endian read cursor over an in-memory .zfib file.
class ByteReader {
public:
  ByteReader(const std::byte *data, const size_t size) : data(data), size(size), position(0) {}

  template <typename T> T fetch() {
    if (position + sizeof(T) > size)
      throw Exception("malformed .zfib file: unexpected end of data");
    const T value = Raw::fetch_LE<T>(data + position);
    position += sizeof(T);
    return value;
  }

  const std::byte *take(const size_t bytes) {
    if (position + bytes > size)
      throw Exception("malformed .zfib file: unexpected end of data");
    const std::byte *const pointer = data + position;
    position += bytes;
    return pointer;
  }

  size_t remaining() const { return size - position; }

private:
  const std::byte *data;
  size_t size;
  size_t position;
};

//! \brief read the whole file at \a path into a byte buffer.
std::vector<std::byte> slurp(const std::filesystem::path &path) {
  std::ifstream in(path, std::ios::binary | std::ios::ate);
  if (!in)
    throw Exception("failed to open .zfib file \"" + path.string() + "\"");
  const std::streamsize size = in.tellg();
  in.seekg(0);
  std::vector<std::byte> out(static_cast<size_t>(std::max<std::streamsize>(0, size)));
  if (!out.empty())
    in.read(reinterpret_cast<char *>(out.data()), size);
  return out;
}

//! \brief append \a value to \a out in little-endian byte order.
template <typename T> void append_LE(std::vector<std::byte> &out, const T value) {
  const size_t offset = out.size();
  out.resize(offset + sizeof(T));
  Raw::store_LE<T>(value, out.data() + offset);
}

//! \brief throw a two-level "unsupported .zfib variant" exception.
[[noreturn]] void reject_variant(const std::filesystem::path &path, std::string_view detail) {
  throw Exception(Exception(std::string(detail)), "cannot read \"" + path.string() + "\" as a .zfib tractography file");
}

} // namespace

/* ************************************************************************ */
/*                          ZFIBReader<ValueType>                          */
/* ************************************************************************ */

template <class ValueType>
ZFIBReader<ValueType>::ZFIBReader(const std::filesystem::path &path, Properties &)
    : current_streamline(0), vertex_cursor(0) {
  const std::vector<std::byte> bytes = slurp(path);
  ByteReader reader(bytes.data(), bytes.size());

  const int32_t fiber_type = reader.fetch<int32_t>();
  const int32_t count_lines = reader.fetch<int32_t>();
  const int32_t encoding_type = reader.fetch<int32_t>();
  const int32_t ttype = reader.fetch<int32_t>();

  if (fiber_type != fiber_type_mrtrix)
    reject_variant(path,
                   "declares fiber type " + str(fiber_type) + " (expected " + str(fiber_type_mrtrix) +
                       ", scanner-space MRtrix)");
  if (encoding_type != encoding_huffman)
    reject_variant(path,
                   "declares encoding type " + str(encoding_type) + " but only Huffman encoding (0) is supported");
  if (ttype != transform_none)
    reject_variant(path,
                   "declares transformation type " + str(ttype) +
                       " but only the no-transformation variant (6) is supported");
  if (count_lines < 0)
    throw Exception("malformed .zfib file: negative streamline count");

  line_sizes.resize(static_cast<size_t>(count_lines));
  uint64_t total_points = 0;
  for (size_t i = 0; i != line_sizes.size(); ++i) {
    const int32_t length = reader.fetch<int32_t>();
    if (length < 0)
      throw Exception("malformed .zfib file: negative streamline length");
    line_sizes[i] = length;
    total_points += static_cast<uint64_t>(length);
  }

  // Huffman signal block: the three encoded-stream sizes, the shared dictionary,
  //   then the packed x, y and z streams in sequence.
  const EncodedSize x_encoded_size = reader.fetch<EncodedSize>();
  const EncodedSize y_encoded_size = reader.fetch<EncodedSize>();
  const EncodedSize z_encoded_size = reader.fetch<EncodedSize>();
  const EncodedSize dict_size = reader.fetch<EncodedSize>();

  std::vector<Compression::Huffman::DictEntry> dictionary(static_cast<size_t>(dict_size));
  for (size_t i = 0; i != dictionary.size(); ++i) {
    dictionary[i].symbol = reader.fetch<float>();
    dictionary[i].frequency = reader.fetch<float>();
  }
  const Compression::Huffman coder = Compression::Huffman::from_dictionary(std::move(dictionary));

  const std::byte *const x_data = reader.take(static_cast<size_t>(x_encoded_size));
  const std::byte *const y_data = reader.take(static_cast<size_t>(y_encoded_size));
  const std::byte *const z_data = reader.take(static_cast<size_t>(z_encoded_size));

  const size_t points = static_cast<size_t>(total_points);
  xs = coder.decode(x_data, static_cast<size_t>(x_encoded_size), points);
  ys = coder.decode(y_data, static_cast<size_t>(y_encoded_size), points);
  zs = coder.decode(z_data, static_cast<size_t>(z_encoded_size), points);

  // Colour flag: a minimal writer emits 0 (no colour block). A non-zero flag
  //   indicates an appended colour block this reader does not decode; warn and
  //   ignore it, the geometry having already been recovered.
  if (reader.remaining() >= 1) {
    const uint8_t colour_flag = reader.fetch<uint8_t>();
    if (colour_flag != 0) {
      WARN("\".zfib\" file \"" + path.string() + "\" carries a colour block, which is not decoded");
    }
  }
}

template <class ValueType> bool ZFIBReader<ValueType>::operator()(Streamline<ValueType> &tck) {
  tck.clear();
  if (current_streamline >= line_sizes.size())
    return false;
  const size_t npoints = static_cast<size_t>(line_sizes[current_streamline]);
  tck.set_index(current_streamline);
  tck.weight = 1.0F;
  tck.reserve(npoints);
  for (size_t i = 0; i != npoints; ++i) {
    const size_t v = vertex_cursor + i;
    tck.push_back(Eigen::Matrix<ValueType, 3, 1>(
        static_cast<ValueType>(xs[v]), static_cast<ValueType>(ys[v]), static_cast<ValueType>(zs[v])));
  }
  vertex_cursor += npoints;
  ++current_streamline;
  return true;
}

/* ************************************************************************ */
/*                          ZFIBWriter<ValueType>                          */
/* ************************************************************************ */

template <class ValueType>
ZFIBWriter<ValueType>::ZFIBWriter(const std::filesystem::path &path, const Properties &)
    : path(path), warned_weight(false), warned_sidecar(false) {
  if (path.extension() != ".zfib")
    throw Exception("output .zfib file must use the .zfib suffix");
  App::check_overwrite(path);

  max_error_mm = App::get_option_value("zfib_max_error", float(0.5));
  if (max_error_mm <= 0.0F)
    throw Exception("-zfib_max_error must be a positive value (in mm)");

  // Precision selection (paper §5.1): p = -1 below 0.2 mm, else p = 0. The
  //   quantization error budget α = √3·10ᵖ is reserved from the user error so the
  //   linearization tolerance plus the quantization error respects the worst case.
  precision = Compression::select_precision(static_cast<double>(max_error_mm));
  const double alpha = Compression::quantization_error(precision);
  tolerance_mm = static_cast<ValueType>(std::max(0.0, static_cast<double>(max_error_mm) - alpha));
}

template <class ValueType> void ZFIBWriter<ValueType>::append(const Streamline<ValueType> &tck) {
  if (tck.weight != 1.0F && !warned_weight) {
    WARN("the \".zfib\" format does not store streamline weights; weights are being discarded");
    warned_weight = true;
  }

  const Streamline<ValueType> linearized = Compression::linearize<ValueType>(tck, tolerance_mm);
  line_sizes.push_back(static_cast<int32_t>(linearized.size()));
  for (const Eigen::Matrix<ValueType, 3, 1> &vertex : linearized) {
    const float qx = Compression::uniform_quantize(static_cast<double>(vertex[0]), precision);
    const float qy = Compression::uniform_quantize(static_cast<double>(vertex[1]), precision);
    const float qz = Compression::uniform_quantize(static_cast<double>(vertex[2]), precision);
    xs.push_back(qx);
    ys.push_back(qy);
    zs.push_back(qz);
    // One dictionary is shared across x, y and z, so all three axes feed one
    //   histogram (the symbols are absolute quantized world-mm coordinates).
    ++histogram[qx];
    ++histogram[qy];
    ++histogram[qz];
  }
}

template <class ValueType> bool ZFIBWriter<ValueType>::operator()(const Streamline<ValueType> &tck) {
  append(tck);
  return true;
}

template <class ValueType> bool ZFIBWriter<ValueType>::operator()(const TractogramItem<ValueType> &item) {
  if ((!item.dps.empty() || !item.dpv.empty()) && !warned_sidecar) {
    WARN("the \".zfib\" format stores geometry only;"
         " per-streamline (dps) and per-vertex (dpv) sidecar data are being discarded");
    warned_sidecar = true;
  }
  append(item.streamline);
  return true;
}

template <class ValueType> void ZFIBWriter<ValueType>::finalise() {
  // Build the global Huffman coder from the accumulated symbol histogram, then
  //   encode each axis stream with that one shared coder.
  const Compression::Huffman coder = Compression::Huffman::from_histogram(histogram);
  const std::vector<Compression::Huffman::DictEntry> &dictionary = coder.dictionary();

  const std::vector<std::byte> x_encoded = coder.encode(xs);
  const std::vector<std::byte> y_encoded = coder.encode(ys);
  const std::vector<std::byte> z_encoded = coder.encode(zs);

  std::vector<std::byte> out;
  append_LE<int32_t>(out, fiber_type_mrtrix);
  append_LE<int32_t>(out, static_cast<int32_t>(line_sizes.size()));
  append_LE<int32_t>(out, encoding_huffman);
  append_LE<int32_t>(out, transform_none);
  for (const int32_t length : line_sizes)
    append_LE<int32_t>(out, length);

  append_LE<EncodedSize>(out, static_cast<EncodedSize>(x_encoded.size()));
  append_LE<EncodedSize>(out, static_cast<EncodedSize>(y_encoded.size()));
  append_LE<EncodedSize>(out, static_cast<EncodedSize>(z_encoded.size()));
  append_LE<EncodedSize>(out, static_cast<EncodedSize>(dictionary.size()));
  for (const Compression::Huffman::DictEntry &entry : dictionary) {
    append_LE<float>(out, entry.symbol);
    append_LE<float>(out, entry.frequency);
  }
  out.insert(out.end(), x_encoded.begin(), x_encoded.end());
  out.insert(out.end(), y_encoded.begin(), y_encoded.end());
  out.insert(out.end(), z_encoded.begin(), z_encoded.end());

  // Colour flag: this writer emits geometry only, so no colour block follows.
  append_LE<uint8_t>(out, static_cast<uint8_t>(0));

  File::OFStream stream(path, std::ios::out | std::ios::binary | std::ios::trunc);
  if (!out.empty())
    stream.write(reinterpret_cast<const char *>(out.data()), static_cast<std::streamsize>(out.size()));
}

template <class ValueType> ZFIBWriter<ValueType>::~ZFIBWriter() {
  try {
    finalise();
  } catch (Exception &e) {
    Exception(e, "ZFIB tractography file not properly finalised").display();
  }
}

/* ************************************************************************ */
/*               Explicit instantiation for float and double              */
/* ************************************************************************ */

template class ZFIBReader<float>;
template class ZFIBReader<double>;
template class ZFIBWriter<float>;
template class ZFIBWriter<double>;

namespace Formats {

bool ZFIB::handles(const std::filesystem::path &path) const { return path.extension() == ".zfib"; }

std::unique_ptr<ReaderInterface<float>> ZFIB::read_float(const std::filesystem::path &path,
                                                         Properties &properties,
                                                         FieldRegistry &,
                                                         const OptionalHeader &) const {
  return std::make_unique<ZFIBReader<float>>(path, properties);
}

std::unique_ptr<ReaderInterface<double>> ZFIB::read_double(const std::filesystem::path &path,
                                                           Properties &properties,
                                                           FieldRegistry &,
                                                           const OptionalHeader &) const {
  return std::make_unique<ZFIBReader<double>>(path, properties);
}

std::unique_ptr<WriterInterface<float>> ZFIB::create_float(const std::filesystem::path &path,
                                                           const Properties &properties,
                                                           const FieldRegistry &,
                                                           const OptionalHeader &) const {
  return std::make_unique<ZFIBWriter<float>>(path, properties);
}

std::unique_ptr<WriterInterface<double>> ZFIB::create_double(const std::filesystem::path &path,
                                                             const Properties &properties,
                                                             const FieldRegistry &,
                                                             const OptionalHeader &) const {
  return std::make_unique<ZFIBWriter<double>>(path, properties);
}

} // namespace Formats

} // namespace MR::DWI::Tractography
