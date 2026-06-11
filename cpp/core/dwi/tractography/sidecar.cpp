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

#include "dwi/tractography/sidecar.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include "datatype.h"
#include "dwi/tractography/scalar_file.h"
#include "dwi/tractography/sidecar_value.h"
#include "dwi/tractography/streamline.h"
#include "fetch_store.h"
#include "file/matrix.h"
#include "file/mmap.h"
#include "file/npy.h"
#include "file/path.h"
#include "types.h"

namespace MR::DWI::Tractography {

SidecarReference parse_sidecar_reference(std::string_view arg) {
  // §2.4: split on the LAST "::". A bare path (no "::", including a Windows
  //   "C:\\..." drive-letter path which carries only single colons) yields a
  //   reference with no field name; "DATASET::NAME" yields both components.
  const std::string_view::size_type pos = arg.rfind("::");
  if (pos == std::string_view::npos)
    return SidecarReference{std::filesystem::path(std::string(arg)), std::nullopt};
  return SidecarReference{std::filesystem::path(std::string(arg.substr(0, pos))), std::string(arg.substr(pos + 2))};
}

namespace {

//! \brief the registered field name for a standalone sidecar file (its stem).
std::string field_name_for(const std::filesystem::path &path) { return path.stem().string(); }

//! \brief is the standalone reference a per-vertex (.tsf) sidecar?
bool is_tsf(const std::filesystem::path &path) { return Path::has_suffix(path, ".tsf"); }

//! \brief is the standalone reference a NumPy (.npy) sidecar?
bool is_npy(const std::filesystem::path &path) { return Path::has_suffix(path, {".npy", ".NPY"}); }

// ---------------------------------------------------------------------------
//  Input loaders (Stage 11, step 5)
// ---------------------------------------------------------------------------

//! \brief load a per-streamline text/.csv sidecar fully into RAM and yield one
//!   row per streamline as a dps field (§2.5; step 5).
template <class ValueType> class MatrixLoader : public SidecarLoader<ValueType> {
public:
  MatrixLoader(const std::filesystem::path &path, FieldRegistry &registry)
      : data(File::Matrix::load_matrix<ValueType>(path)), row(0) {
    FieldDescriptor descriptor{field_name_for(path),
                               FieldRole::DPS,
                               sidecar_datatype<ValueType>(),
                               static_cast<size_t>(data.cols()),
                               FieldSource::External,
                               0};
    ordinal = registry.add(std::move(descriptor));
  }

  bool operator()(TractogramItem<ValueType> &item) override {
    if (row >= static_cast<size_t>(data.rows()))
      return false;
    if (item.dps.size() <= ordinal)
      item.dps.resize(ordinal + 1);
    ScalarOrVector<ValueType> value = data.row(static_cast<Eigen::Index>(row));
    item.dps[ordinal] = make_dps(std::move(value));
    ++row;
    return true;
  }

private:
  Eigen::Matrix<ValueType, Eigen::Dynamic, Eigen::Dynamic> data;
  size_t ordinal;
  size_t row;
};

//! \brief memory-map a per-streamline .npy sidecar and yield one row per
//!   streamline via an Eigen::Map over the native data region (§2.5; step 5).
template <class ValueType> class NpyLoader : public SidecarLoader<ValueType> {
public:
  NpyLoader(const std::filesystem::path &path, FieldRegistry &registry)
      : info(File::NPY::read_header(path)),
        mmap({path, info.data_offset}, false),
        fetch(MR::_set_fetch_function<ValueType>(info.data_type)),
        rows(info.shape.empty() ? 0 : info.shape[0]),
        cols(info.shape.size() == 2 ? info.shape[1] : 1),
        row(0) {
    FieldDescriptor descriptor{
        field_name_for(path), FieldRole::DPS, info.data_type, static_cast<size_t>(cols), FieldSource::External, 0};
    ordinal = registry.add(std::move(descriptor));
  }

  bool operator()(TractogramItem<ValueType> &item) override {
    if (row >= static_cast<size_t>(rows))
      return false;
    if (item.dps.size() <= ordinal)
      item.dps.resize(ordinal + 1);
    ScalarOrVector<ValueType> value(cols);
    for (ssize_t c = 0; c != cols; ++c) {
      // .npy stores row-major (C order) unless flagged column-major (F order).
      const size_t flat = info.column_major ? (static_cast<size_t>(c) * static_cast<size_t>(rows) + row)
                                            : (row * static_cast<size_t>(cols) + static_cast<size_t>(c));
      value(0, c) = fetch(mmap.address(), flat);
    }
    item.dps[ordinal] = make_dps(std::move(value));
    ++row;
    return true;
  }

private:
  File::NPY::ReadInfo info;
  File::MMap mmap;
  std::function<ValueType(const void *, size_t)> fetch;
  ssize_t rows;
  ssize_t cols;
  size_t ordinal;
  size_t row;
};

//! \brief stream a per-vertex .tsf sidecar, yielding one streamline's scalars
//!   per read as a dpv field (§2.5; step 5).
template <class ValueType> class TsfLoader : public SidecarLoader<ValueType> {
public:
  TsfLoader(const std::filesystem::path &path, Properties &properties, FieldRegistry &registry)
      : reader(path, properties) {
    FieldDescriptor descriptor{
        field_name_for(path), FieldRole::DPV, sidecar_datatype<ValueType>(), 1, FieldSource::External, 0};
    ordinal = registry.add(std::move(descriptor));
  }

  bool operator()(TractogramItem<ValueType> &item) override {
    TrackScalar<ValueType> scalars;
    if (!reader(scalars))
      return false;
    if (item.dpv.size() <= ordinal)
      item.dpv.resize(ordinal + 1);
    VectorOrMatrix<ValueType> value(static_cast<Eigen::Index>(scalars.size()), 1);
    for (size_t v = 0; v != scalars.size(); ++v)
      value(static_cast<Eigen::Index>(v), 0) = scalars[v];
    item.dpv[ordinal] = make_dpv(std::move(value));
    return true;
  }

private:
  ScalarReader<ValueType> reader;
  size_t ordinal;
};

} // namespace

template <class ValueType>
std::unique_ptr<SidecarLoader<ValueType>>
make_sidecar_loader(const SidecarReference &reference, Properties &properties, FieldRegistry &registry) {
  if (reference.is_qualified())
    throw Exception("import of a qualified \"DATASET::NAME\" tractogram-sidecar reference"
                    " is not yet implemented");
  if (is_tsf(reference.dataset))
    return std::make_unique<TsfLoader<ValueType>>(reference.dataset, properties, registry);
  if (is_npy(reference.dataset))
    return std::make_unique<NpyLoader<ValueType>>(reference.dataset, registry);
  return std::make_unique<MatrixLoader<ValueType>>(reference.dataset, registry);
}

template std::unique_ptr<SidecarLoader<float>>
make_sidecar_loader<float>(const SidecarReference &, Properties &, FieldRegistry &);
template std::unique_ptr<SidecarLoader<double>>
make_sidecar_loader<double>(const SidecarReference &, Properties &, FieldRegistry &);

} // namespace MR::DWI::Tractography
