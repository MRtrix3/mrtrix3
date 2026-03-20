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

#include "command.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/trx_utils.h"
#include "file/ofstream.h"
#include "progressbar.h"

using namespace MR;
using namespace MR::DWI;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Print out information about a track file";

  ARGUMENTS
  + Argument ("tracks", "the input track file.").type_tracks_in().type_file_in().type_directory_in().allow_multiple();

  OPTIONS
  + Option ("count", "count number of tracks in file explicitly, ignoring the header")
  + Option ("prefix_depth", "for TRX files, collapse groups by the first N underscore-delimited "
                            "tokens of their name. Defaults to 1, which groups by atlas name prefix. "
                            "Use 0 to list all groups individually, or higher values for finer detail.")
    + Argument ("N").type_integer(0);

}
// clang-format on

void run() {
  const bool actual_count = !get_options("count").empty();
  const auto prefix_depth_opt = get_options("prefix_depth");
  const bool prefix_depth_specified = !prefix_depth_opt.empty();
  const int prefix_depth = prefix_depth_specified ? int(prefix_depth_opt[0][0]) : 1;

  for (size_t i = 0; i < argument.size(); ++i) {
    std::cout << "***********************************\n";
    std::cout << "  Tracks file: \"" << argument[i] << "\"\n";

    if (Tractography::TRX::is_trx(argument[i])) {
      auto trx = Tractography::TRX::load_trx_header_only(argument[i]);
      if (!trx)
        throw Exception("Failed to load TRX file: " + std::string(argument[i]));
      Tractography::TRX::print_info(std::cout, *trx, prefix_depth, !prefix_depth_specified);
      trx->close();
      continue;
    }

    Tractography::Properties properties;
    Tractography::Reader<float> file(argument[i], properties);

    for (Tractography::Properties::iterator i = properties.begin(); i != properties.end(); ++i) {
      std::string S(i->first + ':');
      S.resize(22, ' ');
      const auto lines = split_lines(i->second);
      std::cout << "    " << S << lines[0] << "\n";
      for (size_t i = 1; i != lines.size(); ++i)
        std::cout << "                          " << lines[i] << "\n";
    }

    if (!properties.comments.empty()) {
      std::cout << "    Comments:             ";
      for (std::vector<std::string>::iterator i = properties.comments.begin(); i != properties.comments.end(); ++i)
        std::cout << (i == properties.comments.begin() ? "" : "                       ") << *i << "\n";
    }

    for (std::multimap<std::string, std::string>::const_iterator i = properties.prior_rois.begin();
         i != properties.prior_rois.end();
         ++i)
      std::cout << "    ROI:                  " << i->first << " " << i->second << "\n";

    if (actual_count) {
      Tractography::Streamline<float> tck;
      size_t count = 0;
      {
        ProgressBar progress("counting tracks in file");
        while (file(tck)) {
          ++count;
          ++progress;
        }
      }
      std::cout << "actual count in file: " << count << "\n";
    }
  }
}
