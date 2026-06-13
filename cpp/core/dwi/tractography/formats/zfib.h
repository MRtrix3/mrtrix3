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
#include <map>
#include <memory>
#include <vector>

#include "dwi/tractography/compression/huffman.h"
#include "dwi/tractography/formats/base.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"

namespace MR::DWI::Tractography {

//! \brief Streaming reader backend for the lossy ".zfib" compression format.
/*! A ".zfib" dataset (Presseau et al., "A new compression format for fiber
 * tracking datasets") stores streamline geometry that has been linearized,
 * uniformly quantized and Huffman-encoded; decoding is exact with respect to the
 * quantized coordinates, so the reader needs no parameters. Because the Huffman
 * dictionary is global over the whole dataset, the reader decodes the entire
 * file into RAM at construction and then emits streamlines sequentially.
 *
 * Coordinates are absolute world-mm floats (FIBERTYPE_MRTRIX ≡ ".tck"
 * scanner-space), so no grid transform is applied. Explicitly instantiated for
 * float and double in formats/zfib.cpp. */
template <class ValueType = float> class ZFIBReader : public ReaderInterface<ValueType> {
public:
  ZFIBReader(const std::filesystem::path &path, Properties &properties);

  bool operator()(Streamline<ValueType> &tck) override;

private:
  std::vector<int32_t> line_sizes; //!< per-streamline vertex count, post-linearization
  std::vector<float> xs;           //!< decoded x coordinates, concatenated over all streamlines
  std::vector<float> ys;
  std::vector<float> zs;
  size_t current_streamline; //!< ordinal of the next streamline to emit
  size_t vertex_cursor;      //!< running index into xs/ys/zs

  ZFIBReader(const ZFIBReader &) = delete;
};

//! \brief Streaming writer backend for the lossy ".zfib" compression format.
/*! The writer cannot stream: the Huffman dictionary is global over the whole
 * dataset, so all (linearized, quantized) coordinates are buffered, the coder is
 * built at finalisation, and the file is then emitted — the buffer-and-finalise
 * pattern shared with TRXWriter (finalise() is invoked from the destructor).
 *
 * The maximum permitted error (mm) is read from the "-zfib_max_error" command
 * line option (default 0.5). From it the quantization precision p and the
 * linearization tolerance are derived per the paper: p = -1 for an error below
 * 0.2 mm, else p = 0; the quantization error budget α = √3·10ᵖ is subtracted
 * from the user error to leave the linearization tolerance.
 *
 * The format carries geometry only: per-streamline weight (SIFT2) and dps/dpv
 * sidecars have no slot and are warned about once and dropped, rather than
 * aborting a geometry-only lossy export. Explicitly instantiated for float and
 * double in formats/zfib.cpp. */
template <class ValueType = float> class ZFIBWriter : public WriterInterface<ValueType> {
public:
  ZFIBWriter(const std::filesystem::path &path, const Properties &properties);
  ~ZFIBWriter() override;

  bool operator()(const Streamline<ValueType> &tck) override;
  //! \brief append a composite item, warning once before dropping any sidecars.
  bool operator()(const TractogramItem<ValueType> &item) override;

private:
  const std::filesystem::path path;

  float max_error_mm;     //!< worst-case error budget (mm) from -zfib_max_error
  int precision;          //!< uniform-quantization precision p (0 or -1)
  ValueType tolerance_mm; //!< linearization tolerance = max(0, max_error - α)

  std::vector<int32_t> line_sizes; //!< per-streamline vertex count, post-linearization
  std::vector<float> xs;           //!< quantized x coordinates, concatenated
  std::vector<float> ys;
  std::vector<float> zs;
  std::map<float, uint64_t> histogram; //!< symbol counts shared across x, y and z

  bool warned_weight;  //!< whether the dropped-weight warning has been emitted
  bool warned_sidecar; //!< whether the dropped-sidecar warning has been emitted

  //! \brief linearize, quantize and buffer one streamline's vertices.
  void append(const Streamline<ValueType> &tck);

  //! \brief build the coder and emit the .zfib file from the buffered data.
  void finalise();

  ZFIBWriter(const ZFIBWriter &) = delete;
};

//! \brief non-finite tolerance broadcast by the ".zfib" handler and enforced by its writer.
/*! ".zfib" linearizes and uniformly quantizes coordinates before Huffman
 * encoding, which cannot represent a non-finite coordinate. */
inline constexpr Formats::NonFinite zfib_vertex_tolerance = Formats::NonFinite::Forbidden;

namespace Formats {

//! \brief Format handler for the lossy ".zfib" streamline-compression format.
/*! ".zfib" is the lossy compression format of Presseau et al.: a linearization
 * (Ramer–Douglas–Peucker), a uniform quantization, and a Huffman encoding, all
 * implemented natively on the MRtrix API. Byte layout reverse-engineered from
 * the reference encoder (github.com/scilus/FiberCompression).
 *
 * Capabilities: read+write (decode is exact and the format is writable);
 * Streaming (no random access — the reader loads all, the writer buffers all);
 * Rewrite (the global dictionary and per-streamline size table are fixed at
 * finalisation, so an existing dataset cannot be appended to in place). */
class ZFIB : public Base {
public:
  ZFIB()
      : Base("ZFIB (lossy streamline compression)",
             {IO::ReadWrite,
              Access::Streaming,
              Augment::Rewrite,
              StepSize::Arbitrary,
              zfib_vertex_tolerance,
              NonFinite::Forbidden}) {}

  bool handles(const std::filesystem::path &path) const override;

protected:
  std::unique_ptr<ReaderInterface<float>> read_float(const std::filesystem::path &path,
                                                     Properties &properties,
                                                     FieldRegistry &registry,
                                                     const OptionalHeader &grid) const override;
  std::unique_ptr<ReaderInterface<double>> read_double(const std::filesystem::path &path,
                                                       Properties &properties,
                                                       FieldRegistry &registry,
                                                       const OptionalHeader &grid) const override;
  std::unique_ptr<WriterInterface<float>> create_float(const std::filesystem::path &path,
                                                       const Properties &properties,
                                                       const FieldRegistry &registry,
                                                       const OptionalHeader &grid) const override;
  std::unique_ptr<WriterInterface<double>> create_double(const std::filesystem::path &path,
                                                         const Properties &properties,
                                                         const FieldRegistry &registry,
                                                         const OptionalHeader &grid) const override;
};

} // namespace Formats

} // namespace MR::DWI::Tractography
