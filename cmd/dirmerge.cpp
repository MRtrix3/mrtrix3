/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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
#include "dwi/directions/file.h"
#include "file/ofstream.h"

#include <array>
#include <random>
#include <algorithm>

using namespace MR;
using namespace App;

void usage ()
{
AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

SYNOPSIS = "Splice / merge multiple sets of directions in such a way as to maintain near-optimality upon truncation";

ARGUMENTS
  + Argument ("subsets", "the number of subsets (eg. phase encoding directions) per b-value").type_integer(1,10000)
  + Argument ("bvalue files", "the b-value and sets of corresponding files, in order").type_text().allow_multiple()
  + Argument ("out", "the output directions file, with each row listing "
      "the X Y Z gradient directions, the b-value, and an index representing "
      "the phase encode direction").type_file_out();

OPTIONS
  + Option ("unipolar_weight",
      "set the weight given to the unipolar electrostatic repulsion model compared to the "
      "bipolar model (default: 0.2).")
    + Argument ("value").type_float(0.0, 1.0);
}



using value_type = double;
using Direction = Eigen::Matrix<value_type,3,1>;
using DirectionSet = vector<Direction>;


struct OutDir { MEMALIGN(OutDir)
  Direction d;
  size_t b;
  size_t pe;
};

inline std::ostream& operator<< (std::ostream& stream, const OutDir& d) {
  stream << "[ " << d.d << "], " << d.b << ", " << d.pe << " ]";
  return stream;
}


void run ()
{
  size_t num_subsets = argument[0];
  value_type unipolar_weight = App::get_option_value ("unipolar_weight", 0.2);
  value_type bipolar_weight = 1.0 - unipolar_weight;


  vector<vector<DirectionSet>> dirs;
  vector<value_type> bvalue ((argument.size() - 2) / (1+num_subsets));
  INFO ("expecting " + str(bvalue.size()) + " b-values");
  if (bvalue.size()*(1+num_subsets) + 2 != argument.size())
    throw Exception ("inconsistent number of arguments");


  // read them in:
  size_t current = 1, nb = 0;
  while (current < argument.size()-1) {
    bvalue[nb] = to<value_type> (argument[current++]);
    vector<DirectionSet> d;
    for (size_t i = 0; i < num_subsets; ++i) {
      auto m = DWI::Directions::load_cartesian (argument[current++]);
      DirectionSet set;
      for (ssize_t r = 0; r < m.rows(); ++r)
        set.push_back (Direction (m(r,0), m(r,1), m(r,2)));
      d.push_back (set);
    }
    INFO ("found b = " + str(bvalue[nb]) + ", " +
        str ([&]{ vector<size_t> s; for (auto& n : d) s.push_back (n.size()); return s; }()) + " volumes");

    dirs.push_back (d);
    ++nb;
  }

  size_t total = [&]{ size_t n = 0; for (auto& d : dirs) for (auto& m : d) n += m.size(); return n; }();
  INFO ("found total of " + str(total) + " volumes") ;


  // pick random direction from first direction set:
  std::random_device rd;
  std::mt19937 rng (rd());
  size_t first = std::uniform_int_distribution<size_t> (0, dirs[0][0].size()-1)(rng);


  vector<OutDir> merged;

  auto push = [&](size_t b, size_t p, size_t n)
  {
    merged.push_back ({ Direction (dirs[b][p][n][0], dirs[b][p][n][1], dirs[b][p][n][2]), b, p });
    dirs[b][p].erase (dirs[b][p].begin()+n);
  };

  auto energy_pair = [&](const Direction& a, const Direction& b)
  {
    // use combination of mono- and bi-polar electrostatic repulsion models
    // to ensure adequate coverage of eddy-current space as well as
    // orientation space.
    // By default, use a moderate bias, favouring the bipolar model.

    return (unipolar_weight+bipolar_weight) / (b-a).norm()
         + bipolar_weight / (b+a).norm();
  };

  auto energy = [&](size_t b, size_t p, size_t n)
  {
    value_type E = 0.0;
    for (auto& d : merged)
      if (d.b == b)
        E += energy_pair (d.d, dirs[b][p][n]);
    return E;
  };

  auto find_lowest_energy_direction = [&](size_t b, size_t p)
  {
    size_t best = 0;
    value_type bestE = std::numeric_limits<value_type>::max();
    for (size_t n = 0; n < dirs[b][p].size(); ++n) {
      value_type E = energy (b, p, n);
      if (E < bestE) {
        bestE = E;
        best = n;
      }
    }
    return best;
  };



  vector<float> fraction;
  for (auto& d : dirs) {
    size_t n = 0;
    for (auto& m : d)
      n += m.size();
    fraction.push_back (float (n) / float (total));
  };

  push (0, 0, first);

  vector<size_t> counts (bvalue.size(), 0);
  ++counts[0];

  auto num_for_b = [&](size_t b) {
    size_t n = 0;
    for (auto& d : merged)
      if (d.b == b)
        ++n;
    return n;
  };



  size_t nPE = num_subsets > 1 ? 1 : 0;
  while (merged.size() < total) {
    // find shell with shortfall in numbers:
    size_t b = 0, n;
    value_type fraction_diff = std::numeric_limits<value_type>::max();
    for (n = 0; n < bvalue.size(); ++n) {
      value_type f_diff = float(num_for_b(n)) / float (merged.size()) - fraction[n];
      if (f_diff < fraction_diff && dirs[n][nPE].size()) {
        fraction_diff = f_diff;
        b = n;
      }
    }

    // find most distant direction for that shell & in the current PE direction:
    n = find_lowest_energy_direction (b, nPE);
    if (dirs[b][nPE].size())
      push (b, nPE, n);
    else
      WARN ("no directions remaining in b=" + str (bvalue[b]) + " shell for PE direction " + str(n) + " - PE directions will not cycle through perfectly");

    // update PE direction
    ++nPE;
    if (nPE >= num_subsets)
      nPE = 0;
  }





  // write-out:

  File::OFStream out (argument[argument.size()-1]);
  for (auto& d : merged)
    out << MR::printf ("%#20.15f %#20.15f %#20.15f %5d %3d\n",
        d.d[0], d.d[1], d.d[2],
        int (bvalue[d.b]), int (d.pe+1));

}



