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
#include "math/rng.h"
#include "math/gradient_descent.h"
#include "math/check_gradient.h"
#include "dwi/directions/file.h"


using namespace MR;
using namespace App;

void usage () {

DESCRIPTION
  + "generate a set of uniformly distributed directions using a bipolar electrostatic repulsion model.";

REFERENCES 
  + "* Jones, D.; Horsfield, M. & Simmons, A. "
  "Optimal strategies for measuring diffusion in anisotropic systems by magnetic resonance imaging. "
  "Magnetic Resonance in Medicine, 42: 515-525 (1999)."
             
  + "* Papadakis, N. G.; Murrills, C. D.; Hall, L. D.; Huang, C. L.-H. & Adrian Carpenter, T. "
  "Minimal gradient encoding for robust estimation of diffusion anisotropy. "
  "Magnetic Resonance Imaging, 18: 671-679 (2000).";

ARGUMENTS
  + Argument ("ndir", "the number of directions to generate.").type_integer (6, 60, std::numeric_limits<int>::max())
  + Argument ("dirs", "the text file to write the directions to, as [ az el ] pairs.").type_file_out();

OPTIONS
  + Option ("power", "specify exponent to use for repulsion power law (default: 2). This must be a power of 2 (i.e. 2, 4, 8, 16, ...).")
  +   Argument ("exp").type_integer (2, 2, std::numeric_limits<int>::max())

  + Option ("niter", "specify the maximum number of iterations to perform (default: 10000).")
  +   Argument ("num").type_integer (1, 10000, std::numeric_limits<int>::max())

  + Option ("unipolar", "optimise assuming a unipolar electrostatic repulsion model rather than the bipolar model normally assumed in DWI")

  + Option ("cartesian", "Output the directions in Cartesian coordinates [x y z] instead of [az el].");

}




// constrain directions to remain unit length:
class ProjectedUpdate {
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



class Energy {
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
  size_t niter = 10000;
  int target_power = 2;
  bool bipolar = true;

  auto opt = get_options ("power");
  if (opt.size())
    target_power = opt[0][0];

  opt = get_options ("niter");
  if (opt.size())
    niter = opt[0][0];

  int ndirs = to<int> (argument[0]);

  if (get_options ("unipolar").size())
    bipolar = false;


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
    ProgressBar progress ("Optimising directions...");
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
            + ", energy: " + str(optim.value(), 8) + ", gradient: " + str(optim.gradient_norm(), 8) + ", iteration " + str(iter) + ")..."; });
      }

      directions = optim.state();

      progress.set_text ("Optimising directions (power " + str(power) 
        + ", energy: " + str(optim.value(), 8) + ", gradient: " + str(optim.gradient_norm(), 8) + ", iteration " + str(iter) + ")...");
    }
  }

  Eigen::MatrixXd directions_matrix (ndirs, 3);
  for (int n = 0; n < ndirs; ++n) 
    directions_matrix.row (n) = directions.segment (3*n, 3);

  DWI::Directions::save (directions_matrix, argument[1], get_options ("cartesian").size());
}




