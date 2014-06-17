/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt 31/01/2013

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <fstream>


#include "command.h"
#include "progressbar.h"
#include "dwi/tractography/scalar_file.h"
#include "dwi/tractography/properties.h"

using namespace MR;
using namespace MR::DWI;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "print out information about track scalar file";

  ARGUMENTS
  + Argument ("tracks", "the input track scalar file.")
  .allow_multiple()
  .type_file ();

  OPTIONS
  + Option ("count",
            "count number of tracks in file explicitly, ignoring the header")

  + Option ("ascii",
            "save values of each track scalar file in individual ascii files, with the "
            "specified prefix.")
  + Argument ("prefix");
}




void run ()
{

  Options opt = get_options ("ascii");
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
      for (std::vector<std::string>::iterator i = properties.comments.begin(); i != properties.comments.end(); ++i)
        std::cout << (i == properties.comments.begin() ? "" : "                       ") << *i << "\n";
    }

    for (std::multimap<std::string,std::string>::const_iterator i = properties.roi.begin(); i != properties.roi.end(); ++i)
      std::cout << "    ROI:                  " << i->first << " " << i->second << "\n";



    if (actual_count) {
      std::vector<float > tck;
      size_t count = 0;
      {
        ProgressBar progress ("counting tracks in file... ");
        while (file (tck)) {
          ++count;
          ++progress;
        }
      }
      std::cout << "actual count in file: " << count << "\n";
    }

    if (opt.size()) {
      ProgressBar progress ("writing track scalar data to ascii files");
      std::vector<float> tck;
      size_t count = 0;
      while (file (tck)) {
        std::string filename (opt[0][0]);
        filename += "-000000.txt";
        std::string num (str (count));
        filename.replace (filename.size()-4-num.size(), num.size(), num);

        std::ofstream out (filename.c_str());
        if (!out) throw Exception ("error opening ascii file \"" + filename + "\": " + strerror (errno));

        for (std::vector<float>::iterator i = tck.begin(); i != tck.end(); ++i)
          out << (*i) << "\n";

        out.close();

        count++;
        ++progress;
      }
    }
  }
}
