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

#include "dwi/tractography/connectome/extract.h"

namespace MR::DWI::Tractography::Connectome {

bool Selector::operator()(const node_t node) const {
  for (std::vector<node_t>::const_iterator i = list.begin(); i != list.end(); ++i) {
    if (*i == node)
      return true;
  }
  return false;
}

bool Selector::operator()(const NodePair &nodes) const {
  if (!keep_self && (nodes.first == nodes.second))
    return false;
  if (exact_match && list.size() == 2)
    return ((nodes.first == list[0] && nodes.second == list[1]) || (nodes.first == list[1] && nodes.second == list[0]));
  bool found_first = false, found_second = false;
  for (std::vector<node_t>::const_iterator i = list.begin(); i != list.end(); ++i) {
    if (*i == nodes.first)
      found_first = true;
    if (*i == nodes.second)
      found_second = true;
  }
  if (exact_match)
    return (found_first && found_second);
  else
    return (found_first || found_second);
}

bool Selector::operator()(const std::vector<node_t> &nodes) const {
  Eigen::Array<bool, Eigen::Dynamic, 1> found(Eigen::Array<bool, Eigen::Dynamic, 1>::Zero(list.size()));
  for (std::vector<node_t>::const_iterator n = nodes.begin(); n != nodes.end(); ++n) {
    for (size_t i = 0; i != list.size(); ++i)
      if (*n == list[i])
        found[i] = true;
  }
  return exact_match ? found.all() : found.any();
}

WriterExemplars::WriterExemplars(const Tractography::Properties &properties,
                                 const std::vector<node_t> &nodes,
                                 const bool exclusive,
                                 const node_t first_node,
                                 const std::vector<Eigen::Vector3d> &COMs)
    : step_size(properties.get_stepsize()) {
  if (!std::isfinite(step_size))
    step_size = 1.0f;

  // Determine how many points to use in the initial representation of each exemplar
  size_t length = 0;
  auto max_dist_it = properties.find("max_dist");
  if (max_dist_it == properties.end())
    length = 201;
  else
    length = std::round(to<float>(max_dist_it->second) / step_size) + 1;

  size_t index = 0;
  if (exclusive) {
    for (size_t i = 0; i != nodes.size(); ++i) {
      const node_t one = nodes[i];
      for (size_t j = i; j != nodes.size(); ++j) {
        const node_t two = nodes[j];
        selectors.push_back(Selector(one, two));
        exemplars.push_back(Exemplar(index++,
                                     length,
                                     std::make_pair(one, two),
                                     std::make_pair(COMs[one].cast<float>(), COMs[two].cast<float>())));
      }
    }
  } else {
    // FIXME Need to generate only unique exemplars - the write functions are then responsible for
    //   determining which exemplars get written to which file
    for (node_t one = first_node; one != COMs.size(); ++one) {
      for (node_t two = one; two != COMs.size(); ++two) {
        if (std::find(nodes.begin(), nodes.end(), one) != nodes.end() ||
            std::find(nodes.begin(), nodes.end(), two) != nodes.end()) {
          selectors.push_back(Selector(one, two));
          exemplars.push_back(Exemplar(index++,
                                       length,
                                       std::make_pair(one, two),
                                       std::make_pair(COMs[one].cast<float>(), COMs[two].cast<float>())));
        }
      }
    }
  }
}

bool WriterExemplars::operator()(const Tractography::Connectome::Streamline_nodepair &in) {
  for (size_t index = 0; index != selectors.size(); ++index) {
    if (selectors[index](in.get_nodes()))
      exemplars[index].add(in);
  }
  return true;
}

bool WriterExemplars::operator()(const Tractography::Connectome::Streamline_nodelist &in) {
  for (size_t index = 0; index != selectors.size(); ++index) {
    if (selectors[index](in.get_nodes()))
      exemplars[index].add(in);
  }
  return true;
}

// TODO Multi-thread
void WriterExemplars::finalize() {
  ProgressBar progress("finalizing exemplars", exemplars.size());
  for (std::vector<Exemplar>::iterator i = exemplars.begin(); i != exemplars.end(); ++i) {
    i->finalize(step_size);
    ++progress;
  }
}

void WriterExemplars::enable_embedding() {
  embed = true;
  weights_ordinal = embed_registry.add({"weights", FieldRole::DPS, DataType::Float32, 1, FieldSource::Internal, 0});
}

Tractography::Tractogram<float> WriterExemplars::make_writer(const std::filesystem::path &path) const {
  Tractography::Properties properties;
  properties["step_size"] = str(step_size);
  // connectome2tck manages its own per-file weights (either the manual stream in
  //   the vertices-only path, or the embedded dps field); no handler auto-detection
  //   of weights exists to suppress.
  return Tractography::Tractogram<float>::create(path,
                                                 properties,
                                                 embed ? embed_registry : Tractography::FieldRegistry(),
                                                 Tractography::AccessRequest::Streaming,
                                                 Formats::WriteOptions{std::nullopt});
}

void WriterExemplars::write_one(Tractography::Tractogram<float> &output, const size_t i) const {
  if (embed) {
    Tractography::TractogramItem<float> item(exemplars[i].get());
    item.dps.resize(embed_registry.dps_count());
    Tractography::ScalarOrVector<float> w(1);
    w(0, 0) = exemplars[i].get_weight();
    item.dps[weights_ordinal] = Tractography::make_dps(std::move(w));
    output.write(item);
  } else {
    output.write(Tractography::TractogramItem<float>(exemplars[i].get()));
  }
}

void WriterExemplars::write(const node_t one,
                            const node_t two,
                            const std::filesystem::path &path,
                            const std::optional<std::filesystem::path> &weights_path = std::nullopt) {
  auto output = make_writer(path);
  for (size_t i = 0; i != exemplars.size(); ++i) {
    if (selectors[i](one, two))
      write_one(output, i);
    else
      output.note_unexported();
  }
  if (weights_path.has_value()) {
    File::OFStream out(weights_path.value());
    for (size_t i = 0; i != exemplars.size(); ++i) {
      if (selectors[i](one, two))
        out << str(exemplars[i].get_weight()) << "\n";
    }
  }
}

void WriterExemplars::write(const node_t node,
                            const std::filesystem::path &path,
                            const std::optional<std::filesystem::path> &weights_path = std::nullopt) {
  auto output = make_writer(path);
  for (size_t i = 0; i != exemplars.size(); ++i) {
    if (selectors[i](node))
      write_one(output, i);
    else
      output.note_unexported();
  }
  if (weights_path.has_value()) {
    File::OFStream out(weights_path.value());
    for (size_t i = 0; i != exemplars.size(); ++i) {
      if (selectors[i](node))
        out << str(exemplars[i].get_weight()) << "\n";
    }
  }
}

void WriterExemplars::write(const std::filesystem::path &path,
                            const std::optional<std::filesystem::path> &weights_path = std::nullopt) {
  auto output = make_writer(path);
  for (size_t i = 0; i != exemplars.size(); ++i)
    write_one(output, i);
  if (weights_path.has_value()) {
    File::OFStream out(weights_path.value());
    for (std::vector<Exemplar>::const_iterator i = exemplars.begin(); i != exemplars.end(); ++i)
      out << str(i->get_weight()) << "\n";
  }
}

WriterExtraction::WriterExtraction(const Tractography::Properties &p,
                                   const std::vector<node_t> &nodes,
                                   const bool exclusive,
                                   const bool keep_self)
    : properties(p), node_list(nodes), exclusive(exclusive), keep_self(keep_self) {}

void WriterExtraction::enable_embedding(const Tractography::FieldRegistry &input_registry) {
  embed = true;
  // Declare the output sidecar field set from the input registry so that any
  //   per-streamline (dps) / per-vertex (dpv) data present on the input items
  //   propagates natively to the embedding-format output (mirroring how
  //   tckconvert's run_generic passes input.fields() to create()).
  for (const FieldDescriptor &descriptor : input_registry)
    embed_registry.add(descriptor);
  // The per-streamline weight is managed explicitly (Streamline::weight, its
  //   single source of truth); add the privileged "weights" dps field unless the
  //   input already carries one, in which case its existing ordinal is reused.
  const std::optional<size_t> existing = embed_registry.ordinal("weights", FieldRole::DPS);
  weights_ordinal =
      existing.has_value()
          ? *existing
          : embed_registry.add({"weights", FieldRole::DPS, DataType::Float32, 1, FieldSource::Internal, 0});
}

Tractography::Tractogram<float> WriterExtraction::make_writer(const std::filesystem::path &path) const {
  return Tractography::Tractogram<float>::create(path,
                                                 properties,
                                                 embed ? embed_registry : Tractography::FieldRegistry(),
                                                 Tractography::AccessRequest::Streaming,
                                                 Formats::WriteOptions{size_t(0)});
}

void WriterExtraction::add(const node_t node,
                           const std::filesystem::path &path,
                           const std::optional<std::filesystem::path> &weights_path) {
  selectors.emplace_back(Selector(node, keep_self));
  writers.push_back(std::make_unique<Tractography::Tractogram<float>>(make_writer(path)));
  if (weights_path.has_value())
    writers.back()->register_weight_output_external(weights_path.value(), 0);
}

void WriterExtraction::add(const node_t node_one,
                           const node_t node_two,
                           const std::filesystem::path &path,
                           const std::optional<std::filesystem::path> &weights_path) {
  if (keep_self || (node_one != node_two)) {
    selectors.emplace_back(Selector(node_one, node_two));
    writers.push_back(std::make_unique<Tractography::Tractogram<float>>(make_writer(path)));
    if (weights_path.has_value())
      writers.back()->register_weight_output_external(weights_path.value(), 0);
  }
}

void WriterExtraction::add(const std::vector<node_t> &list,
                           const std::filesystem::path &path,
                           const std::optional<std::filesystem::path> &weights_path) {
  selectors.emplace_back(Selector(list, exclusive, keep_self));
  writers.push_back(std::make_unique<Tractography::Tractogram<float>>(make_writer(path)));
  if (weights_path.has_value())
    writers.back()->register_weight_output_external(weights_path.value(), 0);
}

void WriterExtraction::clear() {
  selectors.clear();
  writers.clear();
}

void WriterExtraction::write_one(const size_t i, const Tractography::TractogramItem<float> &item) const {
  if (embed) {
    // Embed the per-streamline weight directly as the named "weights" dps field;
    //   the input item's per-vertex (dpv) and other dps payloads ride through
    //   unchanged (the output registry preserves their ordinals).
    Tractography::TractogramItem<float> out(item);
    if (out.dps.size() < embed_registry.dps_count())
      out.dps.resize(embed_registry.dps_count());
    out.dps[weights_ordinal] = Tractography::make_dps_scalar(item.streamline.weight);
    writers[i]->write(out);
    return;
  }
  // Vertices-only output: the per-streamline weight is routed to its separate
  //   file via register_weight_output_external; any input dpv is dropped.
  writers[i]->write(item);
}

void WriterExtraction::skip_one(const size_t i) const { writers[i]->note_unexported(); }

bool WriterExtraction::operator()(const Tractography::TractogramItem<float> &item, const NodePair &nodes) const {
  if (exclusive) {
    // Make sure that both nodes are within the list of nodes of interest;
    //   if not, don't bother passing to any of the selectors
    bool first_in_list = false, second_in_list = false;
    for (std::vector<node_t>::const_iterator i = node_list.begin(); i != node_list.end(); ++i) {
      if (*i == nodes.first)
        first_in_list = true;
      if (*i == nodes.second)
        second_in_list = true;
    }
    if (!first_in_list || !second_in_list) {
      for (size_t i = 0; i != file_count(); ++i)
        skip_one(i);
      return true;
    }
  }
  for (size_t i = 0; i != file_count(); ++i) {
    if (selectors[i](nodes))
      write_one(i, item);
    else
      skip_one(i);
  }
  return true;
}

bool WriterExtraction::operator()(const Tractography::TractogramItem<float> &item,
                                  const std::vector<node_t> &nodes) const {
  if (exclusive) {
    // Make sure _all_ nodes are within the list of nodes of interest;
    //   if not, don't pass to any of the selectors
    Eigen::Array<bool, Eigen::Dynamic, 1> in_list(Eigen::Array<bool, Eigen::Dynamic, 1>::Zero(nodes.size()));
    for (std::vector<node_t>::const_iterator i = node_list.begin(); i != node_list.end(); ++i) {
      for (size_t n = 0; n != nodes.size(); ++n)
        if (*i == nodes[n])
          in_list[n] = true;
    }
    if (!in_list.all()) {
      for (size_t i = 0; i != file_count(); ++i)
        skip_one(i);
      return true;
    }
  }
  for (size_t i = 0; i != file_count(); ++i) {
    if (selectors[i](nodes))
      write_one(i, item);
    else
      skip_one(i);
  }
  return true;
}

} // namespace MR::DWI::Tractography::Connectome
