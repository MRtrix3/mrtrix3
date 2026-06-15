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

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include "app.h"
#include "datatype.h"
#include "dwi/tractography/scalar_file.h"
#include "dwi/tractography/sidecar_value.h"
#include "dwi/tractography/streamline.h"
#include "fetch_store.h"
#include "file/matrix.h"
#include "file/mmap.h"
#include "file/npy.h"
#include "file/path.h"
#include "mrtrix.h"
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

} // namespace

std::string sidecar_field_name(const SidecarReference &reference) {
  if (reference.is_qualified())
    return *reference.name;
  return reference.dataset.stem().string();
}

namespace {

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

// ---------------------------------------------------------------------------
//  Output exporters (Stage 11, step 6)
// ---------------------------------------------------------------------------

//! \brief accumulate processed per-streamline dps data in an Eigen::Array<>,
//!   grown on the std::vector<> doubling schedule, and write a numerical text
//!   file or .npy on finalise (§2.7; step 6).
template <class ValueType> class MatrixExporter : public SidecarExporter<ValueType> {
public:
  MatrixExporter(const std::filesystem::path &path, const size_t initial_streamlines)
      : path(path),
        data(Eigen::Array<ValueType, Eigen::Dynamic, Eigen::Dynamic>::Zero(initial_streamlines, 1)),
        rows(0),
        cols(0),
        finalised(false) {}

  ~MatrixExporter() override {
    try {
      finalise();
    } catch (Exception &e) {
      Exception(e, "Error finalising tractogram-sidecar output \"" + path.string() + "\"").display();
    }
  }

  bool operator()(const TractogramItem<ValueType> &item) override {
    if (item.dps.empty())
      throw Exception("tractogram-sidecar export \"" + path.string() + "\"" +
                      " received a streamline with no per-streamline data to export");
    const ScalarOrVector<ValueType> &value = std::get<ScalarOrVector<ValueType>>(item.dps.front());
    const size_t index = item.get_index();
    grow_to(index + 1, static_cast<size_t>(value.cols()));
    data.row(static_cast<Eigen::Index>(index)).head(value.cols()) = value.array();
    if (index + 1 > rows)
      rows = index + 1;
    return true;
  }

  void finalise() override {
    if (finalised)
      return;
    finalised = true;
    Eigen::Matrix<ValueType, Eigen::Dynamic, Eigen::Dynamic> out =
        data.topLeftCorner(static_cast<Eigen::Index>(rows), static_cast<Eigen::Index>(cols)).matrix();
    File::Matrix::save_matrix(out, path);
  }

private:
  std::filesystem::path path;
  Eigen::Array<ValueType, Eigen::Dynamic, Eigen::Dynamic> data;
  size_t rows;
  size_t cols;
  bool finalised;

  //! grow the backing array to at least \a need_rows x \a need_cols, expanding
  //!   the row capacity geometrically (the std::vector<> doubling schedule).
  void grow_to(const size_t need_rows, const size_t need_cols) {
    if (need_cols > cols)
      cols = need_cols;
    size_t capacity = static_cast<size_t>(data.rows());
    if (need_rows > capacity || static_cast<size_t>(data.cols()) < cols) {
      while (capacity < need_rows)
        capacity = (capacity == 0) ? 1 : capacity * 2;
      data.conservativeResizeLike(
          Eigen::Array<ValueType, Eigen::Dynamic, Eigen::Dynamic>::Zero(capacity, static_cast<Eigen::Index>(cols)));
    }
  }
};

//! \brief write processed per-vertex dpv data to a .tsf file as streamlines are
//!   fed to the output tractogram (§2.7; step 6).
template <class ValueType> class TsfExporter : public SidecarExporter<ValueType> {
public:
  TsfExporter(const std::filesystem::path &path, const Properties &properties) : writer(path, properties) {}

  bool operator()(const TractogramItem<ValueType> &item) override {
    if (item.dpv.empty())
      throw Exception("per-vertex tractogram-sidecar (.tsf) export received a streamline with no per-vertex data");
    const VectorOrMatrix<ValueType> &value = std::get<VectorOrMatrix<ValueType>>(item.dpv.front());
    TrackScalar<ValueType> scalars;
    scalars.set_index(item.get_index());
    scalars.resize(static_cast<size_t>(value.rows()));
    for (Eigen::Index v = 0; v != value.rows(); ++v)
      scalars[static_cast<size_t>(v)] = value(v, 0);
    writer(scalars);
    return true;
  }

  void finalise() override {}

private:
  ScalarWriter<ValueType> writer;
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

template <class ValueType>
std::unique_ptr<SidecarExporter<ValueType>>
make_sidecar_exporter(const SidecarReference &reference, const Properties &properties, const bool is_random_access) {
  if (reference.is_qualified())
    throw Exception("export of a qualified \"DATASET::NAME\" tractogram-sidecar reference"
                    " is not yet implemented");
  if (is_tsf(reference.dataset)) {
    // Step 7: the .tsf format is inherently sequential and cannot be produced
    //   under a random-access request to the tractogram content.
    if (is_random_access)
      throw Exception("cannot export a per-vertex track scalar (.tsf) file \"" + reference.dataset.string() + "\"" +
                      " when random access to the tractogram content has been requested" +
                      " (the .tsf format precludes random access)");
    return std::make_unique<TsfExporter<ValueType>>(reference.dataset, properties);
  }
  // A per-streamline export starts sized for the input streamline count where
  //   known (0 -> grown geometrically as streamlines arrive).
  const size_t initial = properties.find("count") != properties.end() ? to<size_t>(properties.at("count")) : size_t(0);
  return std::make_unique<MatrixExporter<ValueType>>(reference.dataset, initial);
}

template std::unique_ptr<SidecarExporter<float>>
make_sidecar_exporter<float>(const SidecarReference &, const Properties &, bool);
template std::unique_ptr<SidecarExporter<double>>
make_sidecar_exporter<double>(const SidecarReference &, const Properties &, bool);

// ---------------------------------------------------------------------------
//  Streamline-weight I/O (the privileged Streamline::weight route)
// ---------------------------------------------------------------------------

template <class ValueType>
ExternalWeightLoader<ValueType>::ExternalWeightLoader(const std::filesystem::path &path)
    : weights(File::Matrix::load_vector<ValueType>(path)), source(path), warned_short(false) {}

template <class ValueType> bool ExternalWeightLoader<ValueType>::operator()(Streamline<ValueType> &streamline) {
  const size_t index = streamline.get_index();
  if (index >= static_cast<size_t>(weights.size())) {
    if (!warned_short) {
      WARN("streamline weights file \"" + source.string() + "\" contains fewer entries (" + str(weights.size()) +
           ") than the tractogram; ceasing reading of streamline data");
      warned_short = true;
    }
    return false;
  }
  streamline.weight = static_cast<float>(weights[static_cast<Eigen::Index>(index)]);
  return true;
}

template <class ValueType> void ExternalWeightLoader<ValueType>::check_excess(const size_t streamline_count) const {
  if (static_cast<size_t>(weights.size()) > streamline_count) {
    WARN("streamline weights file \"" + source.string() + "\" contains more entries (" + str(weights.size()) +
         ") than the tractogram (" + str(streamline_count) + ")");
  }
}

template class ExternalWeightLoader<float>;
template class ExternalWeightLoader<double>;

template <class ValueType>
ExternalWeightExporter<ValueType>::ExternalWeightExporter(const std::filesystem::path &path,
                                                          const size_t initial_streamlines)
    : path(path),
      data(Eigen::Array<ValueType, Eigen::Dynamic, 1>::Zero(
          static_cast<Eigen::Index>(std::max<size_t>(initial_streamlines, 1)))),
      rows(0),
      finalised(false) {}

template <class ValueType> ExternalWeightExporter<ValueType>::~ExternalWeightExporter() {
  try {
    finalise();
  } catch (Exception &e) {
    Exception(e, "Error finalising streamline weights output \"" + path.string() + "\"").display();
  }
}

template <class ValueType> void ExternalWeightExporter<ValueType>::operator()(const Streamline<ValueType> &streamline) {
  grow_to(rows + 1);
  data[static_cast<Eigen::Index>(rows)] = static_cast<ValueType>(streamline.weight);
  ++rows;
}

template <class ValueType> void ExternalWeightExporter<ValueType>::finalise() {
  if (finalised)
    return;
  finalised = true;
  File::Matrix::save_vector(data.head(static_cast<Eigen::Index>(rows)).eval(), path);
}

template <class ValueType> void ExternalWeightExporter<ValueType>::grow_to(const size_t need_rows) {
  size_t capacity = static_cast<size_t>(data.size());
  if (need_rows <= capacity)
    return;
  while (capacity < need_rows)
    capacity = (capacity == 0) ? 1 : capacity * 2;
  data.conservativeResizeLike(Eigen::Array<ValueType, Eigen::Dynamic, 1>::Zero(static_cast<Eigen::Index>(capacity)));
}

template class ExternalWeightExporter<float>;
template class ExternalWeightExporter<double>;

} // namespace MR::DWI::Tractography
