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

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "command.h"
#include "image.h"
#include "progressbar.h"
#include "types.h"

#include "connectome/lut.h"
#include "dwi/tractography/connectome/connectome.h"
#include "dwi/tractography/connectome/tck2nodes.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/trx_utils.h"

#include <trx/trx.h>

using namespace MR;
using namespace App;
using namespace MR::DWI::Tractography;
using namespace MR::DWI::Tractography::Connectome;
using namespace MR::DWI::Tractography::TRX;

// clang-format off
void usage() {

  AUTHOR = "MRtrix3 contributors";

  SYNOPSIS = "Assign streamlines in a TRX file to groups based on one or more parcellation atlases";

  DESCRIPTION
  + "Iterates over all streamlines in the input TRX file and assigns each streamline "
    "to one or two groups per atlas based on the parcellation nodes touched by its endpoints, "
    "using the same endpoint-to-node mechanisms as tck2connectome. "
    "One group is created per node that receives at least one streamline endpoint assignment. "
    "Groups are added to the output TRX file (which may be the same as the input). "
    "Each group is named after the parcellation node index, or the name from the lookup "
    "table if one is provided with -lut."

  + "Multiple atlases can be processed in a single invocation by specifying -nodes more than once, "
    "each optionally paired with a -lut and a -prefix. "
    "The number of -lut and -prefix entries must either be zero (omit entirely to use defaults) "
    "or exactly equal to the number of -nodes entries, one per atlas in order. "
    "All groups from all atlases are written to the output TRX in one shot."

  + MR::DWI::Tractography::Connectome::tck2nodes_description;

  EXAMPLES
  + Example ("Label streamlines by Desikan-Killiany atlas parcels",
             "trxlabel tracks.trx tracks_labeled.trx -nodes dk_nodes.mif -lut FreeSurferColorLUT.txt",
             "Streamlines are assigned to the nearest DK parcellation node using the default "
             "radial search. Group names are taken from the FreeSurfer LUT.")

  + Example ("Label with two atlases in one invocation",
             "trxlabel tracks.trx tracks_labeled.trx "
             "-nodes dk_nodes.mif -nodes aal_nodes.mif "
             "-lut dk_lut.txt -lut aal_lut.txt "
             "-prefix dk -prefix aal",
             "Groups named dk_Left-Hippocampus, aal_Hippocampus_L, etc. are all written "
             "to tracks_labeled.trx in a single pass over the tractogram.")

  + Example ("Label with one atlas, no LUT, with prefix",
             "trxlabel tracks.trx tracks_labeled.trx -nodes schaefer200_nodes.mif -prefix sch200",
             "Groups are named sch200_1, sch200_2, ... using numeric node indices.");

  ARGUMENTS
  + Argument ("tracks_in",  "the input TRX tractogram").type_tracks_in()
  + Argument ("tracks_out", "the output TRX tractogram (can be the same as tracks_in to label in-place)")
      .type_tracks_out()
      .type_directory_out();

  OPTIONS
  + Option ("nodes", "parcellation image defining the nodes; can be specified multiple times for multiple atlases").allow_multiple()
    + Argument ("nodes_image").type_image_in()

  + Option ("lut", "lookup table for mapping node indices to group names "
                   "(supports FreeSurfer, AAL, ITK-SNAP, and MRtrix LUT formats); "
                   "specify once per -nodes entry in the same order, or omit entirely to use numeric node indices").allow_multiple()
    + Argument ("path").type_file_in()

  + Option ("prefix", "prefix to prepend to group names (followed by an underscore); "
                      "specify once per -nodes entry in the same order, or omit entirely for no prefix; "
                      "strongly recommended when using multiple atlases to avoid group name collisions").allow_multiple()
    + Argument ("string").type_text()

  + MR::DWI::Tractography::Connectome::AssignmentOptions;

}
// clang-format on

// Sanitize a string for use as a TRX group name (which becomes a filename inside the archive).
static std::string sanitize_group_name(std::string name) {
  for (char &c : name) {
    if (c == ' ' || c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>')
      c = '_';
  }
  return name;
}

// Populate a Streamline<float> from TRX position arrays for streamline index i.
static void load_streamline(Streamline<float> &tck, const trx::TrxFile<float> &trx, const size_t i) {
  tck.clear();
  tck.set_index(i);
  const Eigen::Index start = trx.streamlines->_offsets(static_cast<Eigen::Index>(i), 0);
  const Eigen::Index end = trx.streamlines->_offsets(static_cast<Eigen::Index>(i + 1), 0);
  tck.reserve(static_cast<size_t>(end - start));
  for (Eigen::Index j = start; j < end; ++j)
    tck.push_back({trx.streamlines->_data(j, 0), trx.streamlines->_data(j, 1), trx.streamlines->_data(j, 2)});
}

// Assign all streamlines in trx to parcellation nodes via tck2nodes, accumulating results into
// node_to_indices (a map from node_id to a vector of streamline indices assigned to that node).
static void assign_atlas(std::map<node_t, std::vector<uint32_t>> &node_to_indices,
                         const trx::TrxFile<float> &trx,
                         Tck2nodes_base &tck2nodes,
                         const size_t n_streamlines,
                         const std::string &atlas_label) {
  ProgressBar progress("Assigning streamlines to nodes (" + atlas_label + ")", n_streamlines);
  Streamline<float> tck;

  if (tck2nodes.provides_pair()) {
    for (size_t i = 0; i < n_streamlines; ++i) {
      load_streamline(tck, trx, i);
      const NodePair pair = tck2nodes(tck);
      if (pair.first)
        node_to_indices[pair.first].push_back(static_cast<uint32_t>(i));
      if (pair.second && pair.second != pair.first)
        node_to_indices[pair.second].push_back(static_cast<uint32_t>(i));
      ++progress;
    }
  } else {
    // -assignment_all_voxels: a streamline may touch many nodes
    for (size_t i = 0; i < n_streamlines; ++i) {
      load_streamline(tck, trx, i);
      std::vector<node_t> node_list;
      tck2nodes(tck, node_list);
      for (const node_t n : node_list)
        if (n)
          node_to_indices[n].push_back(static_cast<uint32_t>(i));
      ++progress;
    }
  }
}

void run() {
  const std::string input_path = argument[0];
  const std::string output_path = argument[1];

  // Require TRX input — groups cannot be stored in TCK files
  if (!is_trx(input_path))
    throw Exception("Input tractogram must be a TRX file; "
                    "for TCK input, first convert with: tckconvert input.tck output.trx");

  // Collect atlas, LUT, and prefix options
  auto nodes_opts = get_options("nodes");
  auto lut_opts = get_options("lut");
  auto prefix_opts = get_options("prefix");

  if (nodes_opts.empty())
    throw Exception("At least one -nodes option must be specified");

  const size_t n_atlases = nodes_opts.size();

  if (!lut_opts.empty() && lut_opts.size() != n_atlases)
    throw Exception("The number of -lut entries (" + str(lut_opts.size()) +
                    ") must match the number of -nodes entries (" + str(n_atlases) + "), or be omitted entirely");
  if (!prefix_opts.empty() && prefix_opts.size() != n_atlases)
    throw Exception("The number of -prefix entries (" + str(prefix_opts.size()) +
                    ") must match the number of -nodes entries (" + str(n_atlases) + "), or be omitted entirely");

  // Load TRX once; all atlas assignments share the same positions
  auto trx = load_trx(input_path);
  if (!trx || !trx->streamlines)
    throw Exception("Failed to load TRX file: " + input_path);
  const size_t n_streamlines = trx->num_streamlines();

  // Accumulate all groups from all atlases into one map before writing
  std::map<std::string, std::vector<uint32_t>> all_groups;

  for (size_t atlas_idx = 0; atlas_idx < n_atlases; ++atlas_idx) {
    // Load atlas image
    const std::string nodes_path(nodes_opts[atlas_idx][0]);
    auto node_header = Header::open(nodes_path);
    MR::Connectome::check(node_header);
    auto node_image = node_header.get_image<node_t>();

    // The assignment mechanism is shared across atlases (same search parameters)
    auto tck2nodes = load_assignment_mode(node_image);

    // Load LUT for this atlas (if provided)
    std::unique_ptr<MR::Connectome::LUT> lut;
    if (!lut_opts.empty())
      lut = std::make_unique<MR::Connectome::LUT>(std::string(lut_opts[atlas_idx][0]));

    // Determine group name prefix for this atlas
    std::string prefix;
    if (!prefix_opts.empty())
      prefix = sanitize_group_name(std::string(prefix_opts[atlas_idx][0])) + "_";

    // Short label for progress display
    const std::string atlas_label = prefix.empty() ? Path::basename(nodes_path) : prefix.substr(0, prefix.size() - 1);

    // Assign streamlines to nodes in this atlas
    std::map<node_t, std::vector<uint32_t>> node_to_indices;
    assign_atlas(node_to_indices, *trx, *tck2nodes, n_streamlines, atlas_label);

    INFO(str(node_to_indices.size()) + " nodes received assignments from atlas '" + atlas_label + "'");

    // Convert node IDs to group name strings and merge into the combined map
    for (auto &[node_id, indices] : node_to_indices) {
      std::string name;
      if (lut) {
        auto it = lut->find(node_id);
        name = (it != lut->end()) ? sanitize_group_name(it->second.get_name()) : str(node_id);
      } else {
        name = str(node_id);
      }
      const std::string full_name = prefix + name;
      if (all_groups.count(full_name))
        WARN("Group name '" + full_name + "' appears in multiple atlases; results will be merged");
      auto &dest = all_groups[full_name];
      dest.insert(dest.end(), indices.begin(), indices.end());
    }

    // Ensure every LUT entry produces a group (even empty ones) so that
    // trx2connectome produces a matrix of the same size as tck2connectome.
    if (lut) {
      for (const auto &[node_id, entry] : *lut) {
        const std::string name = sanitize_group_name(entry.get_name());
        const std::string full_name = prefix + name;
        if (!all_groups.count(full_name))
          all_groups[full_name] = {}; // empty group — node has no assignments
      }
    }
  }

  // Save output TRX (copy of input including any existing metadata)
  if (input_path != output_path)
    trx->save(output_path, ZIP_CM_STORE);

  // Release mmap before writing to the file
  trx->close();
  trx.reset();

  // Append all new groups in one shot
  const std::string &target = (input_path != output_path) ? output_path : input_path;
  try {
    if (trx::is_trx_directory(target))
      trx::append_groups_to_directory(target, all_groups);
    else
      trx::append_groups_to_zip(target, all_groups);
  } catch (const std::exception &e) {
    throw Exception(std::string("Failed to append groups to TRX: ") + e.what());
  }

  INFO(str(all_groups.size()) + " groups written to " + target);
}
