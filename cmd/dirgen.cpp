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
#include "__mrtrix_plugin.h"

#include "command.h"
#include "progressbar.h"
#include "math/rng.h"
#include "math/gradient_descent.h"
#include "math/check_gradient.h"
#include "dwi/directions/file.h"

#define DEFAULT_POWER 2
#define DEFAULT_NITER 10000


using namespace MR;
using namespace App;

void usage ()
{

AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

SYNOPSIS = "Generate a set of uniformly distributed directions using a bipolar electrostatic repulsion model";

REFERENCES 
  + "Jones, D.; Horsfield, M. & Simmons, A. "
  "Optimal strategies for measuring diffusion in anisotropic systems by magnetic resonance imaging. "
  "Magnetic Resonance in Medicine, 1999, 42: 515-525"
             
  + "Papadakis, N. G.; Murrills, C. D.; Hall, L. D.; Huang, C. L.-H. & Adrian Carpenter, T. "
  "Minimal gradient encoding for robust estimation of diffusion anisotropy. "
  "Magnetic Resonance Imaging, 2000, 18: 671-679";

ARGUMENTS
  + Argument ("ndir", "the number of directions to generate.").type_integer (6, std::numeric_limits<int>::max())
  + Argument ("dirs", "the text file to write the directions to, as [ az el ] pairs.").type_file_out();

OPTIONS
  + Option ("power", "specify exponent to use for repulsion power law (default: " + str(DEFAULT_POWER) + "). This must be a power of 2 (i.e. 2, 4, 8, 16, ...).")
  +   Argument ("exp").type_integer (2, std::numeric_limits<int>::max())

  + Option ("niter", "specify the maximum number of iterations to perform (default: " + str(DEFAULT_NITER) + ").")
  +   Argument ("num").type_integer (1, std::numeric_limits<int>::max())

  + Option ("unipolar", "optimise assuming a unipolar electrostatic repulsion model rather than the bipolar model normally assumed in DWI")

  + Option ("cartesian", "Output the directions in Cartesian coordinates [x y z] instead of [az el].");

}




// constrain directions to remain unit length:
class ProjectedUpdate { MEMALIGN(ProjectedUpdate)
  public:
    bool operator() (
        Eigen::VectorXd& newx, 
        const Eigen::VectorXd& x,
        const Eigen::VectorXd& g, 
        double step_size) {
      newx.noalias() = x - step_size * g;
      for (ssize_t n = 0; n < newx.size(); n += 3)
        newx.segment (n,3).normalize();
      return newx != x;
    }
};



class Energy { MEMALIGN(Energy)
  public:
    Energy (size_t n_directions, int power, bool bipolar, const Eigen::VectorXd& init_directions) : 
      ndir (n_directions),
      power (power),
      bipolar (bipolar),
      init_directions (init_directions) { }

    FORCE_INLINE double fast_pow (double x, int p) {
      return p == 1 ? x : fast_pow (x*x, p/2);
    }

    typedef double value_type;

    size_t size () const { return 3 * ndir; }

    // set x to original directions provided in constructor. 
    // The idea is to save the directions from one run to initialise next run
    // at higher power.
    double init (Eigen::VectorXd& x) {
      x = init_directions;
      return 0.01;
    }

    double operator() (const Eigen::VectorXd& x, Eigen::VectorXd& g) {
      double E = 0.0;
      g.setZero();

      for (size_t i = 0; i < ndir-1; ++i) {
        auto d1 = x.segment (3*i, 3);
        auto g1 = g.segment (3*i, 3);
        for (size_t j = i+1; j < ndir; ++j) {
          auto d2 = x.segment (3*j, 3);
          auto g2 = g.segment (3*j, 3);

          Eigen::Vector3d r = d1-d2;
          double _1_r2 = 1.0 / r.squaredNorm();
          double e = fast_pow (_1_r2, power/2); 
          E += e;
          g1 -= (power * e * _1_r2) * r; 
          g2 += (power * e * _1_r2) * r; 

          if (bipolar) {
            r = d1+d2;
            _1_r2 = 1.0 / r.squaredNorm();
            e = fast_pow (_1_r2, power/2); 
            E += e;
            g1 -= (power * e * _1_r2) * r; 
            g2 -= (power * e * _1_r2) * r; 
          }

        }
      }

      // constrain gradients to lie tangent to unit sphere:
      for (size_t n = 0; n < ndir; ++n) 
        g.segment(3*n,3) -= x.segment(3*n,3).dot (g.segment(3*n,3)) * x.segment(3*n,3);

      return E;
    }


  protected:
    size_t ndir;
    int power;
    bool bipolar;
    const Eigen::VectorXd& init_directions;
};



void run () {
  size_t niter = get_option_value ("niter", DEFAULT_NITER);
  int target_power = get_option_value ("power", DEFAULT_POWER);
  bool bipolar = !(get_options ("unipolar").size());
  int ndirs = to<int> (argument[0]);

  Eigen::VectorXd directions (3*ndirs);

  { // random initialisation:
    Math::RNG::Normal<double> rng;
    for (ssize_t n = 0; n < ndirs; ++n) {
      auto d = directions.segment (3*n,3);
      d[0] = rng();
      d[1] = rng();
      d[2] = rng();
      d.normalize();
    }
  }

  // optimisation proper:
  {
    ProgressBar progress ("Optimising directions");
    for (int power = 2; power <= target_power; power *= 2) {
      Energy energy (ndirs, power, bipolar, directions);

      Math::GradientDescent<Energy,ProjectedUpdate> optim (energy, ProjectedUpdate());

      INFO ("setting power = " + str (power));
      optim.init();

      //Math::check_function_gradient (energy, optim.state(), 0.001);
      //return;

      size_t iter = 0;
      for (; iter < niter; iter++) {
        if (!optim.iterate()) 
          break;

        INFO ("[ " + str (iter) + " ] (pow = " + str (power) + ") E = " + str (optim.value(), 8)
            + ", grad = " + str (optim.gradient_norm(), 8));

        progress.update ([&]() { return "Optimising directions (power " + str(power) 
            + ", energy: " + str(optim.value(), 8) + ", gradient: " + str(optim.gradient_norm(), 8) + ", iteration " + str(iter) + ")"; });
      }

      directions = optim.state();

      progress.set_text ("Optimising directions (power " + str(power) 
        + ", energy: " + str(optim.value(), 8) + ", gradient: " + str(optim.gradient_norm(), 8) + ", iteration " + str(iter) + ")");
    }
  }

  Eigen::MatrixXd directions_matrix (ndirs, 3);
  for (int n = 0; n < ndirs; ++n) 
    directions_matrix.row (n) = directions.segment (3*n, 3);

  DWI::Directions::save (directions_matrix, argument[1], get_options ("cartesian").size());
}




