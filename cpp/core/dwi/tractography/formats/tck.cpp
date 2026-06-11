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

#include "dwi/tractography/formats/tck.h"

namespace MR::DWI::Tractography::Formats {

bool TCK::handles(const std::filesystem::path &path) const { return path.extension() == ".tck"; }

std::unique_ptr<ReaderInterface<float>> TCK::read_float(const std::filesystem::path &path,
                                                        Properties &properties) const {
  return std::make_unique<Reader<float>>(path, properties);
}

std::unique_ptr<ReaderInterface<double>> TCK::read_double(const std::filesystem::path &path,
                                                          Properties &properties) const {
  return std::make_unique<Reader<double>>(path, properties);
}

std::unique_ptr<WriterInterface<float>> TCK::create_float(const std::filesystem::path &path,
                                                          const Properties &properties) const {
  return std::make_unique<Writer<float>>(path, properties);
}

std::unique_ptr<WriterInterface<double>> TCK::create_double(const std::filesystem::path &path,
                                                            const Properties &properties) const {
  return std::make_unique<Writer<double>>(path, properties);
}

} // namespace MR::DWI::Tractography::Formats
