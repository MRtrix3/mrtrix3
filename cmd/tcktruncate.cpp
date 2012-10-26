/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 11/05/09.


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
  + "truncate a tracks file by selecting a subset of N tracks.";

  ARGUMENTS
  + Argument ("tracks", "the input track file.").type_file ()
  + Argument ("N", "the number of tracks to include").type_integer(1,1,INT_MAX)
  + Argument ("output", "the output track file");


  OPTIONS
  + Option ("skip",
            "skip a number of tracts from the start of file before truncating")
  + Argument ("number").type_integer(0,1,INT_MAX);
}




void run ()
{

  Options opt = get_options ("skip");
  size_t skip = 0;
  if (opt.size())
    skip = opt[0][0];
  Tractography::Properties properties;
  Tractography::Reader<float> file;
  file.open (argument[0], properties);

  size_t N = argument[1];

  if (skip + N > properties["count"].empty() ? 0 : to<int> (properties["count"]))
    throw Exception ("the number of truncated tracks plus the number of skipped tracks is larger than the total");

  Tractography::Writer<float> writer;
  writer.create (argument[2],  properties);

  std::vector<Point<float> > tck;

  ProgressBar progress ("truncating tracks...", N);
  size_t index = 0;
  while (file.next (tck) && writer.count < N) {
    index++;
    if (index < skip)
      continue;
    writer.append (tck);
    progress++;
  }
  file.close();
  writer.close();

}
