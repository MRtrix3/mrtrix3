/* Copyright (c) 2008-2019 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
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

  SYNOPSIS = "Reorder a set of directions to ensure near-uniformity upon truncation - "
             "i.e. if the scan is terminated early, the acquired directions are still "
             "close to optimal";

  ARGUMENTS
    + Argument ("input", "the input directions file").type_file_in()
    + Argument ("output", "the output directions file").type_file_out();

  OPTIONS
    + Option ("cartesian", "Output the directions in Cartesian coordinates [x y z] instead of [az el].");
}


using value_type = double;


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
        E += 1.0 / (directions.row(a) - directions.row(b)).norm();
        E += 1.0 / (directions.row(a) + directions.row(b)).norm();
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





