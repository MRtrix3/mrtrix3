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
#include "math/SH.h"
#include "file/utils.h"
#include "thread.h"
#include "point.h"
#include "dwi/directions/file.h"



using namespace MR;
using namespace App;

void usage () {

  DESCRIPTION
    + "optimise the polarity of the directions in a scheme with respect to a "
    "unipolar electrostatic repulsion model, by inversion of individual "
    "directions. The orientations themselves are not affected, only their "
    "polarity. This is necessary to ensure near-optimal distribution of DW "
    "directions for eddy-current correction.";
     
  ARGUMENTS
    + Argument ("in", "the input files for the directions.").type_file_in()
    + Argument ("out", "the output files for the directions.").type_file_out();


  OPTIONS
    + Option ("permutations", "number of permutations to try")
    +   Argument ("num").type_integer (1, 1e8)

    + Option ("cartesian", "Output the directions in Cartesian coordinates [x y z] instead of [az el].");
}


typedef double value_type;




class Shared {
  public:
    Shared (const Math::Matrix<value_type>& directions, size_t target_num_permutations) :
      directions (directions), target_num_permutations (target_num_permutations), num_permutations(0),
      progress ("optimising directions for eddy-currents...", target_num_permutations),
      best_signs (directions.rows(), 1), best_eddy (std::numeric_limits<value_type>::max()) { }

    bool update (value_type eddy, const std::vector<int>& signs) 
    {
      std::lock_guard<std::mutex> lock (mutex);
      if (eddy < best_eddy) {
        best_eddy = eddy;
        best_signs = signs;
        progress.set_text ("optimising directions for eddy-currents (current best configuration: energy = " + str(best_eddy) + ")...");
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





class Processor {
  public:
    Processor (Shared& shared) :
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
    Shared& shared;
    std::vector<int> signs;
    Math::RNG rng;
};








void run () 
{
  Math::Matrix<value_type> directions = DWI::Directions::load_cartesian<value_type> (argument[0]);

  size_t num_permutations = 1e8;
  Options opt = get_options ("permutations");
  if (opt.size())
    num_permutations = opt[0][0];

  Shared eddy_shared (directions, num_permutations);
  Thread::run (Thread::multi (Processor (eddy_shared)), "eval thread");

  auto& signs = eddy_shared.get_best_signs();

  for (size_t n = 0; n < directions.rows(); ++n) 
    if (signs[n] < 0)
      directions.row(n) *= -1.0;

  bool cartesian = get_options("cartesian").size();
  DWI::Directions::save (directions, argument[1], cartesian);
}



