/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */



#include <sstream>
#include <string>
#include <vector>

#include "command.h"
#include "progressbar.h"

#include "connectome/connectome.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/weights.h"
#include "dwi/tractography/connectome/extract.h"
#include "dwi/tractography/connectome/streamline.h"
#include "dwi/tractography/mapping/loader.h"

#include "image.h"

#include "thread_queue.h"




using namespace MR;
using namespace App;
using namespace MR::Connectome;
using namespace MR::DWI;
using namespace MR::DWI::Tractography;
using namespace MR::DWI::Tractography::Connectome;


const char* file_outputs[] = { "per_edge", "per_node", "single", NULL };


const OptionGroup OutputOptions = OptionGroup ("Options for determining the content / format of output files")

    + Option ("nodes", "only select tracks that involve a set of nodes of interest (provide as a comma-separated list of integers)")
      + Argument ("list").type_sequence_int()

    + Option ("exclusive", "only select tracks that exclusively connect nodes from within the list of nodes of interest")

    + Option ("files", "select how the resulting streamlines will be grouped in output files. "
                       "Options are: per_edge, per_node, single (default: per_edge)")
      + Argument ("option").type_choice (file_outputs)

    + Option ("exemplars", "generate a mean connection exemplar per edge, rather than keeping all streamlines "
                           "(the parcellation node image must be provided in order to constrain the exemplar endpoints)")
      + Argument ("image").type_image_in()

    + Option ("keep_unassigned", "by default, the program discards those streamlines that are not successfully assigned to a node. "
                                 "Set this option to generate corresponding outputs containing these streamlines (labelled as node index 0)")

    + Option ("keep_self", "by default, the program will not output streamlines that connect to the same node at both ends. "
                           "Set this option to instead keep these self-connections.");




void usage ()
{

  // Note: Creation of this OptionGroup depends on Tractography::TrackWeightsInOption
  //   already being defined; therefore, it cannot be defined statically, and
  //   must be constructed after the command is executed.
  const OptionGroup TrackWeightsOptions = OptionGroup ("Options for importing / exporting streamline weights")
      + Tractography::TrackWeightsInOption
      + Option ("prefix_tck_weights_out", "provide a prefix for outputting a text file corresponding to each output file, "
                                          "each containing only the streamline weights relevant for that track file")
        + Argument ("prefix").type_text();



  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "extract streamlines from a tractogram based on their assignment to parcellated nodes";

  ARGUMENTS
  + Argument ("tracks_in",      "the input track file").type_file_in()
  + Argument ("assignments_in", "text file containing the node assignments for each streamline").type_file_in()
  + Argument ("prefix_out",     "the output file / prefix").type_text();


  OPTIONS
  + OutputOptions
  + TrackWeightsOptions;

}




void run ()
{

  Tractography::Properties properties;
  Tractography::Reader<float> reader (argument[0], properties);

  vector< vector<node_t> > assignments_lists;
  assignments_lists.reserve (to<size_t>(properties["count"]));
  vector<NodePair> assignments_pairs;
  bool nonpair_found = false;
  node_t max_node_index = 0;
  {
    std::ifstream stream (argument[1]);
    std::string line;
    ProgressBar progress ("reading streamline assignments file");
    while (std::getline (stream, line)) {
      std::stringstream line_stream (line);
      vector<node_t> nodes;
      while (1) {
        node_t n;
        line_stream >> n;
        if (!line_stream) break;
        nodes.push_back (n);
        max_node_index = std::max (max_node_index, n);
      }
      if (nodes.size() != 2)
        nonpair_found = true;
      assignments_lists.push_back (std::move (nodes));
      ++progress;
    }
  }

  const size_t count = to<size_t>(properties["count"]);
  if (assignments_lists.size() != count)
    throw Exception ("Assignments file contains " + str(assignments_lists.size()) + " entries; track file contains " + str(count) + " tracks");

  // If the node assignments have been performed in such a way that each streamline is
  //   assigned to precisely two nodes, use the assignments_pairs class which is
  //   designed as such. This _should_ be the majority of cases, but the situation
  //   where each streamline could potentially be assigned to any number of nodes is
  //   now supported.
  if (!nonpair_found) {
    INFO ("Assignments file contains node pair for every streamline; operating accordingly");
    assignments_pairs.reserve (assignments_lists.size());
    for (auto i = assignments_lists.begin(); i != assignments_lists.end(); ++i)
      assignments_pairs.push_back (NodePair ((*i)[0], (*i)[1]));
    assignments_lists.clear();
  }

  const std::string prefix (argument[2]);
  auto opt = get_options ("prefix_tck_weights_out");
  const std::string weights_prefix = opt.size() ? std::string (opt[0][0]) : "";

  INFO ("Maximum node index is " + str(max_node_index));

  const node_t first_node = get_options ("keep_unassigned").size() ? 0 : 1;
  const bool keep_self = get_options ("keep_self").size();

  // Get the list of nodes of interest
  vector<node_t> nodes;
  opt = get_options ("nodes");
  bool manual_node_list = false;
  if (opt.size()) {
    manual_node_list = true;
    vector<int> data = parse_ints (opt[0][0]);
    bool zero_in_list = false;
    for (vector<int>::const_iterator i = data.begin(); i != data.end(); ++i) {
      if (size_t(*i) > max_node_index) {
        WARN ("Node of interest " + str(*i) + " is above the maximum detected node index of " + str(max_node_index));
      } else {
        nodes.push_back (node_t (*i));
        if (!*i)
          zero_in_list = true;
      }
    }
    if (!zero_in_list && !first_node)
      nodes.push_back (0);
    std::sort (nodes.begin(), nodes.end());
  } else {
    for (node_t i = first_node; i <= max_node_index; ++i)
      nodes.push_back (i);
  }

  const bool exclusive = get_options ("exclusive").size();
  if (exclusive && !manual_node_list)
    WARN ("List of nodes of interest not provided; -exclusive option will have no effect");

  opt = get_options ("files");
  const int file_format = opt.size() ? opt[0][0] : 0;

  opt = get_options ("exemplars");
  if (opt.size()) {

    if (keep_self)
      WARN ("Exemplars cannot be calculated for node self-connections; -keep_self option ignored");

    // Load the node image, get the centres of mass
    // Generate exemplars - these can _only_ be done per edge, and requires a mutex per edge to multi-thread
    auto image = Image<node_t>::open (opt[0][0]);
    vector<Eigen::Vector3f> COMs (max_node_index+1, Eigen::Vector3f (0.0f, 0.0f, 0.0f));
    vector<size_t> volumes (max_node_index+1, 0);
    for (auto i = Loop() (image); i; ++i) {
      const node_t index = image.value();
      if (index) {
        assert (index <= max_node_index);
        COMs[index] += Eigen::Vector3f (image.index(0), image.index(1), image.index(2));
        ++volumes[index];
      }
    }
    Transform transform (image);
    for (node_t index = 1; index <= max_node_index; ++index) {
      if (volumes[index])
        COMs[index] = (transform.voxel2scanner * (COMs[index] * (1.0f / float(volumes[index]))).cast<default_type>()).cast<float>();
      else
        COMs[index][0] = COMs[index][1] = COMs[index][2] = NAN;
    }

    // If user specifies a subset of nodes, only a subset of exemplars need to be calculated
    WriterExemplars generator (properties, nodes, exclusive, first_node, COMs);

    {
      std::mutex mutex;
      ProgressBar progress ("generating exemplars for connectome", count);
      if (assignments_pairs.size()) {
        auto loader = [&] (Tractography::Connectome::Streamline_nodepair& out) { if (!reader (out)) return false; out.set_nodes (assignments_pairs[out.index]); return true; };
        auto worker = [&] (const Tractography::Connectome::Streamline_nodepair& in) { generator (in); std::lock_guard<std::mutex> lock (mutex); ++progress; return true; };
        Thread::run_queue (loader, Thread::batch (Tractography::Connectome::Streamline_nodepair()), Thread::multi (worker));
      } else {
        auto loader = [&] (Tractography::Connectome::Streamline_nodelist& out) { if (!reader (out)) return false; out.set_nodes (assignments_lists[out.index]); return true; };
        auto worker = [&] (const Tractography::Connectome::Streamline_nodelist& in) { generator (in); std::lock_guard<std::mutex> lock (mutex); ++progress; return true; };
        Thread::run_queue (loader, Thread::batch (Tractography::Connectome::Streamline_nodelist()), Thread::multi (worker));
      }
    }

    generator.finalize();

    // Get exemplars to the output file(s), depending on the requested format
    if (file_format == 0) { // One file per edge
      if (exclusive) {
        ProgressBar progress ("writing exemplars to files", nodes.size() * (nodes.size()-1) / 2);
        for (size_t i = 0; i != nodes.size(); ++i) {
          const node_t one = nodes[i];
          for (size_t j = i+1; j != nodes.size(); ++j) {
            const node_t two = nodes[j];
            generator.write (one, two, prefix + str(one) + "-" + str(two) + ".tck", weights_prefix.size() ? (weights_prefix + str(one) + "-" + str(two) + ".csv") : "");
            ++progress;
          }
        }
      } else {
        // For each node in the list, write one file for an exemplar to every other node
        ProgressBar progress ("writing exemplars to files", nodes.size() * COMs.size());
        for (vector<node_t>::const_iterator n = nodes.begin(); n != nodes.end(); ++n) {
          for (size_t i = first_node; i != COMs.size(); ++i) {
            generator.write (*n, i, prefix + str(*n) + "-" + str(i) + ".tck", weights_prefix.size() ? (weights_prefix + str(*n) + "-" + str(i) + ".csv") : "");
            ++progress;
          }
        }
      }
    } else if (file_format == 1) { // One file per node
      ProgressBar progress ("writing exemplars to files", nodes.size());
      for (vector<node_t>::const_iterator n = nodes.begin(); n != nodes.end(); ++n) {
        generator.write (*n, prefix + str(*n) + ".tck", weights_prefix.size() ? (weights_prefix + str(*n) + ".csv") : "");
        ++progress;
      }
    } else if (file_format == 2) { // Single file
      std::string path = prefix;
      if (path.rfind (".tck") != path.size() - 4)
        path += ".tck";
      std::string weights_path = weights_prefix;
      if (weights_prefix.size() && weights_path.rfind (".tck") != weights_path.size() - 4)
        weights_path += ".csv";
      generator.write (path, weights_path);
    }

  } else { // Old behaviour ie. all tracks, rather than generating exemplars

    WriterExtraction writer (properties, nodes, exclusive, keep_self);

    switch (file_format) {
      case 0: // One file per edge
        for (size_t i = 0; i != nodes.size(); ++i) {
          const node_t one = nodes[i];
          if (exclusive) {
            for (size_t j = i; j != nodes.size(); ++j) {
              const node_t two = nodes[j];
              writer.add (one, two, prefix + str(one) + "-" + str(two) + ".tck", weights_prefix.size() ? (weights_prefix + str(one) + "-" + str(two) + ".csv") : "");
            }
          } else {
            // Allow duplication of edges; want to have a set of files for each node
            for (node_t two = first_node; two <= max_node_index; ++two)
              writer.add (one, two, prefix + str(one) + "-" + str(two) + ".tck", weights_prefix.size() ? (weights_prefix + str(one) + "-" + str(two) + ".csv") : "");
          }
        }
        INFO ("A total of " + str (writer.file_count()) + " output track files will be generated (one for each edge)");
        break;
      case 1: // One file per node
        for (vector<node_t>::const_iterator i = nodes.begin(); i != nodes.end(); ++i)
          writer.add (*i, prefix + str(*i) + ".tck", weights_prefix.size() ? (weights_prefix + str(*i) + ".csv") : "");
        INFO ("A total of " + str (writer.file_count()) + " output track files will be generated (one for each node)");
        break;
      case 2: // Single file
        std::string path = prefix;
        if (path.rfind (".tck") != path.size() - 4)
          path += ".tck";
        std::string weights_path = weights_prefix;
        if (weights_prefix.size() && weights_path.rfind (".tck") != weights_path.size() - 4)
          weights_path += ".csv";
        writer.add (nodes, path, weights_path);
        break;
    }

    ProgressBar progress ("Extracting tracks from connectome", count);
    if (assignments_pairs.size()) {
      Tractography::Connectome::Streamline_nodepair tck;
      while (reader (tck)) {
        tck.set_nodes (assignments_pairs[tck.index]);
        writer (tck);
        ++progress;
      }
    } else {
      Tractography::Connectome::Streamline_nodelist tck;
      while (reader (tck)) {
        tck.set_nodes (assignments_lists[tck.index]);
        writer (tck);
        ++progress;
      }
    }

  }

}
