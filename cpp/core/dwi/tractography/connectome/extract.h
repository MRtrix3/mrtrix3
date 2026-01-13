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

#include "file/ofstream.h"

#include "dwi/tractography/connectome/connectome.h"
#include "dwi/tractography/connectome/exemplar.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"

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

  void write(const node_t, const node_t, std::string_view, std::string_view);
  void write(const node_t, std::string_view, std::string_view);
  void write(std::string_view, std::string_view);

private:
  float step_size;
  std::vector<Selector> selectors;
  std::vector<Exemplar> exemplars;
};

class WriterExtraction {

public:
  WriterExtraction(const Tractography::Properties &, const std::vector<node_t> &, const bool, const bool);

  void add(const node_t, std::string_view, const std::string);
  void add(const node_t, const node_t, std::string_view, const std::string);
  void add(const std::vector<node_t> &, std::string_view, const std::string);

  void clear();

  bool operator()(const Connectome::Streamline_nodepair &) const;
  bool operator()(const Connectome::Streamline_nodelist &) const;

  size_t file_count() const { return writers.size(); }

private:
  const Tractography::Properties &properties;
  const std::vector<node_t> &node_list;
  const bool exclusive;
  const bool keep_self;
  std::vector<Selector> selectors;
  std::vector<std::unique_ptr<Tractography::WriterUnbuffered<float>>> writers;
  Tractography::Streamline<> empty_tck;
};

} // namespace MR::DWI::Tractography::Connectome
