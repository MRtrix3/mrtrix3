/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 12/12/12 (at the end of the world).


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

MRTRIX_APPLICATION

using namespace MR;
using namespace MR::DWI;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "load a track scalar file and threshold the values.";

  ARGUMENTS
  + Argument ("tracks", "the input track scalar file").type_file ()
  + Argument ("threshold", "the scalar threshold value").type_float()
  + Argument ("output", "the output track scalar file").type_file ();

  OPTIONS
  + Option ("invert", "invert the output");
}



void run ()
{
  Options opt = get_options ("invert");
  bool invert = opt.size() ? true : false;

  Tractography::Properties properties;
  Tractography::Reader<float> file;
  file.open (argument[0], properties);

  float threshold = argument[1];

  Tractography::Writer<float> writer;
  writer.create (argument[2],  properties);

  std::vector<Point<float> > tck;

  const size_t num_tracks = properties["count"].empty() ? 0 : to<int> (properties["count"]);
  if (!num_tracks)
    throw Exception ("error with track count in input file");

  ProgressBar progress ("thresholding...", num_tracks);

  while (file.next (tck)) {
    std::vector<Point<float> > tck_mask;
    for (size_t p = 0; p < tck.size(); ++p) {
      Point<float> out;
      for (size_t i = 0; i < 3; ++i) {
        if (tck[p][i] != NAN) {
          if (tck[p][i] > threshold) {
            if (invert)
              out[i] = 0.0;
            else
              out[i] = 1.0;
          }
          else {
            if (invert)
              out[i] = 1.0;
            else
              out[i] = 0.0;
          }
        }
      }
      tck_mask.push_back (out);
    }
    writer.append (tck);
    progress++;
  }
  file.close();
  writer.close();
}
