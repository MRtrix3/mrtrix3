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
#include "thread.h"
#include "point.h"



using namespace MR;
using namespace App;

void usage () {

DESCRIPTION
  + "distribute a set of evenly distributed directions (as generated "
  "by gendir) evenly between N subsets.";

ARGUMENTS
  + Argument ("dirs", "the text file containing the directions.").type_file_in()
  + Argument ("num", "the number of subsets into which to partition the directions").type_integer(1,4,10000)
  + Argument ("out", "the prefix for the output partitioned directions").type_text();


OPTIONS
  + Option ("permutations", "number of permutations to try")
  +   Argument ("num").type_integer (1, 1e8);
}


typedef double value_type;



class Shared {
  public:
    Shared (const Math::Matrix<value_type>& directions, size_t num_subsets, size_t target_num_permutations) :
      directions (directions), subset (num_subsets), 
      best_energy (std::numeric_limits<value_type>::max()),
      target_num_permutations (target_num_permutations),
      num_permutations (0) {
        size_t s = 0;
        for (size_t n = 0; n < directions.rows(); ++n) {
          subset[s++].push_back (n);
          if (s >= num_subsets) s = 0;
        }
        INFO ("split " + str(directions.rows()) + " directions into subsets with " + 
            str([&]{ std::vector<size_t> c; for (auto& x : subset) c.push_back (x.size()); return c; }()) + " volumes");
      }




    bool update (value_type energy, const std::vector<std::vector<size_t>>& set) 
    {
      std::lock_guard<std::mutex> lock (mutex);
      if (!progress) progress = new ProgressBar ("distributing directions...", target_num_permutations);;
      if (energy < best_energy) {
        best_energy = energy;
        best_subset = set;
        progress->set_message ("distributing directions (current best configuration: energy = " + str(best_energy) + ")...");
      }
      ++num_permutations;
      ++(*progress);
      return num_permutations < target_num_permutations;
    }



    value_type energy (size_t i, size_t j) const {
      Point<value_type> a = { directions(i,0), directions(i,1), directions(i,2) };
      Point<value_type> b = { directions(j,0), directions(j,1), directions(j,2) };
      return 1.0 / (a-b).norm2() + 1.0 / (a+b).norm2();
    }


    const std::vector<std::vector<size_t>>& get_init_subset () const { return subset; }
    const std::vector<std::vector<size_t>>& get_best_subset () const { return best_subset; }


  protected:
    const Math::Matrix<value_type>& directions;
    std::mutex mutex;
    std::vector<std::vector<size_t>> subset, best_subset;
    value_type best_energy;
    const size_t target_num_permutations;
    size_t num_permutations;
    Ptr<ProgressBar> progress;
};







class EnergyCalculator {
  public:
    EnergyCalculator (Shared& shared) : shared (shared), subset (shared.get_init_subset()) { }

    void execute () {
      while (eval()); 
    }


    void next_permutation ()
    {
      size_t i,j;
      do {
        i = rng.uniform_int (subset.size());
        j = rng.uniform_int (subset.size());
      } while (i == j);

      size_t n_i = rng.uniform_int (subset[i].size());
      size_t n_j = rng.uniform_int (subset[j].size());

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
    std::vector<std::vector<size_t>> subset;
    Math::RNG rng;
};












class EddyShared {
  public:
    EddyShared (const Math::Matrix<value_type>& directions, size_t target_num_permutations) :
      directions (directions), target_num_permutations (target_num_permutations), num_permutations(0),
      progress ("flipping directions...", target_num_permutations),
      best_signs (directions.rows(), 1), best_eddy (std::numeric_limits<value_type>::max()) { }

    bool update (value_type eddy, const std::vector<int>& signs) 
    {
      std::lock_guard<std::mutex> lock (mutex);
      if (eddy < best_eddy) {
        best_eddy = eddy;
        best_signs = signs;
        progress.set_message ("flipping directions (current best configuration: eddy = " + str(best_eddy) + ")...");
      }
      ++num_permutations;
      ++progress;
      return num_permutations < target_num_permutations;
    }



    value_type eddy (size_t i, size_t j, const std::vector<int>& signs) const {
      Point<value_type> a = { directions(i,0), directions(i,1), directions(i,2) };
      Point<value_type> b = { directions(j,0), directions(j,1), directions(j,2) };
      if (signs[i] < 0) a = -a;
      if (signs[j] < 0) b = -b;
      return 1.0 / (a-b).norm2();
    }


    std::vector<int> get_init_signs () const { return std::vector<int> (directions.rows(), 1); }
    const std::vector<int>& get_best_signs () const { return best_signs; }


  protected:
    const Math::Matrix<value_type>& directions;
    const size_t target_num_permutations;
    size_t num_permutations;
    ProgressBar progress;
    std::vector<int> best_signs;
    value_type best_eddy;
    std::mutex mutex;
  
};





class EddyCalculator {
  public:
    EddyCalculator (EddyShared& shared) :
      shared (shared),
      signs (shared.get_init_signs()) { }

    void execute () {
      while (eval()); 
    }


    void next_permutation ()
    {
      signs[rng.uniform_int (signs.size())] *= -1;
    }

    bool eval ()
    {
      next_permutation();

      value_type eddy = 0.0;
      for (size_t i = 0; i < signs.size(); ++i) 
        for (size_t j = i+1; j < signs.size(); ++j) 
          eddy += shared.eddy (i, j, signs);

      return shared.update (eddy, signs);
    }

  protected:
    EddyShared& shared;
    std::vector<int> signs;
    Math::RNG rng;
};









void run () {

  Math::Matrix<value_type> directions;
  directions.load (argument[0]);

  size_t num_subsets = argument[1];
  size_t num_permutations = 1e8;
  Options opt = get_options ("permutations");
  if (opt.size())
    num_permutations = opt[0][0];

  std::vector<std::vector<size_t>> best;
  {
    Shared shared (directions, num_subsets, num_permutations);
    Thread::run (Thread::multi (EnergyCalculator (shared)), "energy eval thread");
    best = shared.get_best_subset();
  }


  if (num_subsets == 2 || num_subsets == 4) {
    for (size_t a = 0; a < num_subsets; a += 2) {
      Math::Matrix<value_type> dirs (best[a].size() + best[a+1].size(), 3);
      size_t n = 0;
      for (size_t x = 0; x < best[a].size(); ++x, ++n)
        dirs.row(n) = directions.row (best[a][x]);
      for (size_t x = 0; x < best[a+1].size(); ++x, ++n) {
        dirs.row(n) = directions.row (best[a+1][x]);
        dirs.row(n) *= -1.0;
      }

      EddyShared eddy_shared (dirs, num_permutations);
      Thread::run (Thread::multi (EddyCalculator (eddy_shared)), "eddy eval thread");

      auto& signs = eddy_shared.get_best_signs();
      n = 0;
      for (size_t x = 0; x < best[a].size(); ++x, ++n)
        if (signs[n] < 0)
          directions.row (best[a][x]) *= -1.0;
      for (size_t x = 0; x < best[a+1].size(); ++x, ++n) 
        if (signs[n] < 0)
          directions.row (best[a+1][x]) *= -1.0;
    }
  }


  for (size_t i = 0; i < best.size(); ++i) {
    Math::Matrix<value_type> output (best[i].size(), 3);
    for (size_t n = 0; n < best[i].size(); ++n) 
      output.row(n) = directions.row (best[i][n]);
    output.save (std::string(argument[2]) + str(i) + ".txt");
  }

}


