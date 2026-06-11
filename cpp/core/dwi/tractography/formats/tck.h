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

#include <filesystem>
#include <memory>

#include "dwi/tractography/formats/base.h"
#include "dwi/tractography/properties.h"

namespace MR::DWI::Tractography::Formats {

//! \brief Format handler for the in-house MRtrix3 ".tck" tractography format.
/*! The ".tck" format is a plaintext header followed by a binary stream of
 * 3-vectors (NaN-delimited streamlines, Inf end-of-data barrier). It is the
 * first concrete handler in the tractography format subsystem and the
 * template against which subsequent handlers are modelled.
 *
 * Capabilities: read+write; sequential streaming access (the binary stream is
 * consumed in order); rewrite-only for structural change, although growth by
 * appending streamlines is supported during a single writing pass. */
class TCK : public Base {
public:
  TCK() : Base("MRtrix tracks", {IO::ReadWrite, Access::Streaming, Augment::Rewrite}) {}

  bool handles(const std::filesystem::path &path) const override;

protected:
  std::unique_ptr<ReaderInterface<float>> read_float(const std::filesystem::path &path,
                                                     Properties &properties) const override;
  std::unique_ptr<ReaderInterface<double>> read_double(const std::filesystem::path &path,
                                                       Properties &properties) const override;
  std::unique_ptr<WriterInterface<float>> create_float(const std::filesystem::path &path,
                                                       const Properties &properties) const override;
  std::unique_ptr<WriterInterface<double>> create_double(const std::filesystem::path &path,
                                                         const Properties &properties) const override;
};

} // namespace MR::DWI::Tractography::Formats
