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

#include <filesystem>
#include <iomanip>
#include <sstream>

#include "command.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/scalar_file.h"
#include "dwi/tractography/streamline.h"
#include "file/ofstream.h"
#include "progressbar.h"

using namespace MR;
using namespace MR::DWI;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Print out information about a track scalar file";

  ARGUMENTS
  + Argument ("tracks", "the input track scalar file.").allow_multiple().type_file_in();

  OPTIONS
  + Option ("count", "count number of tracks in file explicitly, ignoring the header")

  + Option ("ascii", "save values of each track scalar file in individual ascii files"
                     " within the specified output directory;"
                     " each file is named by the zero-padded track index")
    + Argument ("dir").type_directory_out(DirOutMode::EmptyOrAbsent);
}
// clang-format on

void run() {

  bool const actual_count = !get_options("count").empty();

  for (const auto &i : argument) {
    Tractography::Properties properties;
    Tractography::ScalarReader<float> file(i, properties);

    std::cout << "***********************************\n";
    std::cout << "  Track scalar file: \"" << i.as_text() << "\"\n";

    for (auto &propertie : properties) {
      std::string S(propertie.first + ':');
      S.resize(22, ' ');
      std::cout << "    " << S << propertie.second << "\n";
    }

    if (!properties.comments.empty()) {
      std::cout << "    Comments:             ";
      for (auto i = properties.comments.begin(); i != properties.comments.end(); ++i)
        std::cout << (i == properties.comments.begin() ? "" : "                       ") << *i << "\n";
    }

    for (auto &prior_roi : properties.prior_rois)
      std::cout << "    ROI:                  " << prior_roi.first << " " << prior_roi.second << "\n";

    if (actual_count) {
      DWI::Tractography::TrackScalar<> tck;
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

    auto opt = get_options("ascii");
    if (!opt.empty()) {
      const std::filesystem::path ascii_dir(opt[0][0]);
      std::filesystem::create_directories(ascii_dir);
      ProgressBar progress("writing track scalar data to ascii files");
      DWI::Tractography::TrackScalar<> tck;
      while (file(tck)) {
        std::ostringstream index_str;
        index_str << std::setfill('0') << std::setw(6) << tck.get_index();
        File::OFStream out(ascii_dir / (index_str.str() + ".txt"));
        for (float &i : tck)
          out << i << "\n";
        out.close();
        ++progress;
      }
    }
  }
}
