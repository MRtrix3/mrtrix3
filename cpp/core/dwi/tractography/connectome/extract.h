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
#include <optional>
#include <vector>

#include "file/ofstream.h"

#include "dwi/tractography/connectome/connectome.h"
#include "dwi/tractography/connectome/exemplar.h"
#include "dwi/tractography/field_registry.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/sidecar_value.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/tractogram.h"
#include "dwi/tractography/tractogram_item.h"

namespace MR::DWI::Tractography::Connectome {

class Selector {
public:
  Selector(const node_t node, const bool keep_self = true) : list(1, node), exact_match(false), keep_self(keep_self) {}
  Selector(const node_t node_one, const node_t node_two) : exact_match(true), keep_self(true) {
    list.push_back(node_one);
    list.push_back(node_two);
  }
  Selector(const std::vector<node_t> &node_list, const bool both, const bool keep_self = false)
      : list(node_list), exact_match(both), keep_self(keep_self) {}
  Selector(const Selector &that) : list(that.list), exact_match(that.exact_match), keep_self(that.keep_self) {}
  Selector(Selector &&that) : list(std::move(that.list)), exact_match(that.exact_match), keep_self(that.keep_self) {}

  bool operator()(const node_t) const;
  bool operator()(const NodePair &) const;
  bool operator()(const node_t one, const node_t two) const { return (*this)(NodePair(one, two)); }
  bool operator()(const std::vector<node_t> &) const;

private:
  std::vector<node_t> list;
  bool exact_match, keep_self;
};

class WriterExemplars {
public:
  WriterExemplars(const Tractography::Properties &,
                  const std::vector<node_t> &,
                  const bool,
                  const node_t,
                  const std::vector<Eigen::Vector3d> &);

  bool operator()(const Tractography::Connectome::Streamline_nodepair &);
  bool operator()(const Tractography::Connectome::Streamline_nodelist &);

  void finalize();

  void write(const node_t, const node_t, const std::filesystem::path &, const std::optional<std::filesystem::path> &);
  void write(const node_t, const std::filesystem::path &, const std::optional<std::filesystem::path> &);
  void write(const std::filesystem::path &, const std::optional<std::filesystem::path> &);

  //! \brief embed each exemplar's weight as a "weights" per-streamline (dps) field.
  /*! Used when the output format can serialise sidecar data (e.g. ".trx"): the
   * exemplar weight rides inside the output tractogram rather than being written
   * to a separate -tck_weights_out file. */
  void enable_embedding();

private:
  float step_size;
  std::vector<Selector> selectors;
  std::vector<Exemplar> exemplars;
  bool embed = false;
  FieldRegistry embed_registry;
  size_t weights_ordinal = 0;

  //! \brief create an output tractogram for \a path (embed-aware registry).
  Tractography::Tractogram<float> make_writer(const std::filesystem::path &path) const;
  //! \brief write exemplar \a i to \a output, embedding its weight where enabled.
  void write_one(Tractography::Tractogram<float> &output, const size_t i) const;
};

class WriterExtraction {

public:
  WriterExtraction(const Tractography::Properties &, const std::vector<node_t> &, const bool, const bool);

  void add(const node_t, const std::filesystem::path &, const std::optional<std::filesystem::path> &);
  void add(const node_t, const node_t, const std::filesystem::path &, const std::optional<std::filesystem::path> &);
  void add(const std::vector<node_t> &, const std::filesystem::path &, const std::optional<std::filesystem::path> &);

  void clear();

  //! \brief embed per-streamline weights into the output tractograms.
  /*! Used when the output format can serialise sidecar data (e.g. ".trx"): each
   * streamline's weight rides as a "weights" per-streamline (dps) field instead
   * of being written to a separate -tck_weights_out file. Any per-vertex (dpv)
   * data present on the input items propagates natively to the output (no extra
   * declaration is required here, as it is carried by \a input_registry — see the
   * constructor of the output tractograms). Call once before any add().
   *
   * \param input_registry the read tractogram's field registry, whose dps/dpv
   *   fields are declared on the output so that input sidecar data propagates. */
  void enable_embedding(const Tractography::FieldRegistry &input_registry);

  //! \brief extract an item routed to a node pair, propagating its sidecar payloads.
  /*! Writes the item to every matching output file and skips it in the rest. For
   * an embedding output format the item's per-vertex (dpv) data rides inside the
   * output tractogram; for a vertices-only format it is dropped. */
  bool operator()(const Tractography::TractogramItem<float> &, const NodePair &) const;
  //! \brief extract an item routed to a node list, propagating its sidecar payloads.
  bool operator()(const Tractography::TractogramItem<float> &, const std::vector<node_t> &) const;

  size_t file_count() const { return writers.size(); }

private:
  const Tractography::Properties &properties;
  const std::vector<node_t> &node_list;
  const bool exclusive;
  const bool keep_self;
  bool embed = false;
  FieldRegistry embed_registry;
  size_t weights_ordinal = 0;
  std::vector<Selector> selectors;
  std::vector<std::unique_ptr<Tractography::Tractogram<float>>> writers;

  //! \brief create a per-file output tractogram for the just-added selector.
  /*! The write-back buffer is created with zero capacity, so it grows only as far
   * as the longest streamline encountered and flushes each streamline as it
   * arrives: with one writer open per edge/node this costs a single streamline's
   * memory per file rather than a full buffer. Each file's per-streamline weights
   * are routed explicitly (register_weight_output_external) to a distinct path. */
  Tractography::Tractogram<float> make_writer(const std::filesystem::path &path) const;
  //! \brief write \a item to output file \a i, embedding its weight where enabled.
  void write_one(size_t i, const Tractography::TractogramItem<float> &item) const;
  //! \brief skip output file \a i in the tractogram stream.
  void skip_one(size_t i) const;
};

} // namespace MR::DWI::Tractography::Connectome
