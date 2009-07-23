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

DESCRIPTION = {
  "print out information about track file",
  NULL
};

ARGUMENTS = {
  Argument ("tracks", "track file", "the input track file.", AllowMultiple).type_file (),
  Argument::End
};



OPTIONS = {
  Option ("ascii", "output tracks as text", "save positions of each track in individual ascii files.")
    .append (Argument ("prefix", "file prefix", "the prefix of each file").type_string ()),
  Option::End
};




EXECUTE {

  std::vector<OptBase> opt = get_options (0); 
  uint count = 0;

  for (std::vector<ArgBase>::iterator arg = argument.begin(); arg != argument.end(); ++arg) {
    Tractography::Properties properties;
    Tractography::Reader file;
    file.open (arg->get_string(), properties);

    std::cout << "***********************************\n";
    std::cout << "  Tracks file: \"" << arg->get_string() << "\"\n";
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

    for (std::vector<RefPtr<Tractography::ROI> >::iterator i = properties.roi.begin(); i != properties.roi.end(); ++i)
      std::cout << "    ROI:                  " << (*i)->specification() << "\n";


    if (opt.size()) {
      ProgressBar::init (0, "writing track data to ascii files");
      std::vector<Point> tck;
      while (file.next (tck)) {
        std::string filename (opt[0][0].get_string());
        filename += "-000000.txt";
        std::string num (str(count));
        filename.replace (filename.size()-4-num.size(), num.size(), num);

        std::ofstream out (filename.c_str());
        if (!out) throw Exception ("error opening ascii file \"" + filename + "\": " + strerror (errno));

        for (std::vector<Point>::iterator i = tck.begin(); i != tck.end(); ++i)
          out << (*i)[0] << " " << (*i)[1] << " " << (*i)[2] << "\n";

        out.close();
          
        count++;
        ProgressBar::inc();
      }
      ProgressBar::done();
    }
  }
}
