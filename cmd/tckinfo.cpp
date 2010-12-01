/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#include "app.h"
#include "progressbar.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"

using namespace MR; 
using namespace MR::DWI; 
using namespace std; 

SET_VERSION_DEFAULT;
SET_AUTHOR (NULL);
SET_COPYRIGHT (NULL);

DESCRIPTION = {
  "print out information about track file",
  NULL
};

ARGUMENTS = {
  Argument ("tracks", "the input track file.")
    .allow_multiple()
    .type_file (),

  Argument ()
};



OPTIONS = {
  Option ("ascii",
      "save positions of each track in individual ascii files, with the "
      "specified prefix.")
    + Argument ("prefix"),
  Option ()
};




EXECUTE {

  Options opt = get_options ("ascii"); 
  size_t count = 0;

  for (size_t i = 0; i < argument.size(); ++i) {
    Tractography::Properties properties;
    Tractography::Reader<float> file;
    file.open (argument[i], properties);

    std::cout << "***********************************\n";
    std::cout << "  Tracks file: \"" << argument[i] << "\"\n";
    for (Tractography::Properties::iterator i = properties.begin(); i != properties.end(); ++i) {
      std::string S (i->first + ':');
      S.resize (22, ' ');
      std::cout << "    " << S << i->second << "\n";
    }

    if (properties.comments.size()) {
      std::cout << "    Comments:             ";
      for (std::vector<std::string>::iterator i = properties.comments.begin(); i != properties.comments.end(); ++i)
        std::cout << ( i == properties.comments.begin() ? "" : "                       " ) << *i << "\n";
    }

    for (std::multimap<std::string,std::string>::const_iterator i = properties.roi.begin(); i != properties.roi.end(); ++i)
      std::cout << "    ROI:                  " << i->first << " " << i->second << "\n";


    if (opt.size()) {
      ProgressBar progress ("writing track data to ascii files");
      std::vector<Point<float> > tck;
      while (file.next (tck)) {
        std::string filename (opt[0][0]);
        filename += "-000000.txt";
        std::string num (str(count));
        filename.replace (filename.size()-4-num.size(), num.size(), num);

        std::ofstream out (filename.c_str());
        if (!out) throw Exception ("error opening ascii file \"" + filename + "\": " + strerror (errno));

        for (std::vector<Point<float> >::iterator i = tck.begin(); i != tck.end(); ++i)
          out << (*i)[0] << " " << (*i)[1] << " " << (*i)[2] << "\n";

        out.close();
          
        count++;
        ++progress;
      }
    }
  }
}
