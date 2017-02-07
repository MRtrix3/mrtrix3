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
#include "thread.h"
#include "dwi/directions/file.h"

#define DEFAULT_PERMUTATIONS 1e8


using namespace MR;
using namespace App;

void usage ()
{
AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

DESCRIPTION
  + "split a set of evenly distributed directions (as generated "
  "by dirgen) into approximately uniformly distributed subsets.";

ARGUMENTS
  + Argument ("dirs", "the text file containing the directions.").type_file_in()
  + Argument ("out", "the output partitioned directions").type_file_out().allow_multiple();


OPTIONS
  + Option ("permutations", "number of permutations to try")
  +   Argument ("num").type_integer (1)

  + Option ("cartesian", "Output the directions in Cartesian coordinates [x y z] instead of [az el].");
}


typedef double value_type;
typedef Eigen::Vector3d vector3_type;



class Shared { MEMALIGN(Shared)
  public:
    Shared (const Eigen::MatrixXd& directions, size_t num_subsets, size_t target_num_permutations) :
      directions (directions), subset (num_subsets), 
      best_energy (std::numeric_limits<value_type>::max()),
      target_num_permutations (target_num_permutations),
      num_permutations (0) {
        size_t s = 0;
        for (ssize_t n = 0; n < directions.rows(); ++n) {
          subset[s++].push_back (n);
          if (s >= num_subsets) s = 0;
        }
        INFO ("split " + str(directions.rows()) + " directions into subsets with " + 
            str([&]{ vector<size_t> c; for (auto& x : subset) c.push_back (x.size()); return c; }()) + " volumes");
      }




    bool update (value_type energy, const vector<vector<size_t>>& set) 
    {
      std::lock_guard<std::mutex> lock (mutex);
      if (!progress) progress.reset (new ProgressBar ("distributing directions", target_num_permutations));
      if (energy < best_energy) {
        best_energy = energy;
        best_subset = set;
        progress->set_text ("distributing directions (current best configuration: energy = " + str(best_energy) + ")");
      }
      ++num_permutations;
      ++(*progress);
      return num_permutations < target_num_permutations;
    }



    value_type energy (size_t i, size_t j) const {
      vector3_type a = { directions(i,0), directions(i,1), directions(i,2) };
      vector3_type b = { directions(j,0), directions(j,1), directions(j,2) };
      return 1.0 / (a-b).squaredNorm() + 1.0 / (a+b).squaredNorm();
    }


    const vector<vector<size_t>>& get_init_subset () const { return subset; }
    const vector<vector<size_t>>& get_best_subset () const { return best_subset; }


  protected:
    const Eigen::MatrixXd& directions;
    std::mutex mutex;
    vector<vector<size_t>> subset, best_subset;
    value_type best_energy;
    const size_t target_num_permutations;
    size_t num_permutations;
    std::unique_ptr<ProgressBar> progress;
};







class EnergyCalculator { MEMALIGN(EnergyCalculator)
  public:
    EnergyCalculator (Shared& shared) : shared (shared), subset (shared.get_init_subset()) { }

    void execute () {
      while (eval()); 
    }


    void next_permutation ()
    {
      size_t i,j;
      std::uniform_int_distribution<size_t> dist(0, subset.size()-1);
      do {
        i = dist (rng);
        j = dist (rng);
      } while (i == j);

      size_t n_i = std::uniform_int_distribution<size_t> (0, subset[i].size()-1) (rng);
      size_t n_j = std::uniform_int_distribution<size_t> (0, subset[j].size()-1) (rng);

      std::swap (subset[i][n_i], subset[j][n_j]);
    }

    bool eval ()
    {
      next_permutation();

      value_type energy = 0.0;
      for (auto& s: subset) {
        value_type current_energy = 0.0;
        for (size_t i = 0; i < s.size(); ++i) 
          for (size_t j = i+1; j < s.size(); ++j) 
            current_energy += shared.energy (s[i], s[j]);
        energy = std::max (energy, current_energy);
      }

      return shared.update (energy, subset);
    }

  protected:
    Shared& shared;
    vector<vector<size_t>> subset;
    Math::RNG rng;
};







void run () 
{
  auto directions = DWI::Directions::load_cartesian (argument[0]);

  size_t num_subsets = argument.size() - 1;

  size_t num_permutations = get_option_value ("permutations", DEFAULT_PERMUTATIONS);

  vector<vector<size_t>> best;
  {
    Shared shared (directions, num_subsets, num_permutations);
    Thread::run (Thread::multi (EnergyCalculator (shared)), "energy eval thread");
    best = shared.get_best_subset();
  }



  bool cartesian = get_options("cartesian").size();
  for (size_t i = 0; i < best.size(); ++i) {
    Eigen::MatrixXd output (best[i].size(), 3);
    for (size_t n = 0; n < best[i].size(); ++n) 
      output.row(n) = directions.row (best[i][n]);
    DWI::Directions::save (output, argument[i+1], cartesian);
  }

}


