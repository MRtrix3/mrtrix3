/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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
#include "dwi/directions/file.h"
#include "dwi/gradient.h"
#include "file/matrix.h"
#include "math/SH.h"
#include "math/rng.h"
#include "progressbar.h"

#include <functional>
#include <random>

using namespace MR;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Reorder a set of directions to ensure near-uniformity upon truncation";

  DESCRIPTION
  + "The intent of this command is to reorder a set of gradient directions"
    " such that if a scan is terminated prematurely,"
    " at any point,"
    " the acquired directions will still be close to optimally distributed on the half-sphere.";

  ARGUMENTS
  + Argument ("input", "the input directions file").type_file_in()
  + Argument ("output", "the output directions file").type_file_out();

  OPTIONS
    + Option ("preserve", "preserve some number of directions in their position at the start of the set")
      + Argument ("num").type_integer(1)
    + DWI::Directions::cartesian_option
    + Option ("indices", "Write the indices of the reordered directions to file")
      + Argument("path").type_file_out();

}
// clang-format on

// Don't use size_t: no get/set functions for unsigned long on MacOS
using index_type = uint32_t;
using value_type = double;

std::vector<index_type>
optimise(const Eigen::MatrixXd &directions, const index_type preserve, const size_t first_volume) {
  assert(first_volume >= preserve);
  std::vector<index_type> indices;
  indices.reserve(directions.rows());
  for (index_type n = 0; n != preserve; ++n)
    indices.push_back(n);
  indices.push_back(first_volume);
  std::vector<index_type> remaining;
  remaining.reserve(directions.rows() - preserve);
  for (index_type n = preserve; n < static_cast<index_type>(directions.rows()); ++n)
    if (n != first_volume)
      remaining.push_back(n);

  while (!remaining.empty()) {
    index_type best = 0;
    value_type best_E = std::numeric_limits<value_type>::max();

    for (index_type n = 0; n < remaining.size(); ++n) {
      value_type E = 0.0;
      index_type a = remaining[n];
      for (index_type i = 0; i < indices.size(); ++i) {
        index_type b = indices[i];
        E += 1.0 / (directions.row(a) - directions.row(b)).norm();
        E += 1.0 / (directions.row(a) + directions.row(b)).norm();
      }
      if (E < best_E) {
        best_E = E;
        best = n;
      }
    }

    indices.push_back(remaining[best]);
    remaining.erase(remaining.begin() + best);
  }

  return indices;
}

value_type calc_cost(const Eigen::MatrixXd &directions, const std::vector<index_type> &order) {
  const ssize_t start = Math::SH::NforL(2);
  if (directions.rows() <= start)
    return value_type(0);
  Eigen::MatrixXd subset(start, 3);
  for (ssize_t i = 0; i != start; ++i)
    subset.row(i) = directions.row(order[i]);
  value_type cost = value_type(0);
  for (ssize_t N = start + 1; N < directions.rows(); ++N) {
    // Don't include condition numbers where precisely the number of coefficients
    //   for that spherical harmonic degree are included, as these tend to
    //   be outliers
    const ssize_t lmax = Math::SH::LforN(N - 1);
    subset.conservativeResize(N, 3);
    subset.row(N - 1) = directions.row(order[N - 1]);
    const value_type cond = DWI::condition_number_for_lmax(subset, lmax);
    cost += cond;
  }
  return cost;
}

void run() {
  auto directions = DWI::Directions::load_cartesian(argument[0]);

  const index_type preserve = get_option_value<index_type>("preserve", 0);

  index_type last_candidate_first_volume = static_cast<index_type>(directions.rows());
  if (static_cast<size_t>(directions.rows()) <= Math::SH::NforL(2)) {
    // clang-format off
    WARN("Very few directions in input (" + str(directions.rows()) + ";" +
         " selection of first direction cannot be optimised" +
         (preserve ?
              " (direction #" + str(preserve + 1) + " will be first direction in output "
              "as that is the first direction after those to be preserved)" :
              " (first direction in input will be first direction in output)"));
    // clang-format on
    last_candidate_first_volume = preserve + 1;
  }
  value_type min_cost = std::numeric_limits<value_type>::infinity();
  std::vector<index_type> best_order;
  {
    ProgressBar progress("Determining best reordering", directions.rows());
    for (index_type first_volume = preserve; first_volume != last_candidate_first_volume; ++first_volume) {
      const std::vector<index_type> order = optimise(directions, preserve, first_volume);
      const value_type cost = calc_cost(directions, order);
      if (cost < min_cost) {
        min_cost = cost;
        best_order = order;
      }
      ++progress;
    }
  }

  auto opt = get_options("indices");
  if (!opt.empty())
    File::Matrix::save_vector(best_order, opt[0][0]);

  decltype(directions) output(directions.rows(), 3);
  for (ssize_t n = 0; n < directions.rows(); ++n)
    output.row(n) = directions.row(best_order[n]);

  DWI::Directions::save(output, argument[1], !get_options("cartesian").empty());
}
