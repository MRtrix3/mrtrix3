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

#include "dwi/tractography/formats/qfib.h"

#include "dwi/tractography/nonfinite.h"

#include <array>
#include <cstddef>
#include <limits>
#include <vector>

#include "app.h"
#include "exception.h"
#include "math/math.h"
#include "mrtrix.h"
#include "raw.h"

namespace MR::DWI::Tractography {

namespace {

//! \brief the on-disk format version written by the reference encoder (Mercier et al.).
/*! ".qfib" has only ever defined this single version. (MRtrix once also wrote a
 * non-standard "version 2" with a different, manuscript-derived octahedral packing;
 * as no such files exist outside MRtrix's own former output, that variant has been
 * removed — MRtrix now reads and writes only the reference format.) */
constexpr uint8_t qfib_version = 1;

//! \brief the method byte values (offset 9): which per-direction quantization was used.
constexpr uint8_t qfib_method_octahedral = 0;
constexpr uint8_t qfib_method_fibonacci = 1;

//! \brief default relative tolerance for the constant-stepsize requirement.
constexpr double qfib_default_step_tolerance = 5.0e-2;

//! \brief default maximum deviation angle (degrees) when none is otherwise known.
constexpr double qfib_default_max_angle = 90.0;

//! \brief on-disk header size: version(1) + count(4) + ratio(4) + method(1) + bit depth(1).
constexpr size_t qfib_header_bytes = 11;
//! \brief on-disk per-fiber preamble: first + second point + nb_compressed.
constexpr size_t qfib_record_preamble_bytes = 3 * sizeof(float) + 3 * sizeof(float) + sizeof(uint16_t);

//! \brief map the on-disk quantization byte to the codec bit depth.
Formats::QFibCodec::BitDepth bitdepth_from_byte(uint8_t quantization, const std::filesystem::path &path) {
  switch (quantization) {
  case 8:
    return Formats::QFibCodec::BitDepth::M8;
  case 16:
    return Formats::QFibCodec::BitDepth::M16;
  default:
    throw Exception(Exception("declares quantization " + str(static_cast<int>(quantization)) +
                              " bits, but only 8 and 16 are supported"),
                    "cannot read \"" + path.string() + "\" as a .qfib tractography file");
  }
}

//! \brief number of bytes occupied by one quantized direction at the given bit depth.
constexpr size_t index_bytes(Formats::QFibCodec::BitDepth bitdepth) noexcept {
  return bitdepth == Formats::QFibCodec::BitDepth::M8 ? sizeof(uint8_t) : sizeof(uint16_t);
}

} // namespace

/* ************************************************************************ */
/*                          QFibReader<ValueType>                          */
/* ************************************************************************ */

template <class ValueType>
QFibReader<ValueType>::QFibReader(const std::filesystem::path &path, Properties &properties)
    : path(path),
      in(path, std::ios::binary),
      scheme(Formats::QFibCodec::Scheme::Octahedral),
      nb_fibers(0),
      current_index(0) {
  if (!in)
    throw Exception("failed to open .qfib file \"" + path.string() + "\"");

  std::array<std::byte, qfib_header_bytes> header{};
  read_exact(header.data(), header.size());

  const uint8_t version = Raw::fetch_LE<uint8_t>(header.data());
  if (version != qfib_version)
    throw Exception(Exception("declares version " + str(static_cast<int>(version)) + ", but only version " +
                              str(static_cast<int>(qfib_version)) + " is supported"),
                    "cannot read \"" + path.string() + "\" as a .qfib tractography file");
  nb_fibers = Raw::fetch_LE<uint32_t>(header.data() + 1);
  const float header_ratio = Raw::fetch_LE<float>(header.data() + 5);
  const uint8_t method = Raw::fetch_LE<uint8_t>(header.data() + 9);
  const uint8_t bit_depth_byte = Raw::fetch_LE<uint8_t>(header.data() + 10);

  std::string scheme_label;
  switch (method) {
  case qfib_method_octahedral:
    scheme = Formats::QFibCodec::Scheme::Octahedral;
    scheme_label = "octahedral";
    break;
  case qfib_method_fibonacci:
    scheme = Formats::QFibCodec::Scheme::Fibonacci;
    scheme_label = "spherical-Fibonacci";
    break;
  default:
    throw Exception(Exception("declares quantization method " + str(static_cast<int>(method)) +
                              ", but only octahedral (0) and spherical-Fibonacci (1) are supported"),
                    "cannot read \"" + path.string() + "\" as a .qfib tractography file");
  }

  bitdepth = bitdepth_from_byte(bit_depth_byte, path);
  ratio = static_cast<double>(header_ratio);
  if (ratio <= 0.0)
    throw Exception("malformed .qfib file \"" + path.string() + "\": non-positive cap ratio");

  // Provenance: the deviation angle implied by the stored ratio (re-read by the
  //   writer as the default angle, so a .qfib -> .qfib copy preserves the ratio),
  //   plus the quantization method and depth recorded as a comment.
  const double psi_degrees = Formats::QFibCodec::angle_from_ratio(ratio) * 180.0 / Math::pi;
  properties["max_angle"] = str(psi_degrees);
  properties.comments.push_back("QFib " + scheme_label + " quantization: " + str(static_cast<int>(bit_depth_byte)) +
                                "-bit");
}

template <class ValueType> void QFibReader<ValueType>::read_exact(std::byte *buffer, size_t count) {
  in.read(reinterpret_cast<char *>(buffer), static_cast<std::streamsize>(count));
  if (static_cast<size_t>(in.gcount()) != count)
    throw Exception("malformed .qfib file \"" + path.string() + "\": unexpected end of data");
}

template <class ValueType> bool QFibReader<ValueType>::operator()(Streamline<ValueType> &tck) {
  tck.clear();
  if (current_index >= nb_fibers)
    return false;

  std::array<std::byte, qfib_record_preamble_bytes> preamble{};
  read_exact(preamble.data(), preamble.size());

  Formats::QFibCodec::Compressed<ValueType> cfiber;
  for (size_t axis = 0; axis != 3; ++axis)
    cfiber.first[axis] = static_cast<ValueType>(Raw::fetch_LE<float>(preamble.data(), axis));
  for (size_t axis = 0; axis != 3; ++axis)
    cfiber.second[axis] = static_cast<ValueType>(Raw::fetch_LE<float>(preamble.data() + 3 * sizeof(float), axis));
  const uint16_t nb_compressed = Raw::fetch_LE<uint16_t>(preamble.data() + 6 * sizeof(float));

  std::vector<std::byte> packed(static_cast<size_t>(nb_compressed) * index_bytes(bitdepth));
  if (!packed.empty())
    read_exact(packed.data(), packed.size());

  cfiber.indices.resize(nb_compressed);
  for (size_t i = 0; i != nb_compressed; ++i) {
    if (bitdepth == Formats::QFibCodec::BitDepth::M8)
      cfiber.indices[i] = static_cast<int32_t>(Raw::fetch_LE<uint8_t>(packed.data(), i));
    else
      cfiber.indices[i] = static_cast<int32_t>(Raw::fetch_LE<uint16_t>(packed.data(), i));
  }

  tck = Formats::QFibCodec::decompress<ValueType>(cfiber, scheme, bitdepth, ratio);
  tck.set_index(current_index);
  tck.weight = 1.0F;
  ++current_index;
  return true;
}

/* ************************************************************************ */
/*                          QFibWriter<ValueType>                          */
/* ************************************************************************ */

template <class ValueType>
QFibWriter<ValueType>::QFibWriter(const std::filesystem::path &path, const Properties &properties)
    : path(path),
      scheme(Formats::QFibCodec::Scheme::Octahedral),
      step_tolerance(qfib_default_step_tolerance),
      count_offset(0),
      nb_fibers(0),
      warned_short(false) {
  if (path.extension() != ".qfib")
    throw Exception("output .qfib file must use the .qfib suffix");

  // Output is always the reference format (version 1) with octahedral quantization:
  //   MRtrix does not implement the spherical-Fibonacci encoder, and the reference
  //   tool reads either method, so octahedral output is fully interoperable.

  // Bit depth: -qfib_bits is either 8 or 16, defaulting to 16.
  switch (App::get_option_value("qfib_bits", 16)) {
  case 8:
    bitdepth = Formats::QFibCodec::BitDepth::M8;
    break;
  case 16:
    bitdepth = Formats::QFibCodec::BitDepth::M16;
    break;
  default:
    throw Exception("-qfib_bits must be either 8 or 16");
  }

  // Maximum deviation angle psi: the -qfib_max_angle option if given, else the
  //   "max_angle" property carried by the input, else a documented default.
  double psi_degrees = qfib_default_max_angle;
  if (!App::get_options("qfib_max_angle").empty()) {
    psi_degrees = App::get_option_value("qfib_max_angle", qfib_default_max_angle);
  } else {
    const auto it = properties.find("max_angle");
    if (it != properties.end()) {
      try {
        psi_degrees = to<double>(it->second);
      } catch (Exception &) {
        psi_degrees = qfib_default_max_angle;
      }
    }
  }
  ratio = Formats::QFibCodec::ratio_from_angle(psi_degrees * Math::pi / 180.0);

  out.open(path, std::ios::out | std::ios::binary | std::ios::trunc);

  // Write the header with a placeholder streamline count (offset 1), recording
  //   that offset for backfill at finalisation.
  std::array<std::byte, qfib_header_bytes> header{};
  Raw::store_LE<uint8_t>(qfib_version, header.data());
  Raw::store_LE<uint32_t>(0U, header.data() + 1);
  Raw::store_LE<float>(static_cast<float>(ratio), header.data() + 5);
  Raw::store_LE<uint8_t>(qfib_method_octahedral, header.data() + 9);
  Raw::store_LE<uint8_t>(static_cast<uint8_t>(bitdepth), header.data() + 10);
  out.write(reinterpret_cast<const char *>(header.data()), header.size());
  count_offset = 1;
}

template <class ValueType> bool QFibWriter<ValueType>::operator()(const Streamline<ValueType> &tck) {
  if (tck.size() < 2) {
    if (!warned_short) {
      WARN("the \".qfib\" format requires at least two vertices per streamline;"
           " shorter streamlines are being skipped");
      warned_short = true;
    }
    return true;
  }

  enforce_vertices(tck, qfib_vertex_tolerance);
  Formats::QFibCodec::Compressed<ValueType> cfiber;
  try {
    cfiber = Formats::QFibCodec::compress<ValueType>(tck, scheme, bitdepth, ratio, step_tolerance);
  } catch (Exception &e) {
    throw Exception(e, "cannot write streamline " + str(nb_fibers) + " to \"" + path.string() + "\"");
  }
  if (cfiber.indices.size() > std::numeric_limits<uint16_t>::max())
    throw Exception("streamline of " + str(tck.size()) + " vertices exceeds the .qfib per-record limit");

  std::vector<std::byte> record(qfib_record_preamble_bytes + cfiber.indices.size() * index_bytes(bitdepth));
  for (size_t axis = 0; axis != 3; ++axis)
    Raw::store_LE<float>(static_cast<float>(cfiber.first[axis]), record.data(), axis);
  for (size_t axis = 0; axis != 3; ++axis)
    Raw::store_LE<float>(static_cast<float>(cfiber.second[axis]), record.data() + 3 * sizeof(float), axis);
  Raw::store_LE<uint16_t>(static_cast<uint16_t>(cfiber.indices.size()), record.data() + 6 * sizeof(float));

  std::byte *const packed = record.data() + qfib_record_preamble_bytes;
  for (size_t i = 0; i != cfiber.indices.size(); ++i) {
    if (bitdepth == Formats::QFibCodec::BitDepth::M8)
      Raw::store_LE<uint8_t>(static_cast<uint8_t>(cfiber.indices[i]), packed, i);
    else
      Raw::store_LE<uint16_t>(static_cast<uint16_t>(cfiber.indices[i]), packed, i);
  }

  out.write(reinterpret_cast<const char *>(record.data()), static_cast<std::streamsize>(record.size()));
  ++nb_fibers;
  return true;
}

template <class ValueType> void QFibWriter<ValueType>::finalise() {
  out.flush();
  out.seekp(count_offset);
  std::array<std::byte, sizeof(uint32_t)> count_bytes{};
  Raw::store_LE<uint32_t>(nb_fibers, count_bytes.data());
  out.write(reinterpret_cast<const char *>(count_bytes.data()), count_bytes.size());
  out.flush();
  if (!out)
    throw Exception("error writing .qfib file \"" + path.string() + "\"");
}

template <class ValueType> QFibWriter<ValueType>::~QFibWriter() {
  try {
    finalise();
  } catch (Exception &e) {
    Exception(e, "QFib tractography file not properly finalised").display();
  }
}

/* ************************************************************************ */
/*               Explicit instantiation for float and double              */
/* ************************************************************************ */

template class QFibReader<float>;
template class QFibReader<double>;
template class QFibWriter<float>;
template class QFibWriter<double>;

namespace Formats {

bool QFIB::handles(const std::filesystem::path &path) const { return path.extension() == ".qfib"; }

std::unique_ptr<ReaderInterface<float>>
QFIB::read_float(const std::filesystem::path &path, Properties &properties, FieldRegistry &) const {
  return std::make_unique<QFibReader<float>>(path, properties);
}

std::unique_ptr<ReaderInterface<double>>
QFIB::read_double(const std::filesystem::path &path, Properties &properties, FieldRegistry &) const {
  return std::make_unique<QFibReader<double>>(path, properties);
}

std::unique_ptr<WriterInterface<float>> QFIB::create_float(const std::filesystem::path &path,
                                                           const Properties &properties,
                                                           const FieldRegistry &,
                                                           const WriteOptions &options) const {
  return std::make_unique<QFibWriter<float>>(path, properties);
}

std::unique_ptr<WriterInterface<double>> QFIB::create_double(const std::filesystem::path &path,
                                                             const Properties &properties,
                                                             const FieldRegistry &,
                                                             const WriteOptions &options) const {
  return std::make_unique<QFibWriter<double>>(path, properties);
}

} // namespace Formats

} // namespace MR::DWI::Tractography
