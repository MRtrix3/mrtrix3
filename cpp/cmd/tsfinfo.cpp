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
#include "dwi/tractography/properties.h"
#include "dwi/tractography/scalar_file.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/trx_utils.h"
#include "file/ofstream.h"
#include "progressbar.h"

using namespace MR;
using namespace MR::DWI;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Print out information about a track scalar file";

  DESCRIPTION
  + "Accepts .tsf track scalar files or TRX files (.trx). "
    "For TRX files, all dpv (data-per-vertex) fields are listed. "
    "Use -field to restrict output (including -ascii export) to one field.";

  ARGUMENTS
  + Argument ("tracks", "the input track scalar file or TRX tractogram.").allow_multiple().type_file_in();

  OPTIONS
  + Option ("count", "count number of tracks in file explicitly, ignoring the header")

  + Option ("field", "for TRX input: the name of the dpv field to inspect or export with -ascii")
    + Argument ("name").type_text()

  + Option ("ascii", "save values of each track scalar file in individual ascii files,"
                     " with the specified prefix.")
    + Argument ("prefix").type_text();
}
// clang-format on

void run() {

  bool actual_count = !get_options("count").empty();
  auto field_opt = get_options("field");
  const std::string field_name = field_opt.empty() ? "" : std::string(field_opt[0][0]);

  for (size_t i = 0; i < argument.size(); ++i) {
    const std::string path(argument[i]);

    std::cout << "***********************************\n";
    std::cout << "  Track scalar file: \"" << path << "\"\n";

    if (DWI::Tractography::TRX::is_trx(path)) {
      // TRX mode: list dpv fields
      auto trx = DWI::Tractography::TRX::load_trx(path);
      if (!trx)
        throw Exception("Failed to load TRX file: " + path);

      std::cout << "    Format:               TRX\n";
      std::cout << "    Streamlines:          " << trx->num_streamlines() << "\n";
      std::cout << "    Total vertices:       " << trx->num_vertices() << "\n";

      if (trx->data_per_vertex.empty()) {
        std::cout << "    No dpv fields found.\n";
      } else {
        std::cout << "    DPV fields (" << trx->data_per_vertex.size() << "):\n";
        for (const auto &kv : trx->data_per_vertex) {
          if (!field_name.empty() && kv.first != field_name)
            continue;
          const size_t nv = kv.second ? static_cast<size_t>(kv.second->_data.rows()) : 0;
          const size_t nc = kv.second ? static_cast<size_t>(kv.second->_data.cols()) : 0;
          std::cout << "      " << kv.first << ": " << nv << " vertices x " << nc << " col(s)\n";
        }
      }

      if (actual_count) {
        // For TRX the streamline count is in the header
        std::cout << "actual count in file: " << trx->num_streamlines() << "\n";
      }

      auto opt = get_options("ascii");
      if (!opt.empty()) {
        if (field_name.empty())
          throw Exception("Use -field to specify which dpv field to export to ASCII for TRX input");
        DWI::Tractography::TRX::TRXScalarReader reader(path, field_name);
        DWI::Tractography::TrackScalar<float> tck;
        ProgressBar progress("writing dpv data to ascii files");
        while (reader(tck)) {
          std::string filename(opt[0][0]);
          filename += "-000000.txt";
          std::string num(str(tck.get_index()));
          filename.replace(filename.size() - 4 - num.size(), num.size(), num);
          File::OFStream out(filename);
          for (float v : tck)
            out << v << "\n";
          out.close();
          ++progress;
        }
      }

    } else {
      // TSF mode: existing behaviour
      Tractography::Properties properties;
      Tractography::ScalarReader<float> file(path, properties);

      for (Tractography::Properties::iterator it = properties.begin(); it != properties.end(); ++it) {
        std::string S(it->first + ':');
        S.resize(22, ' ');
        std::cout << "    " << S << it->second << "\n";
      }

      if (!properties.comments.empty()) {
        std::cout << "    Comments:             ";
        for (std::vector<std::string>::iterator it = properties.comments.begin(); it != properties.comments.end(); ++it)
          std::cout << (it == properties.comments.begin() ? "" : "                       ") << *it << "\n";
      }

      for (std::multimap<std::string, std::string>::const_iterator it = properties.prior_rois.begin();
           it != properties.prior_rois.end();
           ++it)
        std::cout << "    ROI:                  " << it->first << " " << it->second << "\n";

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
        ProgressBar progress("writing track scalar data to ascii files");
        DWI::Tractography::TrackScalar<> tck;
        while (file(tck)) {
          std::string filename(opt[0][0]);
          filename += "-000000.txt";
          std::string num(str(tck.get_index()));
          filename.replace(filename.size() - 4 - num.size(), num.size(), num);

          File::OFStream out(filename);
          for (std::vector<float>::iterator it = tck.begin(); it != tck.end(); ++it)
            out << (*it) << "\n";
          out.close();

          ++progress;
        }
      }
    }
  }
}
