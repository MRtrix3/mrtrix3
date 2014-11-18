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

#include "command.h"
#include "progressbar.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "math/rng.h"
#include "point.h"
#include "dwi/directions/file.h"
#include "file/ofstream.h"

#include <array>
#include <random>
#include <algorithm>

using namespace MR;
using namespace App;

void usage () {

DESCRIPTION
  + "splice or merge sets of directions over multiple shells into a single set, "
    "in such a way as to maintain near-optimality upon truncation.";

ARGUMENTS
  + Argument ("subsets", "the number of subsets (phase-encode directions) per b-value").type_integer(1,4,10000)
  + Argument ("bvalue files", "the b-value and sets of corresponding files, in order").type_text().allow_multiple()
  + Argument ("out", "the output directions file, with each row listing "
      "the X Y Z gradient directions, the b-value, and an index representing "
      "the phase encode direction").type_file_out();
}


typedef double value_type;



void run () 
{
  size_t num_subsets = argument[0];

  std::vector<std::vector<Math::Matrix<value_type>>> dirs;
  std::vector<value_type> bvalue ((argument.size() - 2) / (1+num_subsets));
  INFO ("expecting " + str(bvalue.size()) + " b-values");
  if (bvalue.size()*(1+num_subsets) + 2 != argument.size())
    throw Exception ("inconsistent number of arguments");


  // read them in:
  size_t current = 1, n = 0;
  while (current < argument.size()-1) {
    bvalue[n] = to<value_type> (argument[current++]);
    std::vector<Math::Matrix<value_type>> d;
    for (size_t i = 0; i < num_subsets; ++i) 
      d.push_back (DWI::Directions::load_cartesian<value_type> (argument[current++]));
    INFO ("found b = " + str(bvalue[n]) + ", " + 
        str ([&]{ std::vector<size_t> s; for (auto& n : d) s.push_back (n.rows()); return s; }()) + " volumes");

    dirs.push_back (d);
    ++n;
  }


  // TODO: need to adjust PE binning to ensure even PE cycling:







  n = 0;
  std::vector<std::vector<std::array<value_type, 4>>> merged_dirs (num_subsets);
  for (size_t b = 0; b < bvalue.size(); ++b) {
    size_t x = 0;
    size_t s = 0;
    while (true) {
      auto& d = dirs[b][s++];
      if (x >= d.rows()) break;

      merged_dirs[n++].push_back ({ { d(x,0), d(x,1), d(x,2), bvalue[b] } });

      if (s >= num_subsets) { 
        s = 0;
        ++x;
      }
      if (n >= num_subsets) 
        n = 0;
    }
  }
  INFO ("reordered into " + str (num_subsets) + " sets of " + 
      str ([&]{ std::vector<size_t> s; for (auto& n : merged_dirs) s.push_back (n.size()); return s; }()) + " volumes");


  // randomise:
  std::random_device rd;
  std::mt19937 g (rd());
  for (auto& s: merged_dirs)
    std::shuffle (s.begin(), s.end(), g);





  // write-out:
  
  File::OFStream out (argument[argument.size()-1]);
  size_t s = 0;
  n = 0;
  while (true) {
    if (n >= merged_dirs[s].size())
      break;
    out << MR::printf ("%#10f %#10f %#10f %5d %3d\n", 
        float (merged_dirs[s][n][0]), float (merged_dirs[s][n][1]), float (merged_dirs[s][n][2]), 
        int (merged_dirs[s][n][3]), int (s+1));
    ++s;
    if (s >= num_subsets) {
      s = 0;
      ++n;
    }
  }


}



