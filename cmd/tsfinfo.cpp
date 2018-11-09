/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "command.h"
#include "progressbar.h"
#include "file/ofstream.h"
#include "dwi/tractography/scalar_file.h"
#include "dwi/tractography/properties.h"

using namespace MR;
using namespace MR::DWI;
using namespace App;

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  SYNOPSIS = "Print out information about a track scalar file";

  ARGUMENTS
  + Argument ("tracks", "the input track scalar file.")
  .allow_multiple()
  .type_file_in();

  OPTIONS
  + Option ("count",
            "count number of tracks in file explicitly, ignoring the header")

  + Option ("ascii",
            "save values of each track scalar file in individual ascii files, with the "
            "specified prefix.")
  + Argument ("prefix").type_text();
}




void run ()
{

  auto opt = get_options ("ascii");
  bool actual_count = get_options ("count").size();

  for (size_t i = 0; i < argument.size(); ++i) {
    Tractography::Properties properties;
    Tractography::ScalarReader<float> file (argument[i], properties);

    std::cout << "***********************************\n";
    std::cout << "  Track scalar file: \"" << argument[i] << "\"\n";

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
      vector<float > tck;
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

    if (opt.size()) {
      ProgressBar progress ("writing track scalar data to ascii files");
      vector<float> tck;
      size_t count = 0;
      while (file (tck)) {
        std::string filename (opt[0][0]);
        filename += "-000000.txt";
        std::string num (str (count));
        filename.replace (filename.size()-4-num.size(), num.size(), num);

        File::OFStream out (filename);
        for (vector<float>::iterator i = tck.begin(); i != tck.end(); ++i)
          out << (*i) << "\n";
        out.close();

        count++;
        ++progress;
      }
    }
  }
}
