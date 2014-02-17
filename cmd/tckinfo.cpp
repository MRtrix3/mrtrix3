#include <fstream>

#include "command.h"
#include "progressbar.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"


using namespace MR;
using namespace MR::DWI;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "print out information about track file";

  ARGUMENTS
  + Argument ("tracks", "the input track file.")
  .allow_multiple()
  .type_file ();

  OPTIONS
  + Option ("count",
            "count number of tracks in file explicitly, ignoring the header")

  + Option ("ascii",
            "save positions of each track in individual ascii files, with the "
            "specified prefix.")
  + Argument ("prefix");
}




void run ()
{

  Options opt = get_options ("ascii");
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

    std::cout.precision (properties.timestamp_precision);
    std::string S ("timestamp:");
    S.resize (22, ' ');
    std::cout << "    " << S << properties.timestamp << "\n";

    if (properties.comments.size()) {
      std::cout << "    Comments:             ";
      for (std::vector<std::string>::iterator i = properties.comments.begin(); i != properties.comments.end(); ++i)
        std::cout << (i == properties.comments.begin() ? "" : "                       ") << *i << "\n";
    }

    for (std::multimap<std::string,std::string>::const_iterator i = properties.roi.begin(); i != properties.roi.end(); ++i)
      std::cout << "    ROI:                  " << i->first << " " << i->second << "\n";


    if (actual_count) {
      Tractography::Streamline<float> tck;
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
      ProgressBar progress ("writing track data to ascii files");
      Tractography::Streamline<float> tck;
      size_t count = 0;
      while (file (tck)) {
        std::string filename (opt[0][0]);
        filename += "-000000.txt";
        std::string num (str (count));
        filename.replace (filename.size()-4-num.size(), num.size(), num);

        std::ofstream out (filename.c_str());
        if (!out) throw Exception ("error opening ascii file \"" + filename + "\": " + strerror (errno));

        for (std::vector<Point<float> >::iterator i = tck.begin(); i != tck.end(); ++i)
          out << (*i) [0] << " " << (*i) [1] << " " << (*i) [2] << "\n";

        out.close();

        count++;
        ++progress;
      }
    }
  }
}
