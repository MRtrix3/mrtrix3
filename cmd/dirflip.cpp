/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */


#include "command.h"
#include "progressbar.h"
#include "math/rng.h"
#include "math/SH.h"
#include "file/utils.h"
#include "thread.h"
#include "dwi/directions/file.h"

#define DEFAULT_PERMUTATIONS 1e8


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

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
    + Option ("permutations", "number of permutations to try.")
    +   Argument ("num").type_integer (1)

    + Option ("cartesian", "Output the directions in Cartesian coordinates [x y z] instead of [az el].");
}


typedef double value_type;
typedef Eigen::Vector3d vector3_type;




class Shared { MEMALIGN(Shared)
  public:
    Shared (const Eigen::MatrixXd& directions, size_t target_num_permutations) :
      directions (directions), target_num_permutations (target_num_permutations), num_permutations(0),
      progress ("optimising directions for eddy-currents", target_num_permutations),
      best_signs (directions.rows(), 1), best_eddy (std::numeric_limits<value_type>::max()) { }

    bool update (value_type eddy, const vector<int>& signs) 
    {
      std::lock_guard<std::mutex> lock (mutex);
      if (eddy < best_eddy) {
        best_eddy = eddy;
        best_signs = signs;
        progress.set_text ("optimising directions for eddy-currents (current best configuration: energy = " + str(best_eddy) + ")");
      }
      ++num_permutations;
      ++progress;
      return num_permutations < target_num_permutations;
    }



    value_type eddy (size_t i, size_t j, const vector<int>& signs) const {
      vector3_type a = { directions(i,0), directions(i,1), directions(i,2) };
      vector3_type b = { directions(j,0), directions(j,1), directions(j,2) };
      if (signs[i] < 0) a = -a;
      if (signs[j] < 0) b = -b;
      return 1.0 / (a-b).squaredNorm();
    }


    vector<int> get_init_signs () const { return vector<int> (directions.rows(), 1); }
    const vector<int>& get_best_signs () const { return best_signs; }


  protected:
    const Eigen::MatrixXd& directions;
    const size_t target_num_permutations;
    size_t num_permutations;
    ProgressBar progress;
    vector<int> best_signs;
    value_type best_eddy;
    std::mutex mutex;
  
};





class Processor { MEMALIGN(Processor)
  public:
    Processor (Shared& shared) :
      shared (shared),
      signs (shared.get_init_signs()),
      uniform (0, signs.size()-1) { }

    void execute () {
      while (eval()); 
    }


    void next_permutation ()
    {
      signs[uniform(rng)] *= -1;
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
    vector<int> signs;
    Math::RNG rng;
    std::uniform_int_distribution<int> uniform;
};








void run () 
{
  auto directions = DWI::Directions::load_cartesian (argument[0]);

  size_t num_permutations = get_option_value ("permutations", DEFAULT_PERMUTATIONS);

  Shared eddy_shared (directions, num_permutations);
  Thread::run (Thread::multi (Processor (eddy_shared)), "eval thread");

  auto& signs = eddy_shared.get_best_signs();

  for (ssize_t n = 0; n < directions.rows(); ++n) 
    if (signs[n] < 0)
      directions.row(n) *= -1.0;

  bool cartesian = get_options("cartesian").size();
  DWI::Directions::save (directions, argument[1], cartesian);
}



