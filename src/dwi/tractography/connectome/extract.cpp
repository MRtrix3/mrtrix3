/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "dwi/tractography/connectome/extract.h"

#include "misc/bitset.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectome {



bool Selector::operator() (const node_t node) const
{
  for (vector<node_t>::const_iterator i = list.begin(); i != list.end(); ++i) {
    if (*i == node)
      return true;
  }
  return false;
}



bool Selector::operator() (const NodePair& nodes) const
{
  if (!keep_self && (nodes.first == nodes.second))
    return false;
  if (exact_match && list.size() == 2)
    return ((nodes.first == list[0] && nodes.second == list[1]) || (nodes.first == list[1] && nodes.second == list[0]));
  bool found_first = false, found_second = false;
  for (vector<node_t>::const_iterator i = list.begin(); i != list.end(); ++i) {
    if (*i == nodes.first)  found_first = true;
    if (*i == nodes.second) found_second = true;
  }
  if (exact_match)
    return (found_first && found_second);
  else
    return (found_first || found_second);
}

bool Selector::operator() (const vector<node_t>& nodes) const
{
  BitSet found (list.size());
  for (vector<node_t>::const_iterator n = nodes.begin(); n != nodes.end(); ++n) {
    for (size_t i = 0; i != list.size(); ++i)
      if (*n == list[i]) found[i] = true;
  }
  if (exact_match)
    return found.full();
  else
    return !found.empty();
}








WriterExemplars::WriterExemplars (const Tractography::Properties& properties, const vector<node_t>& nodes, const bool exclusive, const node_t first_node, const vector<Eigen::Vector3f>& COMs) :
    step_size (NAN)
{
  // Determine how many points to use in the initial representation of each exemplar
  size_t length = 0;
  auto output_step_size_it = properties.find ("output_step_size");
  if (output_step_size_it == properties.end()) {
    auto step_size_it = properties.find ("step_size");
    if (step_size_it == properties.end())
      step_size = 1.0f;
    else
      step_size = to<float>(step_size_it->second);
  } else {
    step_size = to<float>(output_step_size_it->second);
  }

  auto max_dist_it = properties.find ("max_dist");
  if (max_dist_it == properties.end())
    length = 201;
  else
    length = std::round (to<float>(max_dist_it->second) / step_size) + 1;

  if (exclusive) {
    for (size_t i = 0; i != nodes.size(); ++i) {
      const node_t one = nodes[i];
      for (size_t j = i; j != nodes.size(); ++j) {
        const node_t two = nodes[j];
        selectors.push_back (Selector (one, two));
        exemplars.push_back (Exemplar (length, std::make_pair (one, two), std::make_pair (COMs[one], COMs[two])));
      }
    }
  } else {
    // FIXME Need to generate only unique exemplars - the write functions are then responsible for
    //   determining which exemplars get written to which file
    for (node_t one = first_node; one != COMs.size(); ++one) {
      for (node_t two = one; two != COMs.size(); ++two) {
        if (std::find (nodes.begin(), nodes.end(), one) != nodes.end() || std::find (nodes.begin(), nodes.end(), two) != nodes.end()) {
          selectors.push_back (Selector (one, two));
          exemplars.push_back (Exemplar (length, std::make_pair (one, two), std::make_pair (COMs[one], COMs[two])));
        }
      }
    }
  }
}



bool WriterExemplars::operator() (const Tractography::Connectome::Streamline_nodepair& in)
{
  for (size_t index = 0; index != selectors.size(); ++index) {
    if (selectors[index] (in.get_nodes()))
      exemplars[index].add (in);
  }
  return true;
}

bool WriterExemplars::operator() (const Tractography::Connectome::Streamline_nodelist& in)
{
  for (size_t index = 0; index != selectors.size(); ++index) {
    if (selectors[index] (in.get_nodes()))
      exemplars[index].add (in);
  }
  return true;
}



// TODO Multi-thread
void WriterExemplars::finalize()
{
  ProgressBar progress ("finalizing exemplars", exemplars.size());
  for (vector<Exemplar>::iterator i = exemplars.begin(); i != exemplars.end(); ++i) {
    i->finalize (step_size);
    ++progress;
  }
}



void WriterExemplars::write (const node_t one, const node_t two, const std::string& path, const std::string& weights_path)
{
  Tractography::Properties properties;
  properties["step_size"] = str(step_size);
  Tractography::WriterUnbuffered<float> writer (path, properties);
  for (size_t i = 0; i != exemplars.size(); ++i) {
    if (selectors[i] (one, two))
      writer (exemplars[i].get());
  }
  if (weights_path.size()) {
    File::OFStream output (weights_path);
    for (size_t i = 0; i != exemplars.size(); ++i) {
      if (selectors[i] (one, two))
        output << str(exemplars[i].get_weight()) << "\n";
    }
  }
}

void WriterExemplars::write (const node_t node, const std::string& path, const std::string& weights_path)
{
  Tractography::Properties properties;
  properties["step_size"] = str(step_size);
  Tractography::Writer<float> writer (path, properties);
  for (size_t i = 0; i != exemplars.size(); ++i) {
    if (selectors[i] (node))
      writer (exemplars[i].get());
  }
  if (weights_path.size()) {
    File::OFStream output (weights_path);
    for (size_t i = 0; i != exemplars.size(); ++i) {
      if (selectors[i] (node))
        output << str(exemplars[i].get_weight()) << "\n";
    }
  }
}

void WriterExemplars::write (const std::string& path, const std::string& weights_path)
{
  Tractography::Properties properties;
  properties["step_size"] = str(step_size);
  Tractography::Writer<float> writer (path, properties);
  for (vector<Exemplar>::const_iterator i = exemplars.begin(); i != exemplars.end(); ++i)
    writer (i->get());
  if (weights_path.size()) {
    File::OFStream output (weights_path);
    for (vector<Exemplar>::const_iterator i = exemplars.begin(); i != exemplars.end(); ++i)
      output << str(i->get_weight()) << "\n";
  }
}










WriterExtraction::WriterExtraction (const Tractography::Properties& p, const vector<node_t>& nodes, const bool exclusive, const bool keep_self) :
    properties (p),
    node_list (nodes),
    exclusive (exclusive),
    keep_self (keep_self) { }

WriterExtraction::~WriterExtraction()
{
  for (size_t i = 0; i != writers.size(); ++i) {
    delete writers[i];
    writers[i] = nullptr;
  }
}




void WriterExtraction::add (const node_t node, const std::string& path, const std::string weights_path = "")
{
  selectors.push_back (Selector (node, keep_self));
  writers.push_back (new Tractography::WriterUnbuffered<float> (path, properties));
  if (weights_path.size())
    writers.back()->set_weights_path (weights_path);
}

void WriterExtraction::add (const node_t node_one, const node_t node_two, const std::string& path, const std::string weights_path = "")
{
  if (keep_self || (node_one != node_two)) {
    selectors.push_back (Selector (node_one, node_two));
    writers.push_back (new Tractography::WriterUnbuffered<float> (path, properties));
    if (weights_path.size())
      writers.back()->set_weights_path (weights_path);
  }
}

void WriterExtraction::add (const vector<node_t>& list, const std::string& path, const std::string weights_path = "")
{
  selectors.push_back (Selector (list, exclusive, keep_self));
  writers.push_back (new Tractography::WriterUnbuffered<float> (path, properties));
  if (weights_path.size())
    writers.back()->set_weights_path (weights_path);
}



void WriterExtraction::clear()
{
  selectors.clear();
  for (size_t i = 0; i != writers.size(); ++i) {
    delete writers[i];
    writers[i] = NULL;
  }
  writers.clear();
}



bool WriterExtraction::operator() (const Connectome::Streamline_nodepair& in) const
{
  if (exclusive) {
    // Make sure that both nodes are within the list of nodes of interest;
    //   if not, don't pass to any of the selectors
    bool first_in_list = false, second_in_list = false;
    for (vector<node_t>::const_iterator i = node_list.begin(); i != node_list.end(); ++i) {
      if (*i == in.get_nodes().first)  first_in_list = true;
      if (*i == in.get_nodes().second) second_in_list = true;
    }
    if (!first_in_list || !second_in_list) return true;
  }
  for (size_t i = 0; i != file_count(); ++i) {
    if (selectors[i] (in.get_nodes()))
      (*writers[i]) (in);
    else
      (*writers[i]) (empty_tck);
  }
  return true;
}

bool WriterExtraction::operator() (const Connectome::Streamline_nodelist& in) const
{
  if (exclusive) {
    // Make sure _all_ nodes are within the list of nodes of interest;
    //   if not, don't pass to any of the selectors
    BitSet in_list (in.get_nodes().size());
    for (vector<node_t>::const_iterator i = node_list.begin(); i != node_list.end(); ++i) {
      for (size_t n = 0; n != in.get_nodes().size(); ++n)
        if (*i == in.get_nodes()[n]) in_list[n] = true;
    }
    if (!in_list.full()) return true;
  }
  for (size_t i = 0; i != file_count(); ++i) {
    if (selectors[i] (in.get_nodes()))
      (*writers[i]) (in);
    else
      (*writers[i]) (empty_tck);
  }
  return true;
}







}
}
}
}


