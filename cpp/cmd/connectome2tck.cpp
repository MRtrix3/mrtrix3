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

#include <optional>
#include <sstream>
#include <string>

#include "command.h"
#include "enum.h"
#include "image.h"
#include "progressbar.h"
#include "thread_queue.h"
#include "types.h"

#include "connectome/connectome.h"
#include "connectome/validate.h"

#include "dwi/tractography/connectome/assignments.h"
#include "dwi/tractography/connectome/extract.h"
#include "dwi/tractography/connectome/streamline.h"
#include "dwi/tractography/formats/base.h"
#include "dwi/tractography/formats/list.h"
#include "dwi/tractography/grouping.h"
#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/tractogram.h"
#include "dwi/tractography/weights.h"

#include <filesystem>

using namespace MR;
using namespace App;
using namespace MR::Connectome;
using namespace MR::DWI;
using namespace MR::DWI::Tractography;
using namespace MR::DWI::Tractography::Connectome;

// Disambiguate from the image subsystem's MR::Formats, also in scope here.
namespace TrackFormats = MR::DWI::Tractography::Formats;

enum class FileOutput { PER_EDGE, PER_NODE, SINGLE };
constexpr FileOutput default_file_output = FileOutput::PER_EDGE;

// clang-format off
const OptionGroup TrackOutputOptions = OptionGroup ("Options for determining the content / format of output files")

    + Option ("nodes", "only select tracks that involve a set of nodes of interest"
                       " (provide as a comma-separated list of integers)")
      + Argument ("list").type_sequence_int()

    + Option ("exclusive", "only select tracks that exclusively connect nodes from within the list of nodes of interest")

    + Option ("files", "select how the resulting streamlines will be grouped in output files."
                       " Options are: " + MR::Enum::join<FileOutput>(", ") + ". Default: " + MR::Enum::lowercase_name(default_file_output) + ".")
      + Argument ("option").type_choice<FileOutput>()

    + Option ("file_format", "the output tractogram file format for the per-edge / per-node directory modes,"
                             " given as a filename extension (default: tck);"
                             " ignored for \"-files single\", where the format is taken from the output path."
                             " A format that can store per-streamline data (e.g. trx) embeds the streamline"
                             " weights, in which case -tck_weights_out does not apply.")
      + Argument ("extension").type_text()

    + Option ("exemplars", "generate a mean connection exemplar per edge,"
                           " rather than keeping all streamlines "
                           "(the parcellation node image must be provided in order to constrain the exemplar endpoints)")
      + Argument ("image").type_image_in()

    + Option ("keep_unassigned", "by default, the program discards those streamlines"
                                 " that are not successfully assigned to a node. "
                                 "Set this option to generate corresponding outputs containing these streamlines"
                                 " (labelled as node index 0)")

    + Option ("keep_self", "by default, the program will not output streamlines that connect to the same node at both ends."
                           " Set this option to instead keep these self-connections.");

void usage() {

  // Note: Creation of this OptionGroup
  //   depends on Tractography::TrackWeightsInOption already being defined;
  //   therefore, it cannot be defined statically,
  //   and must be constructed after the command is executed.
  const OptionGroup TrackWeightsOptions = OptionGroup ("Options for importing / exporting streamline weights")
      + Tractography::TrackWeightsInOption
      + Option ("tck_weights_out", "provide the output path for streamline weight data (see Description)")
        + Argument ("path").type_directory_out(DirOutMode::MayExist).type_file_out();

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Extract streamlines from a tractogram based on their assignment to parcellated nodes";

  DESCRIPTION
  + "The compulsory input file \"assignments_in\" should contain a text file"
    " where there is one row for each streamline,"
    " and each row contains a list of numbers corresponding to the parcels to which that streamline was assigned"
    " (most typically there will be two entries per streamline,"
    " one for each endpoint;"
    " but this is not strictly a requirement)."
    " This file will most typically be generated using the tck2connectome command with the -out_assignments option."

  + "When -files single is specified, the third argument is interpreted as a tractogram file path;"
    " otherwise it is interpreted as a directory,"
    " into which individual output tractogram files will be written."
    " The -tck_weights_out path is interpreted in the same manner,"
    " as either a single output file or a directory of per-tract-file weight text files."

  + "In the per-edge / per-node directory modes the output tractogram files are written"
    " in the format selected by the -file_format option (default \".tck\");"
    " with -files single the format is taken from the extension of the output path."
    " If the selected format can store per-streamline / per-vertex data (e.g. \".trx\"),"
    " the streamline weights are embedded within the output tractogram itself,"
    " and the separate -tck_weights_out output does not apply;"
    " for a vertices-only format (e.g. \".tck\") the weights are instead written as a separate sidecar file."
    " Any per-vertex (data-per-vertex) data carried by the input tractogram propagates to the output"
    " for formats that can embed it (e.g. \".trx\"), and is dropped for a vertices-only format."

  + "The -tck_weights_out option behaves similarity to the third argument as described above."
    " If option \"-files single\" is specified,"
    " then the user-specified input to the -tck_weights_out option will be interpreted"
    " as the path to a file to be created."
    " Otherwise, that path will instead be interpreted as a directory to be created,"
    " which will then be populated with files of the same name as the tractogram files"
    " written as the primary command output.";

  EXAMPLES
  + Example ("Default usage",
             "connectome2tck tracks.tck assignments.txt edges/",
             "The command will generate one track file for every edge in the connectome"
             " within the output directory \"edges/\";"
             " the name of each file indicates the nodes connected via that edge."
             " For instance, all streamlines connecting nodes 23 and 49"
             " will be written to file \"edges/23-49.tck\".")

  + Example ("Extract only the streamlines between nodes 1 and 2",
             "connectome2tck tracks.tck assignments.txt edge_1_2.tck -nodes 1,2 -exclusive -files single",
             "Since only a single edge is of interest,"
             " this example provides only the two nodes involved in that edge to the -nodes option,"
             " adds the -exclusive option so that only streamlines for which"
             " both assigned nodes are in the list of nodes of interest are extracted"
             " (i.e. only streamlines connecting nodes 1 and 2 in this example),"
             " and writes the result to output track file \"edge_1_2.tck\".")

  + Example ("Extract the streamlines connecting node 15 to all other nodes in the parcellation,"
             " with one track file for each edge",
             "connectome2tck tracks.tck assignments.txt from_node15/ -nodes 15 -keep_self",
             "The command will generate the same number of track files as there are nodes in the parcellation:"
             " one each for the streamlines connecting node 15 to every other node;"
             " i.e. \"from_node15/15-1.tck\", \"from_node15/15-2.tck\", \"from_node15/15-3.tck\", etc.."
             " Because the -keep_self option is specified,"
             " file \"from_node15/15-15.tck\" will also be generated,"
             " containing those streamlines that connect to node 15 at both endpoints.")

  + Example ("For every node,"
             " generate a file containing all streamlines connected to that node",
             "connectome2tck tracks.tck assignments.txt nodes/ -files per_node",
             "Here the command will generate one track file for every node in the connectome:"
             " \"nodes/1.tck\", \"nodes/2.tck\", \"nodes/3.tck\", etc.."
             " Each of these files will contain all streamlines"
             " that connect the node of that index to another node in the connectome "
             "(it does not select all tracks connecting a particular node,"
             " since the -keep_self option was omitted"
             " and therefore e.g. a streamline that is assigned to node 41"
             " will not be present in file \"nodes/41.tck\")."
             " Each streamline in the input tractogram will in fact"
             " appear in two different output track files;"
             " e.g. a streamline connecting nodes 8 and 56 will be present"
             " both in file \"nodes/8.tck\" and file \"nodes/56.tck\".")

  + Example ("Get all streamlines that were not successfully assigned to a node pair",
             "connectome2tck tracks.tck assignments.txt unassigned.tck -nodes 0 -keep_self -files single",
             "Node index 0 corresponds to streamline endpoints"
             " that were not successfully assigned to a node."
             " As such, by selecting all streamlines that are assigned to \"node 0\""
             " (including those streamlines for which neither endpoint is assigned to a node"
             " due to use of the -keep_self option),"
             " the output track file \"unassigned.tck\" will contain all streamlines"
             " for which at least one of the two endpoints was not successfully assigned to a node.")

  + Example ("Generate a single track file containing edge exemplar trajectories",
             "connectome2tck tracks.tck assignments.txt exemplars.tck -files single -exemplars nodes.mif",
             "This produces the track file \"exemplars.tck\" that is required as input"
             " when attempting to display connectome edges using the streamlines or streamtubes geometries"
             " within the mrview connectome tool.");

  ARGUMENTS
  + Argument ("tracks_in",      "the input track file").type_file_in()
  + Argument ("assignments_in", "input text file containing the node assignments for each streamline").type_tractogram_sidecar_in()
  + Argument ("output",         "the output tractogram file / directory path (see Description)")
              .type_directory_out(DirOutMode::MayExist).type_file_out();

  OPTIONS
  + TrackOutputOptions
  + TrackWeightsOptions;

}
// clang-format on

void run() {
  const std::filesystem::path tracks_input_path{argument[0]};
  const std::filesystem::path assignments_input_path{argument[1]};
  const std::filesystem::path output_path{argument[2]};

  // Determine output file format first, as it affects interpretation of both output paths
  const FileOutput file_format = get_option_choice<FileOutput>("files", default_file_output);

  std::filesystem::path output_dir;
  std::filesystem::path output_file;
  if (file_format == FileOutput::SINGLE) {
    output_file = output_path;
    if (!output_path.has_filename())
      WARN("Output path \"" + argument[2].as_text() + "\" ends with a path separator;" +                  //
           " when -files single is specified, the output path is interpreted as a tractogram file path"); //
  } else {
    output_dir = output_path;
    if (output_path.extension() == ".tck")
      WARN("Output path \"" + argument[2].as_text() + "\" has a .tck extension;" +               //
           " unless -files single is specified, the output path is interpreted as a directory"); //
    std::filesystem::create_directories(output_dir);
  }

  // Resolve the output tractogram file format. In the directory modes the per-edge
  //   / per-node files take the -file_format extension (default "tck"); for a single
  //   file the format is that of the output path itself.
  std::string out_extension = "tck";
  {
    const auto opt_format = get_options("file_format");
    if (!opt_format.empty()) {
      if (file_format == FileOutput::SINGLE) {
        WARN("The -file_format option has no effect with \"-files single\";" +                                  //
             std::string(" the output format is taken from the output path \"") + output_file.string() + "\""); //
      } else {
        out_extension = lowercase(opt_format[0][0].as_text());
        if (!out_extension.empty() && out_extension.front() == '.')
          out_extension.erase(0, 1);
      }
    }
  }
  const std::filesystem::path format_probe =
      (file_format == FileOutput::SINGLE) ? output_file : std::filesystem::path("x." + out_extension);
  const TrackFormats::Base *const out_handler = TrackFormats::get_handler(format_probe);
  if (out_handler == nullptr)
    throw Exception("no tractography format handler recognises the output " +
                    (file_format == FileOutput::SINGLE ? ("path \"" + output_file.string() + "\"")
                                                       : ("file format \"" + out_extension + "\"")));
  if (!out_handler->can_write())
    throw Exception("tractography format \"" + out_handler->description + "\" does not support writing");
  // A format that serialises per-streamline / per-vertex data embeds the weights
  //   directly (and propagates any input per-vertex data); the separate-file weight
  //   output then does not apply.
  const bool embed = out_handler->capabilities.sidecar_data != TrackFormats::SidecarData::Unsupported;
  if (embed) {
    if (!get_options("tck_weights_out").empty())
      throw Exception("output format \"" + out_handler->description + "\" embeds per-streamline weights;" + //
                      " the -tck_weights_out option (separate weight files) does not apply");
  }

  Tractography::Properties properties;
  auto reader = Tractography::Tractogram<float>::open(tracks_input_path, properties);

  // Route "-tck_weights_in" (a standalone scalar file OR a "<dataset>::<field>" named field
  //   of the input tractogram) into Streamline::weight as each streamline is read. The
  //   Tractogram applies the weight on every read, so the manual per-streamline check is no
  //   longer required; an external weight file shorter than the tractogram truncates the read
  //   inside Tractogram::read, mirroring the legacy reader.
  const Tractography::WeightInput weight_input = Tractography::register_weight_input(reader, tracks_input_path);

  // Read the assignments through the shared Assignments class (Stage 17, step 4):
  //   the same byte-faithful "-out_assignments" text format, now the import
  //   interface to the canonical streamline Grouping (§2.3 / D6).
  const Tractography::Connectome::Assignments assignments =
      Tractography::Connectome::Assignments::load(assignments_input_path);
  // The canonical grouping encoding (edge "<n1>-<n2>" / node "<n>" → streamline
  //   indices), realising overlap / multi-membership; carried alongside the
  //   per-streamline tables that drive extraction below.
  const Tractography::Grouping grouping = assignments.to_grouping();
  DEBUG("connectome assignments encode " + str(grouping.size()) + " streamline group(s)");
  node_t max_node_index = assignments.max_node();
  const bool nonpair_found = !assignments.all_pairs();

  std::vector<std::vector<node_t>> assignments_lists = assignments.as_lists();
  std::vector<NodePair> assignments_pairs;
  INFO("Maximum node index in assignments file is " + str(max_node_index));

  // The streamline count drives the assignments cross-check and progress sizing.
  //   Not every format header advertises it (e.g. TRX), so fall back to the
  //   assignments count when the property is absent; the index-keyed routing below
  //   does not depend on a pre-known count.
  std::optional<size_t> header_count;
  {
    const auto count_it = properties.find("count");
    if (count_it != properties.end() && !count_it->second.empty())
      header_count = to<size_t>(count_it->second);
  }
  if (header_count.has_value() && assignments_lists.size() != *header_count)
    throw Exception("Assignments file contains " + str(assignments_lists.size()) + " entries;" + //
                    " track file contains " + str(*header_count) + " tracks");                   //
  const size_t count = header_count.value_or(assignments_lists.size());

  // If the node assignments have been performed in such a way that each streamline is
  //   assigned to precisely two nodes, use the assignments_pairs class which is
  //   designed as such. This _should_ be the majority of cases, but the situation
  //   where each streamline could potentially be assigned to any number of nodes is
  //   now supported.
  if (!nonpair_found) {
    INFO("Assignments file contains node pair for every streamline; operating accordingly");
    assignments_pairs = assignments.as_pairs();
    assignments_lists.clear();
  }

  const auto opt_weights = get_options("tck_weights_out");
  std::optional<std::filesystem::path> weights_dir;
  std::optional<std::filesystem::path> weights_file;
  if (!opt_weights.empty()) {
    const std::filesystem::path weights_path{opt_weights[0][0]};
    if (file_format == FileOutput::SINGLE) {
      weights_file.emplace(weights_path);
      if (!weights_path.has_filename())
        WARN("Weights output path \"" + weights_path.string() + "\" ends with a path separator;" +         //
             " when -files single is specified, the -tck_weights_out path is interpreted as a file path"); //
    } else {
      weights_dir.emplace(weights_path);
      if (weights_path.has_extension())
        WARN("Weights output path \"" + weights_path.string() + "\" has a file extension;" +                 //
             " unless -files single is specified, the -tck_weights_out path is interpreted as a directory"); //
      std::filesystem::create_directories(weights_path);
    }
  }
  const auto weights_path_for = [&](const std::string_view filename) -> std::optional<std::filesystem::path> {
    return weights_dir ? std::optional<std::filesystem::path>{*weights_dir / std::string{filename}} : std::nullopt;
  };

  const node_t first_node = get_options("keep_unassigned").empty() ? 1 : 0;
  const bool keep_self = !get_options("keep_self").empty();

  // Get the list of nodes of interest
  std::vector<node_t> nodes;
  auto opt = get_options("nodes");
  bool manual_node_list = false;
  if (!opt.empty()) {
    manual_node_list = true;
    const auto data = opt[0][0].as_sequence_uint();
    bool zero_in_list = false;
    for (auto i : data) {
      if (i > max_node_index) {
        WARN("Node of interest " + str(i) + " is above the maximum detected node index of " + str(max_node_index));
      } else {
        nodes.push_back(i);
        if (!i)
          zero_in_list = true;
      }
    }
    if (!zero_in_list && !first_node)
      nodes.push_back(0);
    std::sort(nodes.begin(), nodes.end());
  } else {
    for (node_t i = first_node; i <= max_node_index; ++i)
      nodes.push_back(i);
  }

  const bool exclusive = !get_options("exclusive").empty();
  if (exclusive && !manual_node_list)
    WARN("List of nodes of interest not provided; -exclusive option will have no effect");

  opt = get_options("exemplars");
  if (!opt.empty()) {
    if (keep_self)
      WARN("Exemplars cannot be calculated for node self-connections; -keep_self option ignored");

    // Load the node image, get the centres of mass
    // Generate exemplars - these can _only_ be done per edge, and requires a mutex per edge to multi-thread
    auto image = Image<node_t>::open(opt[0][0]);

    auto lv = MR::Connectome::validate_label_image(image);
    if (lv.labels.back() != max_node_index) {
      WARN("Highest-valued parcels in label image \"" + opt[0][0] + "\"" +        //
           " (" + str(lv.labels.back()) + ")" +                                   //
           " differs from highest-valued node in connectome assignments file (" + //
           str(max_node_index) + ");" +                                           //
           " this may lead to issues in exemplar generation");                    //
    }
    std::vector<node_t> missing_nodes;
    for (const auto n : nodes) {
      if (std::find(lv.missing_indices.begin(), lv.missing_indices.end(), n) != lv.missing_indices.end())
        missing_nodes.push_back(n);
    }
    if (!missing_nodes.empty())
      throw Exception(str(missing_nodes.size()) + " node" +                      //
                      (missing_nodes.size() > 1 ? "s" : "") + " of interest" +   //
                      " are absent from the parcellation image," +               //
                      " precluding exemplar generation: " + str(missing_nodes)); //

    if (lv.disconnected_components > 0) {
      WARN(str(lv.disconnected_components) + " parcel" + //
           (lv.disconnected_components > 0 ? "s are" : " is") +
           " not spatially contiguous;"
           " this may result in unusual exemplar trajectories");
    }
    std::vector<Eigen::Vector3d> COMs(max_node_index + 1, Eigen::Vector3d::Constant(0.0));
    std::vector<size_t> volumes(max_node_index + 1, 0);
    for (auto i = Loop()(image); i; ++i) {
      const node_t index = image.value();
      if (index) {
        while (index >= COMs.size()) {
          COMs.push_back(Eigen::Vector3d::Zero());
          volumes.push_back(0);
        }
        COMs[index] += Eigen::Vector3d(static_cast<default_type>(image.index(0)),
                                       static_cast<default_type>(image.index(1)),
                                       static_cast<default_type>(image.index(2)));
        ++volumes[index];
      }
    }
    if (COMs.size() > max_node_index + 1) {
      WARN("Parcellation image \"" + opt[0][0].as_text() + "\" provided via -exemplars option" + //
           " contains more nodes (" + str(COMs.size() - 1) + ")" +                               //
           " than are present in input assignments file \"" + argument[1].as_text() + "\"" +     //
           " (" + str(max_node_index) + ")");                                                    //
      max_node_index = COMs.size() - 1;
    }
    Transform transform(image);
    for (node_t index = 1; index <= max_node_index; ++index) {
      if (volumes[index])
        COMs[index] = transform.voxel2scanner * (COMs[index] * (1.0 / static_cast<default_type>(volumes[index])));
      else
        COMs[index].fill(NaN);
    }

    // If user specifies a subset of nodes, only a subset of exemplars need to be calculated
    WriterExemplars generator(properties, nodes, exclusive, first_node, COMs);
    if (embed)
      generator.enable_embedding();

    {
      std::mutex mutex;
      ProgressBar progress("generating exemplars for connectome", count);
      if (!assignments_pairs.empty()) {
        auto loader = [&](Tractography::Connectome::Streamline_nodepair &out) {
          if (!reader(out))
            return false;
          out.set_nodes(assignments_pairs[out.get_index()]);
          return true;
        };
        auto worker = [&](const Tractography::Connectome::Streamline_nodepair &in) {
          generator(in);
          std::lock_guard<std::mutex> lock(mutex);
          ++progress;
          return true;
        };
        Thread::run_queue(
            loader, Thread::batch(Tractography::Connectome::Streamline_nodepair()), Thread::multi(worker));
      } else {
        auto loader = [&](Tractography::Connectome::Streamline_nodelist &out) {
          if (!reader(out))
            return false;
          out.set_nodes(assignments_lists[out.get_index()]);
          return true;
        };
        auto worker = [&](const Tractography::Connectome::Streamline_nodelist &in) {
          generator(in);
          std::lock_guard<std::mutex> lock(mutex);
          ++progress;
          return true;
        };
        Thread::run_queue(
            loader, Thread::batch(Tractography::Connectome::Streamline_nodelist()), Thread::multi(worker));
      }
    }

    generator.finalize();

    // Get exemplars to the output file(s), depending on the requested format
    if (file_format == FileOutput::PER_EDGE) { // One file per edge
      if (exclusive) {
        ProgressBar progress("writing exemplars to files", nodes.size() * (nodes.size() - 1) / 2);
        for (size_t i = 0; i != nodes.size(); ++i) {
          const node_t one = nodes[i];
          for (size_t j = i + 1; j != nodes.size(); ++j) {
            const node_t two = nodes[j];
            generator.write(one,
                            two,
                            output_dir / (str(one) + "-" + str(two) + ("." + out_extension)),
                            weights_path_for(str(one) + "-" + str(two) + ".csv"));
            ++progress;
          }
        }
      } else {
        // For each node in the list, write one file for an exemplar to every other node
        ProgressBar progress("writing exemplars to files", nodes.size() * COMs.size());
        for (std::vector<node_t>::const_iterator n = nodes.begin(); n != nodes.end(); ++n) {
          for (size_t i = first_node; i != COMs.size(); ++i) {
            generator.write(*n,
                            i,
                            output_dir / (str(*n) + "-" + str(i) + ("." + out_extension)),
                            weights_path_for(str(*n) + "-" + str(i) + ".csv"));
            ++progress;
          }
        }
      }
    } else if (file_format == FileOutput::PER_NODE) { // One file per node
      ProgressBar progress("writing exemplars to files", nodes.size());
      for (std::vector<node_t>::const_iterator n = nodes.begin(); n != nodes.end(); ++n) {
        generator.write(*n, output_dir / (str(*n) + ("." + out_extension)), weights_path_for(str(*n) + ".csv"));
        ++progress;
      }
    } else if (file_format == FileOutput::SINGLE) { // Single file
      generator.write(output_file, weights_file);
    }

  } else { // Old behaviour ie. all tracks, rather than generating exemplars

    WriterExtraction writer(properties, nodes, exclusive, keep_self);
    if (embed)
      writer.enable_embedding(reader.fields());

    switch (file_format) {
    case FileOutput::PER_EDGE: // One file per edge
      for (size_t i = 0; i != nodes.size(); ++i) {
        const node_t one = nodes[i];
        if (exclusive) {
          for (size_t j = i; j != nodes.size(); ++j) {
            const node_t two = nodes[j];
            writer.add(one,
                       two,
                       output_dir / (str(one) + "-" + str(two) + ("." + out_extension)),
                       weights_path_for(str(one) + "-" + str(two) + ".csv"));
          }
        } else {
          // Allow duplication of edges; want to have an exhaustive set of files for each node
          for (node_t two = first_node; two <= max_node_index; ++two)
            writer.add(one,
                       two,
                       output_dir / (str(one) + "-" + str(two) + ("." + out_extension)),
                       weights_path_for(str(one) + "-" + str(two) + ".csv"));
        }
      }
      INFO("A total of " + str(writer.file_count()) + " output track files will be generated (one for each edge)");
      break;
    case FileOutput::PER_NODE: // One file per node
      for (std::vector<node_t>::const_iterator i = nodes.begin(); i != nodes.end(); ++i)
        writer.add(*i, output_dir / (str(*i) + ("." + out_extension)), weights_path_for(str(*i) + ".csv"));
      INFO("A total of " + str(writer.file_count()) + " output track files will be generated (one for each node)");
      break;
    case FileOutput::SINGLE: // Single file
      writer.add(nodes, output_file, weights_file);
      break;
    }

    ProgressBar progress("Extracting tracks from connectome", count);
    // Read the composite item (vertices + weight + any per-vertex / per-streamline
    //   sidecar data) so that input dpv propagates to the output unchanged; route by
    //   ordinal index exactly as before, passing the node assignment alongside.
    Tractography::TractogramItem<float> item;
    if (assignments_pairs.empty()) {
      while (reader.read(item)) {
        writer(item, assignments_lists[item.get_index()]);
        ++progress;
      }
    } else {
      while (reader.read(item)) {
        writer(item, assignments_pairs[item.get_index()]);
        ++progress;
      }
    }
  }
}
