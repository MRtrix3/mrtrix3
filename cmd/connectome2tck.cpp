/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#include <sstream>
#include <string>

#include "command.h"
#include "image.h"
#include "progressbar.h"
#include "thread_queue.h"
#include "types.h"

#include "connectome/connectome.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/weights.h"
#include "dwi/tractography/connectome/extract.h"
#include "dwi/tractography/connectome/streamline.h"
#include "dwi/tractography/mapping/loader.h"



using namespace MR;
using namespace App;
using namespace MR::Connectome;
using namespace MR::DWI;
using namespace MR::DWI::Tractography;
using namespace MR::DWI::Tractography::Connectome;


const char* file_outputs[] = { "per_edge", "per_node", "single", NULL };


const OptionGroup TrackOutputOptions = OptionGroup ("Options for determining the content / format of output files")

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

  SYNOPSIS = "Extract streamlines from a tractogram based on their assignment to parcellated nodes";

  DESCRIPTION
  + "The compulsory input file \"assignments_in\" should contain a text file where there is one row for each streamline, "
    "and each row contains a list of numbers corresponding to the parcels to which that streamline was assigned "
    "(most typically there will be two entries per streamline, one for each endpoint; but this is not strictly a requirement). "
    "This file will most typically be generated using the tck2connectome command with the -out_assignments option.";

  EXAMPLES
  + Example ("Default usage",
             "connectome2tck tracks.tck assignments.txt edge-",
             "The command will generate one track file for every edge in the connectome, with the name of each file "
             "indicating the nodes connected via that edge; for instance, all streamlines connecting nodes "
             "23 and 49 will be written to file \"edge-23-49.tck\".")

  + Example ("Extract only the streamlines between nodes 1 and 2",
             "connectome2tck tracks.tck assignments.txt tracks_1_2.tck -nodes 1,2 -exclusive -files single",
             "Since only a single edge is of interest, this example provides only the two nodes involved "
             "in that edge to the -nodes option, adds the -exclusive option so that only streamlines for which "
             "both assigned nodes are in the list of nodes of interest are extracted (i.e. only streamlines connecting "
             "nodes 1 and 2 in this example), and writes the result to a single output track file.")

  + Example ("Extract the streamlines connecting node 15 to all other nodes in the parcellation, with one track file for each edge",
             "connectome2tck tracks.tck assignments.txt from_15_to_ -nodes 15 -keep_self",
             "The command will generate the same number of track files as there are nodes in the parcellation: "
             "one each for the streamlines connecting node 15 to every other node; i.e. "
             "\"from_15_to_1.tck\", \"from_15_to_2.tck\", \"from_15_to_3.tck\", etc.. "
             "Because the -keep_self option is specified, file \"from_15_to_15.tck\" will also be "
             "generated, containing those streamlines that connect to node 15 at both endpoints.")

  + Example ("For every node, generate a file containing all streamlines connected to that node",
             "connectome2tck tracks.tck assignments.txt node -files per_node",
             "Here the command will generate one track file for every node in the connectome: "
             "\"node1.tck\", \"node2.tck\", \"node3.tck\", etc.. Each of these files will contain "
             "all streamlines that connect the node of that index to another node in the connectome "
             "(it does not select all tracks connecting a particular node, since the -keep_self "
             "option was omitted and therefore e.g. a streamline that is assigned to node 41 will "
             "not be present in file \"node41.tck\"). Each streamline in the input tractogram will "
             "in fact appear in two different output track files; e.g. a streamline connecting nodes "
             "8 and 56 will be present both in file \"node8.tck\" and file \"node56.tck\".")

  + Example ("Get all streamlines that were not successfully assigned to a node pair",
             "connectome2tck tracks.tck assignments.txt unassigned.tck -nodes 0 -keep_self -files single",
             "Node index 0 corresponds to streamline endpoints that were not successfully assigned to "
             "a node. As such, by selecting all streamlines that are assigned to \"node 0\" "
             "(including those streamlines for which neither endpoint is assigned to a node due to "
             "use of the -keep_self option), the single output track file will contain all streamlines "
             "for which at least one of the two endpoints was not successfully assigned to a node.")

  + Example ("Generate a single track file containing edge exemplar trajectories",
             "connectome2tck tracks.tck assignments.txt exemplars.tck -files single -exemplars nodes.mif",
             "This produces the track file that is required as input when attempting to display "
             "connectome edges using the streamlines or streamtubes geometries within the meview "
             "connectome tool.");

  ARGUMENTS
  + Argument ("tracks_in",      "the input track file").type_file_in()
  + Argument ("assignments_in", "input text file containing the node assignments for each streamline").type_file_in()
  + Argument ("prefix_out",     "the output file / prefix").type_text();


  OPTIONS
  + TrackOutputOptions
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
      line = strip (line.substr (0, line.find_first_of ('#')));
      if (line.empty())
        continue;
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

  INFO ("Maximum node index in assignments file is " + str(max_node_index));

  const node_t first_node = get_options ("keep_unassigned").size() ? 0 : 1;
  const bool keep_self = get_options ("keep_self").size();

  // Get the list of nodes of interest
  vector<node_t> nodes;
  opt = get_options ("nodes");
  bool manual_node_list = false;
  if (opt.size()) {
    manual_node_list = true;
    const auto data = parse_ints<node_t> (opt[0][0]);
    bool zero_in_list = false;
    for (auto i : data) {
      if (i > max_node_index) {
        WARN ("Node of interest " + str(i) + " is above the maximum detected node index of " + str(max_node_index));
      } else {
        nodes.push_back (i);
        if (!i)
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
        while (index >= COMs.size()) {
          COMs.push_back (Eigen::Vector3f (0.0f, 0.0f, 0.0f));
          volumes.push_back (0);
        }
        COMs[index] += Eigen::Vector3f (image.index(0), image.index(1), image.index(2));
        ++volumes[index];
      }
    }
    if (COMs.size() > max_node_index + 1) {
      WARN ("Parcellation image \"" + std::string (opt[0][0]) + "\" provided via -exemplars option contains more nodes (" + str(COMs.size()-1) + ") than are present in input assignments file \"" + std::string (argument[1]) + "\" (" + str(max_node_index) + ")");
      max_node_index = COMs.size()-1;
    }
    Transform transform (image);
    for (node_t index = 1; index <= max_node_index; ++index) {
      if (volumes[index])
        COMs[index] = (transform.voxel2scanner * (COMs[index] * (1.0f / float(volumes[index]))).cast<default_type>()).cast<float>();
      else
        COMs[index][0] = COMs[index][1] = COMs[index][2] = NaN;
    }

    // If user specifies a subset of nodes, only a subset of exemplars need to be calculated
    WriterExemplars generator (properties, nodes, exclusive, first_node, COMs);

    {
      std::mutex mutex;
      ProgressBar progress ("generating exemplars for connectome", count);
      if (assignments_pairs.size()) {
        auto loader = [&] (Tractography::Connectome::Streamline_nodepair& out) { if (!reader (out)) return false; out.set_nodes (assignments_pairs[out.get_index()]); return true; };
        auto worker = [&] (const Tractography::Connectome::Streamline_nodepair& in) { generator (in); std::lock_guard<std::mutex> lock (mutex); ++progress; return true; };
        Thread::run_queue (loader, Thread::batch (Tractography::Connectome::Streamline_nodepair()), Thread::multi (worker));
      } else {
        auto loader = [&] (Tractography::Connectome::Streamline_nodelist& out) { if (!reader (out)) return false; out.set_nodes (assignments_lists[out.get_index()]); return true; };
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
            // Allow duplication of edges; want to have an exhaustive set of files for each node
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
        tck.set_nodes (assignments_pairs[tck.get_index()]);
        writer (tck);
        ++progress;
      }
    } else {
      Tractography::Connectome::Streamline_nodelist tck;
      while (reader (tck)) {
        tck.set_nodes (assignments_lists[tck.get_index()]);
        writer (tck);
        ++progress;
      }
    }

  }

}
