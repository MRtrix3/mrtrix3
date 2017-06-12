/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#include "command.h"
#include "progressbar.h"
#include "file/ofstream.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"

using namespace MR;
using namespace MR::DWI;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Print out information about a track file";

  ARGUMENTS
  + Argument ("tracks", "the input track file.").type_tracks_in().allow_multiple();

  OPTIONS
  + Option ("count", "count number of tracks in file explicitly, ignoring the header");

}




void run ()
{
  bool actual_count = get_options ("count").size();

  for (size_t i = 0; i < argument.size(); ++i) {
    Tractography::Properties properties;
    Tractography::Reader<float> file (argument[i], properties);

    std::cout << "***********************************\n";
    std::cout << "  Tracks file: \"" << argument[i] << "\"\n";

    for (Tractography::Properties::iterator i = properties.begin(); i != properties.end(); ++i) {
      std::string S (i->first + ':');
      S.resize (22, ' ');
      std::cout << "    " << S << i->second << "\n";
    }

    if (properties.comments.size()) {
      std::cout << "    Comments:             ";
      for (vector<std::string>::iterator i = properties.comments.begin(); i != properties.comments.end(); ++i)
        std::cout << (i == properties.comments.begin() ? "" : "                       ") << *i << "\n";
    }

    for (std::multimap<std::string,std::string>::const_iterator i = properties.roi.begin(); i != properties.roi.end(); ++i)
      std::cout << "    ROI:                  " << i->first << " " << i->second << "\n";



    if (actual_count) {
      Tractography::Streamline<float> tck;
      size_t count = 0;
      {
        ProgressBar progress ("counting tracks in file");
        while (file (tck)) {
          ++count;
          ++progress;
        }
      }
      std::cout << "actual count in file: " << count << "\n";
    }


  }
}
