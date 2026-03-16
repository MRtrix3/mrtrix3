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
#include <set>
#include <string>
#include <vector>

#include "command.h"
#include "file/matrix.h"
#include "file/ofstream.h"
#include "progressbar.h"
#include "types.h"

#include "connectome/connectome.h"
#include "connectome/mat2vec.h"
#include "dwi/tractography/connectome/connectome.h"
#include "dwi/tractography/connectome/mapped_track.h"
#include "dwi/tractography/connectome/matrix.h"
#include "dwi/tractography/trx_utils.h"
#include "dwi/tractography/weights.h"

using namespace MR;
using namespace App;
using namespace MR::DWI::Tractography;
using namespace MR::DWI::Tractography::Connectome;
using namespace MR::DWI::Tractography::TRX;

// clang-format off
void usage() {

  AUTHOR = "MRtrix3 contributors";

  SYNOPSIS = "Generate a connectome matrix from a TRX file whose streamlines have been assigned to groups by trxlabel";

  DESCRIPTION
  + "Reads the groups embedded in a TRX file (as created by trxlabel) and constructs a "
    "symmetric connectivity matrix where each row and column corresponds to one group (node). "
    "The value at position (i, j) reflects the number of streamlines whose endpoints were "
    "assigned to both group i and group j."

  + "Unlike tck2connectome, no parcellation image is needed: the node assignments are already "
    "encoded as groups in the TRX file. SIFT2 weights can be applied by pointing "
    "-tck_weights_in at either an external text file or a dps field name in the TRX file."

  + "The output matrix rows and columns correspond to groups in alphabetical order. "
    "Use -out_node_names to write the ordered list of group names alongside the matrix.";

  EXAMPLES
  + Example ("Default usage",
             "trxlabel tracks.trx nodes.mif tracks_labeled.trx -lut FreeSurferColorLUT.txt; "
             "trx2connectome tracks_labeled.trx connectome.csv -out_node_names node_names.txt",
             "First label the TRX file, then build the connectome. "
             "The -out_node_names file maps each row/column index to its group name.")

  + Example ("SIFT2-weighted connectome from TRX dps field",
             "trx2connectome tracks_labeled.trx connectome.csv -tck_weights_in weights",
             "If the TRX file contains a 'weights' dps field (e.g. added by tcksift2 -trx_dps), "
             "pass the field name directly to -tck_weights_in.")

  + Example ("Group by prefix (use only Desikan-Killiany groups)",
             "trx2connectome tracks_labeled.trx dk_connectome.csv -group_prefix dk",
             "When a TRX file has groups from multiple atlases, -group_prefix restricts "
             "the connectome to groups whose name starts with the given prefix. "
             "The prefix is stripped from node names in -out_node_names.");

  ARGUMENTS
  + Argument ("tracks_in",      "the input TRX tractogram with groups").type_tracks_in()
  + Argument ("connectome_out", "the output connectome matrix (CSV)").type_file_out();

  OPTIONS
  + Option ("group_prefix", "only include groups whose name begins with this prefix; "
                            "the prefix (and trailing underscore) is stripped from node names in the output")
    + Argument ("prefix").type_text()

  + MR::DWI::Tractography::TrackWeightsInOption

  + MR::DWI::Tractography::Connectome::EdgeStatisticOption

  + MR::Connectome::MatrixOutputOptions

  + Option ("out_node_names", "write the ordered list of group names (one per line) to a text file; "
                              "the i-th line corresponds to column i of the connectome matrix")
    + Argument ("path").type_file_out()

  + Option ("keep_unassigned", "include a row and column for streamlines not assigned to any pair of groups "
                               "(these appear as the first row/column in the output matrix)");

}
// clang-format on

template <typename T>
void execute(const node_t n_groups,
             const stat_edge statistic,
             const std::vector<std::vector<node_t>> &streamline_groups,
             const std::vector<float> &weights,
             const std::vector<std::string> &group_names,
             const std::string &output_path) {
  const bool keep_unassigned = !get_options("keep_unassigned").empty();
  const bool symmetric = !get_options("symmetric").empty();
  const bool zero_diagonal = !get_options("zero_diagonal").empty();

  // Matrix uses 1-indexed nodes (0 = unassigned); max valid node index = n_groups
  Matrix<T> connectome(n_groups, statistic, /*vector_output=*/false, /*track_assignments=*/false);

  {
    ProgressBar progress("Building connectome from TRX groups", streamline_groups.size());
    for (size_t i = 0; i < streamline_groups.size(); ++i) {
      const auto &grps = streamline_groups[i];
      const float w = weights.empty() ? 1.0f : weights[i];

      if (grps.empty()) {
        // Unassigned: accumulate at (0, 0) — discarded unless -keep_unassigned
        Mapped_track_nodepair mapped;
        mapped.set_track_index(i);
        mapped.set_factor(1.0f);
        mapped.set_weight(w);
        mapped.set_first_node(0);
        mapped.set_second_node(0);
        connectome(mapped);
      } else if (grps.size() == 1) {
        // One endpoint unassigned: accumulate at (0, grp)
        Mapped_track_nodepair mapped;
        mapped.set_track_index(i);
        mapped.set_factor(1.0f);
        mapped.set_weight(w);
        mapped.set_first_node(0);
        mapped.set_second_node(grps[0]);
        connectome(mapped);
      } else {
        // Two or more groups: add all unique pairs
        for (size_t a = 0; a < grps.size(); ++a) {
          for (size_t b = a + 1; b < grps.size(); ++b) {
            Mapped_track_nodepair mapped;
            mapped.set_track_index(i);
            mapped.set_factor(1.0f);
            mapped.set_weight(w);
            mapped.set_first_node(grps[a]);
            mapped.set_second_node(grps[b]);
            connectome(mapped);
          }
        }
      }
      ++progress;
    }
  }

  connectome.finalize();
  connectome.save(output_path, keep_unassigned, symmetric, zero_diagonal);

  // Write ordered node names if requested
  auto opt = get_options("out_node_names");
  if (!opt.empty()) {
    const std::string names_path(opt[0][0]);
    File::OFStream out{names_path};
    for (const auto &name : group_names)
      out << name << "\n";
  }
}

void run() {
  const std::string tracks_path = argument[0];
  const std::string output_path = argument[1];

  if (!is_trx(tracks_path))
    throw Exception("Input must be a TRX file; use trxlabel first to add group assignments");

  auto trx = load_trx(tracks_path);
  if (!trx)
    throw Exception("Failed to load TRX file: " + tracks_path);

  if (trx->groups.empty())
    throw Exception("TRX file has no groups; run trxlabel to assign streamlines to parcellation nodes first");

  // Determine which groups to include (optionally filtered by prefix)
  std::string group_prefix;
  {
    auto opt = get_options("group_prefix");
    if (!opt.empty())
      group_prefix = std::string(opt[0][0]) + "_";
  }

  // Build sorted list of group names (trx->groups is a std::map, so keys are already alphabetical)
  std::vector<std::string> group_names;
  for (const auto &[name, _] : trx->groups) {
    if (group_prefix.empty() || name.substr(0, group_prefix.size()) == group_prefix)
      group_names.push_back(name);
  }

  if (group_names.empty())
    throw Exception("No groups match the specified prefix '" + group_prefix + "'; check the group names with tckinfo");

  // Map group name → 1-based node index (for use with Matrix<T>)
  const node_t n_groups = static_cast<node_t>(group_names.size());
  std::map<std::string, node_t> group_to_id;
  for (node_t i = 0; i < n_groups; ++i)
    group_to_id[group_names[i]] = i + 1; // 1-indexed; 0 = unassigned

  // Build per-streamline group membership (invert the group → index mapping)
  const size_t n_streamlines = trx->num_streamlines();
  std::vector<std::vector<node_t>> streamline_groups(n_streamlines);

  {
    ProgressBar progress("Inverting group membership", trx->groups.size());
    for (const auto &[name, _] : trx->groups) {
      auto it = group_to_id.find(name);
      if (it == group_to_id.end()) {
        ++progress;
        continue; // group excluded by prefix filter
      }
      const node_t gid = it->second;
      // Groups are lazily loaded; use get_group_members() to trigger the mmap
      const auto *members = trx->get_group_members(name);
      if (!members) {
        ++progress;
        continue;
      }
      for (Eigen::Index row = 0; row < members->_matrix.rows(); ++row) {
        const uint32_t idx = static_cast<uint32_t>(members->_matrix(row, 0));
        if (idx < n_streamlines)
          streamline_groups[idx].push_back(gid);
      }
      ++progress;
    }
  }

  // Release TRX mmap before building connectome (reduces peak memory)
  trx->close();
  trx.reset();

  // Load per-streamline weights (from external file or TRX dps field)
  std::vector<float> weights;
  {
    auto opt = get_options("tck_weights_in");
    if (!opt.empty())
      weights = resolve_dps_weights(tracks_path, std::string(opt[0][0]));
    if (!weights.empty() && weights.size() != n_streamlines)
      throw Exception("Weight vector length (" + str(weights.size()) + ") does not match streamline count (" +
                      str(n_streamlines) + ")");
  }

  // Edge statistic
  stat_edge statistic = stat_edge::SUM;
  {
    auto opt = get_options("stat_edge");
    if (!opt.empty())
      statistic = stat_edge(static_cast<MR::App::ParsedArgument::IntType>(opt[0][0]));
  }

  // Strip prefix from group_names for the -out_node_names output
  std::vector<std::string> display_names;
  display_names.reserve(n_groups);
  for (const auto &name : group_names)
    display_names.push_back(group_prefix.empty() ? name : name.substr(group_prefix.size()));

  if (n_groups >= node_count_ram_limit) {
    INFO("Very large number of nodes detected; using single-precision floating-point storage");
    execute<float>(n_groups, statistic, streamline_groups, weights, display_names, output_path);
  } else {
    execute<double>(n_groups, statistic, streamline_groups, weights, display_names, output_path);
  }
}
