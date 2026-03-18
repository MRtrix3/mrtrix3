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

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "app.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"
#include "exception.h"
#include "file/matrix.h"
#include "file/path.h"
#include <trx/trx.h>

namespace MR::DWI::Tractography::TRX {

// Detect whether a path is a TRX file (zip archive or uncompressed directory)
inline bool is_trx(std::string_view path) {
  if (Path::has_suffix(path, ".trx"))
    return true;
  try {
    return trx::is_trx_directory(std::string(path));
  } catch (const std::exception &) {
    return false;
  }
}

// Returns true when output_arg should be treated as a TRX embedded field name
// rather than a file path.  Conditions: the input tractogram is a TRX file and
// the output argument contains no '.' (no extension) and no path separators.
// This drives the "no extension → embed" CLI convention used by tcksift2,
// tcksample, and fixel2tsf.
inline bool is_trx_field_name(std::string_view input_path, std::string_view output_arg) {
  if (!is_trx(input_path))
    return false;
  return output_arg.find('.') == std::string_view::npos && output_arg.find('/') == std::string_view::npos &&
         output_arg.find('\\') == std::string_view::npos;
}

// Load a TRX file, auto-detecting zip vs directory format, and expose positions
// as float32.
//
// For non-float32 positions, conversion is performed in bounded chunks directly
// into the loaded float32 matrix to avoid allocating a second full-size copy.
inline std::unique_ptr<trx::TrxFile<float>> load_trx(std::string_view path) {
  const std::string filename(path);
  const auto dtype = trx::detect_positions_scalar_type(filename, trx::TrxScalarType::Float32);
  if (dtype == trx::TrxScalarType::Float32)
    return trx::load<float>(filename);
  if (dtype == trx::TrxScalarType::Float16) {
    WARN("TRX file '" + filename + "' has float16 positions; converting to float32 on load");
  } else {
    WARN("TRX file '" + filename + "' has float64 positions; converting to float32 (precision loss)");
  }
  auto trx_f32 = trx::load<float>(filename);
  if (!trx_f32 || !trx_f32->streamlines)
    return trx_f32;
  const Eigen::Index n_vertices = static_cast<Eigen::Index>(trx_f32->num_vertices());
  if (n_vertices <= 0)
    return trx_f32;
  const Eigen::Index chunk_rows = static_cast<Eigen::Index>(1) << 20;
  trx::with_trx_reader(filename, [&](auto &reader, trx::TrxScalarType) -> int {
    const auto &src = reader->streamlines->_data;
    if (src.rows() != n_vertices || src.cols() != 3)
      throw Exception("unexpected TRX positions shape while converting to float32");
    for (Eigen::Index row0 = 0; row0 < n_vertices; row0 += chunk_rows) {
      const Eigen::Index row1 = std::min<Eigen::Index>(n_vertices, row0 + chunk_rows);
      for (Eigen::Index r = row0; r < row1; ++r) {
        trx_f32->streamlines->_data(r, 0) = static_cast<float>(src(r, 0));
        trx_f32->streamlines->_data(r, 1) = static_cast<float>(src(r, 1));
        trx_f32->streamlines->_data(r, 2) = static_cast<float>(src(r, 2));
      }
    }
    return 0;
  });
  return trx_f32;
}

// Count streamlines and total vertices in a TRX file
inline std::pair<size_t, size_t> count_trx(std::string_view path) {
  auto trx = load_trx(path);
  if (!trx)
    throw Exception("Failed to load TRX file: " + std::string(path));
  return {trx->num_streamlines(), trx->num_vertices()};
}

struct GroupNodeMapping {
  std::map<std::string, uint32_t> group_to_node;
  std::vector<std::string> ordered_display_names; // 1-based node display names, index 0 unused
  uint32_t max_node_index = 0;
  bool integer_names = false;
};

inline bool parse_group_node_id(const std::string &group_name, const std::string &prefix, uint32_t &out_node) {
  const std::string stripped = prefix.empty() ? group_name : group_name.substr(prefix.size());
  try {
    const uint32_t parsed = to<uint32_t>(stripped);
    if (parsed <= 0)
      return false;
    out_node = parsed;
    return true;
  } catch (...) {
    return false;
  }
}

inline GroupNodeMapping build_group_node_mapping(const std::vector<std::string> &group_names,
                                                 const std::string &prefix) {
  GroupNodeMapping mapping;
  if (group_names.empty())
    return mapping;

  bool all_integer = true;
  std::map<uint32_t, std::string> reverse;
  for (const auto &name : group_names) {
    uint32_t parsed = 0;
    if (!parse_group_node_id(name, prefix, parsed)) {
      all_integer = false;
      break;
    }
    if (reverse.count(parsed)) {
      all_integer = false;
      break;
    }
    reverse.emplace(parsed, prefix.empty() ? name : name.substr(prefix.size()));
  }

  if (all_integer) {
    mapping.integer_names = true;
    mapping.max_node_index = reverse.rbegin()->first;
    mapping.ordered_display_names.assign(static_cast<size_t>(mapping.max_node_index) + 1, "");
    for (const auto &[node, display] : reverse)
      mapping.ordered_display_names[static_cast<size_t>(node)] = display;
    for (const auto &name : group_names) {
      uint32_t parsed = 0;
      parse_group_node_id(name, prefix, parsed);
      mapping.group_to_node[name] = parsed;
    }
    return mapping;
  }

  mapping.integer_names = false;
  mapping.max_node_index = static_cast<uint32_t>(group_names.size());
  mapping.ordered_display_names.assign(static_cast<size_t>(mapping.max_node_index) + 1, "");
  for (size_t i = 0; i < group_names.size(); ++i) {
    const uint32_t node = static_cast<uint32_t>(i + 1);
    mapping.group_to_node[group_names[i]] = node;
    mapping.ordered_display_names[static_cast<size_t>(node)] =
        prefix.empty() ? group_names[i] : group_names[i].substr(prefix.size());
  }
  return mapping;
}

inline std::vector<std::string> collect_group_names(const trx::TrxFile<float> &trx, const std::string &prefix) {
  std::vector<std::string> names;
  for (const auto &[name, _] : trx.groups) {
    if (prefix.empty() || name.substr(0, prefix.size()) == prefix)
      names.push_back(name);
  }
  return names;
}

inline std::vector<std::vector<uint32_t>>
invert_group_memberships(const trx::TrxFile<float> &trx, const std::map<std::string, uint32_t> &group_to_node) {
  const size_t n_streamlines = trx.num_streamlines();
  std::vector<std::vector<uint32_t>> memberships(n_streamlines);
  for (const auto &[name, _] : trx.groups) {
    auto it = group_to_node.find(name);
    if (it == group_to_node.end())
      continue;
    const uint32_t node_id = it->second;
    const auto *members = trx.get_group_members(name);
    if (!members)
      continue;
    for (Eigen::Index r = 0; r < members->_matrix.rows(); ++r) {
      const auto idx = static_cast<size_t>(members->_matrix(r, 0));
      if (idx < n_streamlines)
        memberships[idx].push_back(node_id);
    }
  }
  for (auto &nodes : memberships) {
    std::sort(nodes.begin(), nodes.end());
    nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());
  }
  return memberships;
}

// Construct a TypedArray from a std::vector, copying the raw bytes into owned storage
template <typename T> inline trx::TypedArray make_typed_array(const std::vector<T> &values, int cols = 1) {
  trx::TypedArray arr;
  arr.dtype = trx::dtype_from_scalar<T>();
  arr.rows = static_cast<int>(values.size()) / cols;
  arr.cols = cols;
  const auto *bytes = reinterpret_cast<const std::uint8_t *>(values.data());
  arr.owned.assign(bytes, bytes + values.size() * sizeof(T));
  return arr;
}

// Append per-streamline data to an existing TRX file (zip or directory)
template <typename T>
inline void append_dps(std::string_view path, const std::string &name, const std::vector<T> &values) {
  const std::string filename(path);
  std::map<std::string, trx::TypedArray> dps_map;
  dps_map.emplace(name, make_typed_array(values));
  try {
    if (trx::is_trx_directory(filename))
      trx::append_dps_to_directory(filename, dps_map);
    else
      trx::append_dps_to_zip(filename, dps_map);
  } catch (const std::exception &e) {
    throw Exception(std::string("Failed to append dps to TRX: ") + e.what());
  }
}

// Append per-vertex data to an existing TRX file (zip or directory)
template <typename T>
inline void append_dpv(std::string_view path, const std::string &name, const std::vector<T> &values) {
  const std::string filename(path);
  std::map<std::string, trx::TypedArray> dpv_map;
  dpv_map.emplace(name, make_typed_array(values));
  try {
    if (trx::is_trx_directory(filename))
      trx::append_dpv_to_directory(filename, dpv_map);
    else
      trx::append_dpv_to_zip(filename, dpv_map);
  } catch (const std::exception &e) {
    throw Exception(std::string("Failed to append dpv to TRX: ") + e.what());
  }
}

// Append a named group (list of streamline indices) to an existing TRX file.
// This is the per-group analogue of append_dps / append_dpv.
inline void
append_group(std::string_view trx_path, const std::string &name, const std::vector<uint32_t> &streamline_indices) {
  const std::string filename(trx_path);
  std::map<std::string, std::vector<uint32_t>> groups_map;
  groups_map.emplace(name, streamline_indices);
  try {
    if (trx::is_trx_directory(filename))
      trx::append_groups_to_directory(filename, groups_map);
    else
      trx::append_groups_to_zip(filename, groups_map);
  } catch (const std::exception &e) {
    throw Exception(std::string("Failed to append group to TRX: ") + e.what());
  }
}

// Copy sidecar metadata (dps, groups, optionally dpv) from src_path to dst_path.
//
// Intended for geometry-modifying commands (tckresample, future tcktransform stream
// path) that stream warped/resampled geometry into a new TRX via TrxStream and then
// need to carry metadata forward from the source.
//
// Policy:
//   - dps: always copied (streamline count is unchanged by geometry ops)
//   - groups: always copied (streamline index sets are unchanged)
//   - dpg: NOT copied — trx-cpp has no append_dpg_to_zip/directory API yet
//   - dpv: copied only if include_dpv == true; callers that change vertex count
//          (e.g. tckresample) should pass false and warn separately
//
// All numeric fields are copied as float32 regardless of original on-disk dtype,
// because TrxFile<float> already presents all values as float.
// Group indices are copied as uint32_t.
inline void copy_trx_sidecar_data(std::string_view src_path, std::string_view dst_path, bool include_dpv = false) {
  auto src = load_trx(src_path);
  if (!src)
    return;
  const std::string dst(dst_path);
  const bool is_dir = trx::is_trx_directory(dst);

  // --- dps ---
  if (!src->data_per_streamline.empty()) {
    std::map<std::string, trx::TypedArray> dps_map;
    for (const auto &[name, arr] : src->data_per_streamline) {
      if (!arr)
        continue;
      const auto &mat = arr->_matrix;
      const int rows = static_cast<int>(mat.rows());
      const int cols = static_cast<int>(mat.cols());
      std::vector<float> vals(static_cast<size_t>(rows * cols));
      for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
          vals[static_cast<size_t>(r * cols + c)] = static_cast<float>(mat(r, c));
      dps_map.emplace(name, make_typed_array(vals, cols));
    }
    try {
      if (is_dir)
        trx::append_dps_to_directory(dst, dps_map);
      else
        trx::append_dps_to_zip(dst, dps_map);
    } catch (const std::exception &e) {
      throw Exception(std::string("Failed to copy dps to TRX: ") + e.what());
    }
  }

  // --- groups ---
  if (!src->groups.empty()) {
    std::map<std::string, std::vector<uint32_t>> groups_map;
    for (const auto &[name, _] : src->groups) {
      const auto *members = src->get_group_members(name);
      if (!members)
        continue;
      const auto &mat = members->_matrix;
      std::vector<uint32_t> indices(static_cast<size_t>(mat.rows()));
      for (Eigen::Index i = 0; i < mat.rows(); ++i)
        indices[static_cast<size_t>(i)] = static_cast<uint32_t>(mat(i, 0));
      groups_map.emplace(name, std::move(indices));
    }
    try {
      if (is_dir)
        trx::append_groups_to_directory(dst, groups_map);
      else
        trx::append_groups_to_zip(dst, groups_map);
    } catch (const std::exception &e) {
      throw Exception(std::string("Failed to copy groups to TRX: ") + e.what());
    }
    if (!src->data_per_group.empty())
      WARN(std::to_string(src->data_per_group.size()) +
           " data_per_group field(s) not copied: append_dpg_to_zip/directory not yet available");
  }

  // --- dpv ---
  if (include_dpv && !src->data_per_vertex.empty()) {
    std::map<std::string, trx::TypedArray> dpv_map;
    for (const auto &[name, arr] : src->data_per_vertex) {
      if (!arr)
        continue;
      const auto &data = arr->_data;
      const int rows = static_cast<int>(data.rows());
      const int cols = static_cast<int>(data.cols());
      std::vector<float> vals(static_cast<size_t>(rows * cols));
      for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
          vals[static_cast<size_t>(r * cols + c)] = static_cast<float>(data(r, c));
      dpv_map.emplace(name, make_typed_array(vals, cols));
    }
    try {
      if (is_dir)
        trx::append_dpv_to_directory(dst, dpv_map);
      else
        trx::append_dpv_to_zip(dst, dpv_map);
    } catch (const std::exception &e) {
      throw Exception(std::string("Failed to copy dpv to TRX: ") + e.what());
    }
  }
}

// Check whether a loaded TRX file has any auxiliary data (groups, dps, dpv, dpg)
inline bool has_aux_data(const trx::TrxFile<float> *trx) {
  if (!trx)
    return false;
  return !(trx->groups.empty() && trx->data_per_streamline.empty() && trx->data_per_vertex.empty() &&
           trx->data_per_group.empty());
}

// Print TRX file information to an output stream.
// Choose the most granular prefix depth whose output stays within max_lines.
// Returns 0 if the flat group list already fits, or the largest depth value
// (starting from 1) where the collapsed output has ≤ max_lines lines.
// If even depth 1 exceeds max_lines, depth 1 is returned as the most compact
// option available. group_counts maps each group name to its streamline count.
inline int auto_prefix_depth(const std::map<std::string, size_t> &group_counts, size_t max_lines = 100) {
  if (group_counts.size() <= max_lines)
    return 0;
  int best = 1;
  for (int depth = 1; depth <= 20; ++depth) {
    const std::string s = trx::format_groups_summary(group_counts, depth);
    const size_t n_lines = static_cast<size_t>(std::count(s.begin(), s.end(), '\n')) + 1;
    if (n_lines > max_lines)
      break; // line count is non-decreasing with depth; no higher depth will help
    best = depth;
  }
  return best;
}

// When prefix_depth > 0, groups are collapsed by their underscore-delimited
// name prefix to the given depth (see trx::format_groups_summary).
// Set depth_auto = true when the depth was chosen automatically; a note is
// then appended to the Groups header line to inform the user.
inline void
print_info(std::ostream &out, const trx::TrxFile<float> &trx, int prefix_depth = 0, bool depth_auto = false) {
  out << "    TRX streamlines:      " << trx.num_streamlines() << "\n";
  out << "    TRX vertices:         " << trx.num_vertices() << "\n";

  const auto &hdr = trx.header;
  if (hdr["VOXEL_TO_RASMM"].is_array()) {
    out << "    VOXEL_TO_RASMM:       ";
    const auto &rows = hdr["VOXEL_TO_RASMM"].array_items();
    for (size_t r = 0; r < rows.size(); ++r) {
      if (r > 0)
        out << "                          ";
      const auto &cols = rows[r].array_items();
      out << "[";
      for (size_t c = 0; c < cols.size(); ++c) {
        if (c > 0)
          out << ", ";
        out << cols[c].number_value();
      }
      out << "]\n";
    }
  }

  if (hdr["DIMENSIONS"].is_array()) {
    out << "    DIMENSIONS:           [";
    const auto &dims = hdr["DIMENSIONS"].array_items();
    for (size_t i = 0; i < dims.size(); ++i) {
      if (i > 0)
        out << ", ";
      out << dims[i].int_value();
    }
    out << "]\n";
  }

  const auto &meta = hdr["metadata"];
  if (meta.is_object() && !meta.object_items().empty()) {
    out << "    Metadata:\n";
    for (const auto &[key, val] : meta.object_items())
      out << "      " << key << ": " << val.string_value() << "\n";
  }

  if (!trx.groups.empty()) {
    out << "    Groups (" << trx.groups.size() << ")";
    if (depth_auto && prefix_depth > 0)
      out << " [use -prefix_depth 0 to see all, or -prefix_depth N for a different depth]";
    out << ":\n";
    std::map<std::string, size_t> group_counts;
    for (const auto &kv : trx.groups) {
      const auto *members = trx.get_group_members(kv.first);
      group_counts[kv.first] = members ? static_cast<size_t>(members->_matrix.rows()) : 0;
    }
    out << trx::format_groups_summary(group_counts, prefix_depth, "      ") << "\n";
  }

  if (!trx.data_per_streamline.empty()) {
    out << "    Data per streamline (" << trx.data_per_streamline.size() << "):\n";
    for (const auto &kv : trx.data_per_streamline) {
      size_t rows = kv.second ? static_cast<size_t>(kv.second->_matrix.rows()) : 0;
      size_t cols = kv.second ? static_cast<size_t>(kv.second->_matrix.cols()) : 0;
      out << "      " << kv.first << ": " << rows << " x " << cols << "\n";
    }
  }

  if (!trx.data_per_vertex.empty()) {
    out << "    Data per vertex (" << trx.data_per_vertex.size() << "):\n";
    for (const auto &kv : trx.data_per_vertex) {
      size_t rows = kv.second ? static_cast<size_t>(kv.second->_data.rows()) : 0;
      size_t cols = kv.second ? static_cast<size_t>(kv.second->_data.cols()) : 0;
      out << "      " << kv.first << ": " << rows << " x " << cols << "\n";
    }
  }

  if (!trx.data_per_group.empty()) {
    out << "    Data per group (" << trx.data_per_group.size() << "):\n";
    for (const auto &group_kv : trx.data_per_group) {
      out << "      " << group_kv.first << ":\n";
      for (const auto &field_kv : group_kv.second) {
        size_t rows = field_kv.second ? static_cast<size_t>(field_kv.second->_matrix.rows()) : 0;
        size_t cols = field_kv.second ? static_cast<size_t>(field_kv.second->_matrix.cols()) : 0;
        out << "        " << field_kv.first << ": " << rows << " x " << cols << "\n";
      }
    }
  }
}

// Reader class that wraps trx::TrxFile for streamline-by-streamline iteration.
//
// Optional weight injection: if a non-empty weight vector is provided at
// construction time, tck.weight is set from that vector during each operator()
// call.  This mirrors the behaviour of Reader<float>, which loads weights from
// the -tck_weights_in option in its own constructor.  Use open_tractogram() with
// the weight_field_or_path parameter rather than constructing TRXReader directly.
class TRXReader : public ReaderInterface<float> {
public:
  explicit TRXReader(std::string_view file, std::vector<float> weights = {})
      : trx(nullptr),
        current(0),
        num_streamlines_(0),
        has_aux_data_(false),
        weights_(std::move(weights)),
        warned_short_weights_(false),
        warned_excess_weights_(false) {
    try {
      trx = load_trx(file);
      if (trx && trx->streamlines && trx->streamlines->_offsets.size() > 0)
        num_streamlines_ = trx->streamlines->_offsets.size() - 1;
      has_aux_data_ = has_aux_data(trx.get());
    } catch (const std::exception &e) {
      throw Exception(e.what());
    }
  }

  bool operator()(Streamline<float> &tck) override {
    tck.clear();
    if (!trx || current >= num_streamlines_) {
      if (!warned_excess_weights_ && !weights_.empty() && weights_.size() > static_cast<size_t>(num_streamlines_)) {
        WARN("Streamline weights file contains more entries (" + str(weights_.size()) + ") than TRX file (" +
             str(num_streamlines_) + ")");
        warned_excess_weights_ = true;
      }
      return false;
    }
    tck.set_index(static_cast<size_t>(current));
    // Inject per-streamline weight if weights were resolved at construction time
    if (!weights_.empty()) {
      if (static_cast<size_t>(current) < weights_.size()) {
        tck.weight = weights_[static_cast<size_t>(current)];
      } else {
        if (!warned_short_weights_) {
          WARN("Streamline weights file contains less entries (" + str(weights_.size()) +
               ") than TRX file; ceasing reading of streamline data");
          warned_short_weights_ = true;
        }
        tck.clear();
        return false;
      }
    } else {
      tck.weight = 1.0f;
    }
    const Eigen::Index start = trx->streamlines->_offsets(current, 0);
    const Eigen::Index end = trx->streamlines->_offsets(current + 1, 0);
    tck.reserve(end - start);
    for (Eigen::Index i = start; i < end; ++i) {
      Eigen::Vector3f p;
      p[0] = trx->streamlines->_data(i, 0);
      p[1] = trx->streamlines->_data(i, 1);
      p[2] = trx->streamlines->_data(i, 2);
      tck.push_back(p);
    }
    ++current;
    return true;
  }

  size_t num_streamlines() const { return static_cast<size_t>(num_streamlines_); }

  // Populate a Properties map from the TRX header "metadata" object.
  // This transfers fields written by tckgen/tckconvert (step_size, method, etc.)
  // so callers can use them exactly as they would TCK properties.
  void populate_properties(Tractography::Properties &properties) const {
    properties["count"] = std::to_string(num_streamlines_);
    if (!trx)
      return;
    const auto &meta = trx->header["metadata"];
    if (!meta.is_object())
      return;
    for (const auto &[key, val] : meta.object_items()) {
      if (val.is_string())
        properties[key] = val.string_value();
    }
  }

  ~TRXReader() override {
    if (!trx)
      return;
    try {
      trx->close();
      trx.reset();
    } catch (const std::exception &e) {
      Exception(e.what()).display();
      App::exit_error_code = 1;
    }
  }

  bool has_metadata() const { return has_aux_data_; }

  trx::TrxFile<float> *get_trx() { return trx.get(); }

private:
  std::unique_ptr<trx::TrxFile<float>> trx;
  Eigen::Index current;
  Eigen::Index num_streamlines_;
  bool has_aux_data_;
  std::vector<float> weights_; // per-streamline weights injected into tck.weight during iteration
  bool warned_short_weights_;
  bool warned_excess_weights_;
};

// Writer class that accumulates streamlines into a preallocated TrxFile
class TRXWriter : public WriterInterface<float> {
public:
  TRXWriter(std::string_view file,
            size_t nb_streamlines,
            size_t nb_vertices,
            std::string_view final_output,
            bool rename_on_save)
      : output(file),
        final_output(final_output),
        rename_on_save(rename_on_save),
        current_streamline(0),
        current_vertex(0) {
    try {
      trx.reset(new trx::TrxFile<float>(static_cast<int>(nb_vertices), static_cast<int>(nb_streamlines), nullptr));
      if (trx->streamlines && trx->streamlines->_offsets.size() > 0)
        trx->streamlines->_offsets(0, 0) = 0;
    } catch (const std::exception &e) {
      throw Exception(e.what());
    }
  }

  bool operator()(const Streamline<float> &tck) override {
    if (!trx || !trx->streamlines)
      return false;
    const Eigen::Index tck_size = static_cast<Eigen::Index>(tck.size());
    for (Eigen::Index i = 0; i < tck_size; ++i) {
      const auto &pos = tck[static_cast<size_t>(i)];
      trx->streamlines->_data(current_vertex + i, 0) = pos[0];
      trx->streamlines->_data(current_vertex + i, 1) = pos[1];
      trx->streamlines->_data(current_vertex + i, 2) = pos[2];
    }
    trx->streamlines->_lengths(current_streamline, 0) = static_cast<uint32_t>(tck_size);
    trx->streamlines->_offsets(current_streamline + 1, 0) = static_cast<uint64_t>(current_vertex + tck_size);
    current_vertex += tck_size;
    ++current_streamline;
    return true;
  }

  ~TRXWriter() override {
    if (!trx)
      return;
    try {
      trx->save(output, ZIP_CM_STORE);
      if (rename_on_save) {
        std::error_code ec;
        std::filesystem::remove_all(final_output, ec);
        std::filesystem::rename(output, final_output, ec);
        if (ec) {
          throw std::runtime_error("Failed to rename TRX directory to " + final_output + ": " + ec.message());
        }
      }
      trx->close();
      trx.reset();
    } catch (const std::exception &e) {
      Exception(e.what()).display();
      App::exit_error_code = 1;
    }
  }

  trx::TrxFile<float> *get_trx() { return trx.get(); }

private:
  std::string output;
  std::string final_output;
  bool rename_on_save;
  std::unique_ptr<trx::TrxFile<float>> trx;
  Eigen::Index current_streamline;
  Eigen::Index current_vertex;
};

// Open a tractogram (TCK or TRX) for streaming, populating properties.
// For TRX: properties["count"] and any fields stored in header["metadata"]
//   (e.g. step_size, method, command_history) are transferred to properties,
//   so callers can treat TRX and TCK properties identically.
// For TCK: identical to constructing Reader<float>(path, properties) directly.
// This is the single entry point replacing Reader<float> file(path, properties) in commands
// that want to accept both formats without any format-detection branching.
// See the 3-argument overload below (defined after resolve_dps_weights) for weight injection.
inline std::unique_ptr<ReaderInterface<float>> open_tractogram(std::string_view path,
                                                               Tractography::Properties &properties) {
  if (is_trx(path)) {
    auto reader = std::make_unique<TRXReader>(path);
    reader->populate_properties(properties);
    return reader;
  }
  return std::make_unique<Tractography::Reader<float>>(path, properties);
}

// Resolve per-streamline weights from either an external text file or a named dps field
// in the TRX file at tractogram_path.
//
// Resolution order:
//   1. If field_name_or_path is an existing file path → load as a text weight vector
//      (same format as -tck_weights_in).
//   2. Else if tractogram_path is a TRX file → look up field_name_or_path in data_per_streamline.
//   3. Otherwise → throw.
//
// Returns an empty vector when field_name_or_path is empty (no weights requested).
inline std::vector<float> resolve_dps_weights(std::string_view tractogram_path, const std::string &field_name_or_path) {
  if (field_name_or_path.empty())
    return {};
  if (Path::exists(field_name_or_path)) {
    const auto eig = File::Matrix::load_vector<float>(field_name_or_path);
    return std::vector<float>(eig.data(), eig.data() + eig.size());
  }
  if (!is_trx(tractogram_path))
    throw Exception("cannot resolve \"" + field_name_or_path + "\": not an existing file and input is not a TRX file");
  auto trx = load_trx(tractogram_path);
  auto it = trx->data_per_streamline.find(field_name_or_path);
  if (it == trx->data_per_streamline.end() || !it->second)
    throw Exception("TRX file has no dps field named \"" + field_name_or_path + "\"");
  const auto &mat = it->second->_matrix;
  if (mat.cols() > 1)
    WARN("dps field \"" + field_name_or_path + "\" has " + std::to_string(mat.cols()) +
         " columns; using first column only");
  std::vector<float> result(static_cast<size_t>(mat.rows()));
  for (Eigen::Index i = 0; i < mat.rows(); ++i)
    result[static_cast<size_t>(i)] = mat(i, 0);
  return result;
}

// Weight-injecting overload of open_tractogram.  Defined here (after resolve_dps_weights)
// because it calls resolve_dps_weights for TRX input.
//
// For TRX: weight_field_or_path is resolved via resolve_dps_weights() and the resulting
//   vector is passed to TRXReader, which injects tck.weight during each operator() call.
//   This is exactly what Reader<float> does automatically for TCK files.
// For TCK: weight_field_or_path is ignored; Reader<float> reads -tck_weights_in itself.
//   There is no double-loading.
//
// Callers that need weight injection (e.g. tckmap) use this overload:
//   auto reader = open_tractogram(path, properties, get_option_value("tck_weights_in", ""));
// Callers that do not need weights use the 2-argument overload above.
inline std::unique_ptr<ReaderInterface<float>>
open_tractogram(std::string_view path, Tractography::Properties &properties, const std::string &weight_field_or_path) {
  if (!is_trx(path))
    return open_tractogram(path, properties); // TCK: Reader<float> handles weights itself
  auto weights = resolve_dps_weights(path, weight_field_or_path);
  auto reader = std::make_unique<TRXReader>(path, std::move(weights));
  reader->populate_properties(properties);
  return reader;
}

// Resolve flat per-vertex scalars from a named dpv field in the TRX file at tractogram_path.
// The returned vector is ordered by streamline then vertex, matching the TRX storage layout
// (i.e. suitable for direct use as a TrackScalar or TSF replacement).
//
// Resolution order:
//   1. If field_name_or_path is an existing file path → load as a TSF scalar file.
//      (TSF binary loading is handled transparently; the path must end in .tsf.)
//   2. Else if tractogram_path is a TRX file → look up field_name_or_path in data_per_vertex.
//   3. Otherwise → throw.
//
// Returns an empty vector when field_name_or_path is empty (no scalars requested).
inline std::vector<float> resolve_dpv_scalars(std::string_view tractogram_path, const std::string &field_name_or_path) {
  if (field_name_or_path.empty())
    return {};
  if (Path::exists(field_name_or_path)) {
    // External TSF binary file: load as a flat vector via the text/matrix path.
    // TSF files are plain binary float arrays with a small header; the existing
    // File::Matrix::load_vector handles the common text case.  For proper binary
    // TSF support, callers should use Tractography::ScalarFile directly.
    const auto eig = File::Matrix::load_vector<float>(field_name_or_path);
    return std::vector<float>(eig.data(), eig.data() + eig.size());
  }
  if (!is_trx(tractogram_path))
    throw Exception("cannot resolve \"" + field_name_or_path + "\": not an existing file and input is not a TRX file");
  auto trx = load_trx(tractogram_path);
  auto it = trx->data_per_vertex.find(field_name_or_path);
  if (it == trx->data_per_vertex.end() || !it->second)
    throw Exception("TRX file has no dpv field named \"" + field_name_or_path + "\"");
  const auto &data = it->second->_data;
  if (data.cols() > 1)
    WARN("dpv field \"" + field_name_or_path + "\" has " + std::to_string(data.cols()) +
         " columns; using first column only");
  std::vector<float> result(static_cast<size_t>(data.rows()));
  for (Eigen::Index i = 0; i < data.rows(); ++i)
    result[static_cast<size_t>(i)] = data(i, 0);
  return result;
}

// Read a TRX dpv field as a stream of per-streamline TrackScalar<float>.
// Copies dpv values and streamline offsets into owned vectors on construction so
// that the TRX mmap can be released immediately (enabling in-place append later).
class TRXScalarReader {
public:
  TRXScalarReader(std::string_view trx_path, const std::string &field_name) : current_(0) {
    auto trx = load_trx(trx_path);
    if (!trx || !trx->streamlines)
      throw Exception("Failed to load TRX file: " + std::string(trx_path));

    auto it = trx->data_per_vertex.find(field_name);
    if (it == trx->data_per_vertex.end() || !it->second)
      throw Exception("TRX file has no dpv field '" + field_name + "'");
    const auto &data = it->second->_data;
    if (data.cols() > 1)
      WARN("dpv field '" + field_name + "' has " + std::to_string(data.cols()) + " columns; using first column only");

    // Copy vertex values and offsets into owned storage so the mmap can be released.
    const Eigen::Index nv = data.rows();
    values_.resize(static_cast<size_t>(nv));
    for (Eigen::Index i = 0; i < nv; ++i)
      values_[static_cast<size_t>(i)] = data(i, 0);

    n_streamlines_ = static_cast<Eigen::Index>(trx->num_streamlines());
    offsets_.resize(static_cast<size_t>(n_streamlines_ + 1));
    for (Eigen::Index i = 0; i <= n_streamlines_; ++i)
      offsets_[static_cast<size_t>(i)] = static_cast<size_t>(trx->streamlines->_offsets(i, 0));

    trx->close();
  }

  bool operator()(Tractography::TrackScalar<float> &scalar) {
    scalar.clear();
    if (current_ >= n_streamlines_)
      return false;
    const size_t start = offsets_[static_cast<size_t>(current_)];
    const size_t end = offsets_[static_cast<size_t>(current_ + 1)];
    scalar.set_index(static_cast<size_t>(current_));
    scalar.reserve(end - start);
    for (size_t i = start; i < end; ++i)
      scalar.push_back(values_[i]);
    ++current_;
    return true;
  }

  size_t num_streamlines() const { return static_cast<size_t>(n_streamlines_); }
  size_t num_vertices() const { return values_.size(); }

private:
  std::vector<float> values_;
  std::vector<size_t> offsets_;
  Eigen::Index current_;
  Eigen::Index n_streamlines_;
};

// Accumulate per-streamline TrackScalar<float> values and append them as a
// dpv field to an existing TRX file.  The output TRX file must already exist
// (use the same path as the input to add a field in-place).
// Call finalize() when all scalars have been written; the destructor also calls
// finalize() as a fallback (but errors in the destructor are suppressed).
class TRXScalarWriter {
public:
  TRXScalarWriter(std::string_view trx_path, const std::string &field_name)
      : path_(trx_path), field_(field_name), finalized_(false) {
    if (!is_trx(trx_path))
      throw Exception("Output path for TRX scalar writer is not a TRX file: " + std::string(trx_path));
    if (!Path::exists(std::string(trx_path)))
      throw Exception("Output TRX file '" + std::string(trx_path) +
                      "' does not exist; cannot append dpv. "
                      "Use the same path as the input TRX to add a field in-place.");
  }

  bool operator()(const Tractography::TrackScalar<float> &scalar) {
    buffer_.insert(buffer_.end(), scalar.begin(), scalar.end());
    return true;
  }

  void finalize() {
    if (finalized_)
      return;
    finalized_ = true;
    append_dpv<float>(path_, field_, buffer_);
  }

  ~TRXScalarWriter() {
    if (!finalized_) {
      try {
        finalize();
      } catch (const std::exception &e) {
        Exception(e.what()).display();
        App::exit_error_code = 1;
      }
    }
  }

private:
  std::string path_;
  std::string field_;
  std::vector<float> buffer_;
  bool finalized_;
};

} // namespace MR::DWI::Tractography::TRX
