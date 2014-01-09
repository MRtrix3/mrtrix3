/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by David A. Raffelt and Robert E. Smith, 25/01/13.


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
#include "bitset.h"
#include "progressbar.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/weights.h"
#include "math/rng.h"


using namespace MR;
using namespace MR::DWI;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "truncate a tracks file by selecting a subset of N tracks.";

  ARGUMENTS
  + Argument ("tracks", "the input track file.").type_file ()
  + Argument ("N",      "the number of tracks to include").type_integer (1, 1, INT_MAX)
  + Argument ("output", "the output track file");


  OPTIONS
  + Option ("skip", "skip a number of tracks from the start of file before truncating")
    + Argument ("number").type_integer (0, 1, INT_MAX)

  + Option ("randomise", "select a random subset of tracks instead of a contiguous block")

  + Tractography::TrackWeightsInOption
  + Tractography::TrackWeightsOutOption;

}




void run ()
{

  Options opt = get_options ("skip");
  size_t skip = 0;
  if (opt.size())
    skip = opt[0][0];

  Tractography::Properties properties;
  Tractography::Reader<float> file (argument[0], properties);

  const size_t N = argument[1];

  const size_t count = properties["count"].empty() ? 0 : to<int> (properties["count"]);

  if (count && (skip + N > count))
    throw Exception ("the number of requested tracks plus the number of skipped tracks exceeds the total number of tracks in the file");

  Tractography::Writer<float> writer (argument[2], properties);

  Tractography::Streamline<float> tck;
  size_t index = 0;

  opt = get_options ("randomise");
  if (opt.size()) {

    if (!count)
      throw Exception ("cannot get random truncation of file \"" + str(argument[0]) + "\", as 'count' field is invalid; first run tckfixcount command on this file");

    BitSet selection (count);
    {
      ProgressBar progress ("selecting random subset of tracks...", N);
      Math::RNG rng;
      size_t tracks_selected = 0;
      do {
        const size_t index = rng.uniform_int (count - skip) + skip;
        if (!selection[index]) {
          selection[index] = true;
          ++tracks_selected;
        }
      } while (tracks_selected < N);
    }
    {
      ProgressBar progress ("writing selected tracks to file...", count);
      while (file (tck)) {
        if (!selection[index++])
          tck.clear();
        writer (tck);
        progress++;
      }
    }
    if (index != count)
      WARN ("'count' field in file \"" + str(argument[0]) + "\" is malformed; recommend applying tckfixcount command");

  } else {

    {
      ProgressBar progress ("truncating tracks...", N + skip);
      while (file (tck) && writer.count < N) {
        if (index++ < skip)
          tck.clear();
        writer (tck);
        progress++;
      }
      tck.clear();
      while (++index < count)
        writer (tck);
      file.close();
    }

  }

  if (writer.count != N)
    WARN ("number of tracks in output file (" + str(writer.count) + ") is less than requested (" + str(N) + "); recommend running tckfixcount command on file \"" + str(argument[0]) + "\"");

}
