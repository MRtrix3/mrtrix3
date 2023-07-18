/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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
#include "dwi/gradient.h"
#include "dwi/directions/file.h"

#include <random>
#include <functional>

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Reorder a set of directions to ensure near-uniformity upon truncation";

  DESCRIPTION
  + "The intent of this command is to reorder a set of gradient directions such that "
    "if a scan is terminated prematurely, at any point, the acquired directions will "
    "still be close to optimally distributed on the half-sphere.";

  ARGUMENTS
    + Argument ("input", "the input directions file").type_file_in()
    + Argument ("output", "the output directions file").type_file_out();

  OPTIONS
    + Option ("cartesian", "Output the directions in Cartesian coordinates [x y z] instead of [az el].");
}



using value_type = double;



vector<size_t> optimise (const Eigen::MatrixXd& directions, const size_t first_volume)
{
  vector<size_t> indices (1, first_volume);
  vector<size_t> remaining;
  for (size_t n = 0; n < size_t(directions.rows()); ++n)
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

  return indices;
}




value_type calc_cost (const Eigen::MatrixXd& directions, const vector<size_t>& order)
{
  const size_t start = Math::SH::NforL (2);
  if (size_t(directions.rows()) <= start)
    return value_type(0);
  Eigen::MatrixXd subset (start, 3);
  for (size_t i = 0; i != start; ++i)
    subset.row(i) = directions.row(order[i]);
  value_type cost = value_type(0);
  for (size_t N = start+1; N < size_t(directions.rows()); ++N) {
    // Don't include condition numbers where precisely the number of coefficients
    //   for that spherical harmonic degree are included, as these tend to
    //   be outliers
    const size_t lmax = Math::SH::LforN (N-1);
    subset.conservativeResize (N, 3);
    subset.row(N-1) = directions.row(order[N-1]);
    const value_type cond = DWI::condition_number_for_lmax (subset, lmax);
    cost += cond;
  }
  return cost;
}




void run ()
{
  auto directions = DWI::Directions::load_cartesian (argument[0]);

  size_t last_candidate_first_volume = directions.rows();
  if (size_t(directions.rows()) <= Math::SH::NforL (2)) {
    WARN ("Very few directions in input ("
          + str(directions.rows())
          + "); selection of first direction cannot be optimised"
          + " (first direction in input will be first direction in output)");
    last_candidate_first_volume = 1;
  }

  value_type min_cost = std::numeric_limits<value_type>::infinity();
  vector<size_t> best_order;
  ProgressBar progress ("Determining best reordering", directions.rows());
  for (size_t first_volume = 0; first_volume != last_candidate_first_volume; ++first_volume) {
    const vector<size_t> order = optimise (directions, first_volume);
    const value_type cost = calc_cost (directions, order);
    if (cost < min_cost) {
      min_cost = cost;
      best_order = order;
    }
    ++progress;
  }

  decltype(directions) output (directions.rows(), 3);
  for (ssize_t n = 0; n < directions.rows(); ++n)
    output.row(n) = directions.row (best_order[n]);

  DWI::Directions::save (output, argument[1], get_options("cartesian").size());
}





