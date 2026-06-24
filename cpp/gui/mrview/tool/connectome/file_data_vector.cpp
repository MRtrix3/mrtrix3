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

#include "mrview/tool/connectome/file_data_vector.h"

#include <filesystem>
#include <limits>
#include <utility>

#include "file/matrix.h"
#include "gui.h"

namespace MR::GUI::MRView::Tool {

FileDataVector::FileDataVector() : base_t(), min(NaNF), mean(NaNF), max(NaNF) {}

FileDataVector::FileDataVector(const FileDataVector &V)
    : base_t(V), name(V.name), min(V.min), mean(V.mean), max(V.max) {}

FileDataVector::FileDataVector(FileDataVector &&V) noexcept
    : base_t(std::move(static_cast<base_t &&>(V))), name(std::move(V.name)), min(V.min), mean(V.mean), max(V.max) {
  V.name.clear();
  V.min = V.mean = V.max = NaNF;
}

FileDataVector::FileDataVector(const size_t nelements) : base_t(nelements), min(NaNF), mean(NaNF), max(NaNF) {}

FileDataVector::FileDataVector(const std::filesystem::path &file)
    : base_t(), name(qstr(file.filename().string())), min(NaNF), mean(NaNF), max(NaNF) {
  const base_t temp = File::Matrix::load_vector<float>(file);
  base_t::operator=(temp);
  calc_stats();
}

FileDataVector &FileDataVector::operator=(const FileDataVector &that) = default;
FileDataVector &FileDataVector::operator=(FileDataVector &&that) noexcept {
  base_t::operator=(std::move(static_cast<base_t &&>(that)));
  name = that.name;
  min = that.min;
  mean = that.mean;
  max = that.max;
  that.name.clear();
  that.min = NaNF;
  that.mean = NaNF;
  that.max = NaNF;
  return *this;
}

FileDataVector &FileDataVector::load(const std::filesystem::path &filePath) {
  const base_t temp = File::Matrix::load_vector<float>(filePath);
  base_t::operator=(temp);
  name = qstr(filePath.filename().string());
  calc_stats();
  return *this;
}

FileDataVector &FileDataVector::clear() {
  base_t::resize(0);
  name.clear();
  min = NaNF;
  mean = NaNF;
  max = NaNF;
  return *this;
}

void FileDataVector::calc_stats() {
  min = InfF;
  double sum = 0.0;
  max = -InfF;
  for (Eigen::Index i = 0; i != size(); ++i) {
    min = std::min(min, operator[](i));
    sum += operator[](i);
    max = std::max(max, operator[](i));
  }
  mean = sum / static_cast<double>(size());
}

} // namespace MR::GUI::MRView::Tool
