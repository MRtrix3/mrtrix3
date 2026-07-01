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

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>

#include "file/ofstream.h"

#include "dwi/tractography/formats/base.h"
#include "dwi/tractography/formats/qfib_codec.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"

namespace MR::DWI::Tractography {

//! \brief Streaming reader backend for the lossy ".qfib" compression format.
/*! A ".qfib" dataset (Mercier et al., "QFib: Fast and Efficient Brain Tractogram
 * Compression") stores each streamline as its first two points plus a sequence
 * of quantized unit tangents, exploiting a constant per-streamline step size and a
 * bounded maximum deviation angle. The 11-byte header carries the format version,
 * the streamline count, the cap-to-sphere area ratio, the quantization method and
 * the bit depth; records are variable-length with no offset table, so the format is
 * read sequentially (one streamline per operator()).
 *
 * This is the reference format (Mercier et al., version 1); the header's method
 * byte selects signed-octahedral or spherical-Fibonacci index packing, both of
 * which are read.
 *
 * Coordinates are absolute scanner-space mm (as in ".tck"), so no grid transform
 * is applied. Explicitly instantiated for float and double in formats/qfib.cpp. */
template <class ValueType = float> class QFibReader : public ReaderInterface<ValueType> {
public:
  QFibReader(const std::filesystem::path &path, Properties &properties);

  bool operator()(Streamline<ValueType> &tck) override;

private:
  const std::filesystem::path path;
  std::ifstream in;
  Formats::QFibCodec::Scheme scheme;
  Formats::QFibCodec::BitDepth bitdepth;
  double ratio;
  uint32_t nb_fibers;
  uint32_t current_index;

  //! \brief read \a count bytes into \a buffer, throwing on a short read.
  void read_exact(std::byte *buffer, size_t count);

  QFibReader(const QFibReader &) = delete;
};

//! \brief Streaming writer backend for the lossy ".qfib" compression format.
/*! Each streamline is compressed independently (Formats::QFibCodec::compress) and appended
 * to the file as it arrives; the streamline count is written as a placeholder in
 * the header and backfilled at finalisation. Streamlines of fewer than two
 * vertices are skipped with a warning. Compression throws if a streamline is not
 * of constant step size (the format cannot represent it).
 *
 * The format carries geometry only: per-streamline weights and dps/dpv sidecars
 * have no slot and are dropped silently (the default TractogramItem overload).
 * Explicitly instantiated for float and double in formats/qfib.cpp. */
template <class ValueType = float> class QFibWriter : public WriterInterface<ValueType> {
public:
  QFibWriter(const std::filesystem::path &path, const Properties &properties);
  ~QFibWriter() override;

  bool operator()(const Streamline<ValueType> &tck) override;

private:
  const std::filesystem::path path;
  File::OFStream out;
  Formats::QFibCodec::Scheme scheme;
  Formats::QFibCodec::BitDepth bitdepth;
  double ratio;
  double step_tolerance;
  std::streamoff count_offset; //!< byte offset of the nb_fibers field, for backfill
  uint32_t nb_fibers;
  bool warned_short; //!< whether the dropped-short-streamline warning has been emitted

  //! \brief seek back to the header and write the final streamline count.
  void finalise();

  QFibWriter(const QFibWriter &) = delete;
};

//! \brief non-finite tolerance broadcast by the ".qfib" handler and enforced by its writer.
/*! ".qfib" octahedrally quantizes unit tangents and stores a constant step size,
 * neither of which can represent a non-finite coordinate. */
inline constexpr Formats::NonFinite qfib_vertex_tolerance = Formats::NonFinite::Forbidden;

namespace Formats {

//! \brief Format handler for the lossy ".qfib" streamline-compression format.
/*! ".qfib" is the lossy per-streamline compression format of Mercier et al.: a
 * streamline is stored as its first two vertices plus octahedrally-quantized unit
 * tangents mapped through a Rousseau-Boubekeur spherical-cap parameterisation.
 * Coordinates are scanner-space mm; the format carries geometry only.
 *
 * Capabilities: read+write; sequential streaming access (records are
 * variable-length with no offset table); rewrite (the header streamline count is
 * fixed at finalisation, so an existing dataset cannot be appended to in place). */
class QFIB : public Base {
public:
  QFIB()
      : Base("QFib compressed tracks",
             {IO::ReadWrite,
              Access::Streaming,
              Augment::Rewrite,
              StepSize::Constant,
              qfib_vertex_tolerance,
              NonFinite::Forbidden}) {}

  bool handles(const std::filesystem::path &path) const override;

protected:
  std::unique_ptr<ReaderInterface<float>>
  read_float(const std::filesystem::path &path, Properties &properties, FieldRegistry &registry) const override;
  std::unique_ptr<ReaderInterface<double>>
  read_double(const std::filesystem::path &path, Properties &properties, FieldRegistry &registry) const override;
  std::unique_ptr<WriterInterface<float>> create_float(const std::filesystem::path &path,
                                                       const Properties &properties,
                                                       const FieldRegistry &registry,
                                                       const WriteOptions &options) const override;
  std::unique_ptr<WriterInterface<double>> create_double(const std::filesystem::path &path,
                                                         const Properties &properties,
                                                         const FieldRegistry &registry,
                                                         const WriteOptions &options) const override;
};

} // namespace Formats

} // namespace MR::DWI::Tractography
