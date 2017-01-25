/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
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

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

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

  vector<ssize_t> indices (1, rng());
  vector<ssize_t> remaining;
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





