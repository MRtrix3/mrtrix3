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
#include "math/rng.h"
#include "math/SH.h"
#include "dwi/directions/file.h"

#include <random>
#include <functional>

using namespace MR;
using namespace App;

void usage () {

  DESCRIPTION
    + "reorder a set of directions to ensure near-uniformity upon truncation - "
    "i.e. if the scan is terminated early, the acquired directions are still "
    "close to optimal";

  ARGUMENTS
    + Argument ("input", "the input directions file").type_file_in()
    + Argument ("output", "the output directions file").type_file_out();

  OPTIONS
    + Option ("cartesian", "Output the directions in Cartesian coordinates [x y z] instead of [az el].");
}


typedef double value_type;


  template <typename value_type>
inline std::function<value_type()> get_rng_uniform (value_type from, value_type to) 
{
  std::random_device rd;
  std::mt19937 gen (rd());
  std::uniform_int_distribution<value_type> dis (from, to);
  return std::bind (dis, gen);
}


void run () 
{
  auto directions = DWI::Directions::load_cartesian (argument[0]);
  auto rng = get_rng_uniform<size_t> (0, directions.rows()-1);

  std::vector<ssize_t> indices (1, rng());
  std::vector<ssize_t> remaining;
  for (ssize_t n = 0; n < directions.rows(); ++n)
    if (n != indices[0])
      remaining.push_back (n);

  while (remaining.size()) {
    ssize_t best = 0;
    value_type best_E = std::numeric_limits<value_type>::max();

    for (size_t n = 0; n < remaining.size(); ++n) {
      value_type E = 0.0;
      ssize_t a = remaining[n];
      for (size_t i = 0; i < indices.size(); ++i) {
        ssize_t b = indices[i];
        E += 1.0 / (
            Math::pow2 (directions(a,0)-directions(b,0)) + 
            Math::pow2 (directions(a,1)-directions(b,1)) + 
            Math::pow2 (directions(a,2)-directions(b,2)));
        E += 1.0 / (
            Math::pow2 (directions(a,0)+directions(b,0)) + 
            Math::pow2 (directions(a,1)+directions(b,1)) + 
            Math::pow2 (directions(a,2)+directions(b,2)));
      }
      if (E < best_E) {
        best_E = E;
        best = n;
      }
    }

    indices.push_back (remaining[best]);
    remaining.erase (remaining.begin()+best);
  }


  decltype(directions) output (directions.rows(), 3);
  for (ssize_t n = 0; n < directions.rows(); ++n)
    output.row(n) = directions.row (indices[n]);

  DWI::Directions::save (output, argument[1], get_options("cartesian").size());
}





