/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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
#include "math/gradient_descent.h"
#include "math/check_gradient.h"
#include "dwi/directions/file.h"

#define DEFAULT_POWER 1
#define DEFAULT_NITER 10000
#define DEFAULT_RESTARTS 10


using namespace MR;
using namespace App;

void usage ()
{

  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Generate a set of uniformly distributed directions using a bipolar electrostatic repulsion model";

  DESCRIPTION
    + "Directions are distributed by analogy to an electrostatic repulsion system, with each direction "
    "corresponding to a single electrostatic charge (for -unipolar), or a pair of diametrically opposed charges "
    "(for the default bipolar case). The energy of the system is determined based on the Coulomb repulsion, "
    "which assumes the form 1/r^power, where r is the distance between any pair of charges, and p is the power "
    "assumed for the repulsion law (default: 1). The minimum energy state is obtained by gradient descent.";


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
    + Option ("power", "specify exponent to use for repulsion power law (default: " + str(DEFAULT_POWER) + "). This must be a power of 2 (i.e. 1, 2, 4, 8, 16, ...).")
    +   Argument ("exp").type_integer (1, std::numeric_limits<int>::max())

    + Option ("niter", "specify the maximum number of iterations to perform (default: " + str(DEFAULT_NITER) + ").")
    +   Argument ("num").type_integer (1, std::numeric_limits<int>::max())

    + Option ("restarts", "specify the number of restarts to perform (default: " + str(DEFAULT_RESTARTS) + ").")
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
    Energy (ProgressBar& progress) :
      progress (progress),
      ndirs (to<int> (argument[0])),
      bipolar (!(get_options ("unipolar").size())),
      power (0),
      directions (3 * ndirs) { }

// Non-optimised compilation can't handle recursive inline functions
#ifdef __OPTIMIZE__
FORCE_INLINE
#endif
    double fast_pow (double x, int p) {
      return p == 1 ? x : fast_pow (x*x, p/2);
    }

    using value_type = double;

    size_t size () const { return 3 * ndirs; }

    // set x to original directions provided in constructor.
    // The idea is to save the directions from one run to initialise next run
    // at higher power.
    double init (Eigen::VectorXd& x)
    {
      Math::RNG::Normal<double> rng;
      for (size_t n = 0; n < ndirs; ++n) {
        auto d = x.segment (3*n,3);
        d[0] = rng();
        d[1] = rng();
        d[2] = rng();
        d.normalize();
      }
      return 0.01;
    }



    // function executed by optimiser at each iteration:
    double operator() (const Eigen::VectorXd& x, Eigen::VectorXd& g) {
      double E = 0.0;
      g.setZero();

      for (size_t i = 0; i < ndirs-1; ++i) {
        auto d1 = x.segment (3*i, 3);
        auto g1 = g.segment (3*i, 3);
        for (size_t j = i+1; j < ndirs; ++j) {
          auto d2 = x.segment (3*j, 3);
          auto g2 = g.segment (3*j, 3);

          Eigen::Vector3d r = d1-d2;
          double _1_r2 = 1.0 / r.squaredNorm();
          double _1_r = std::sqrt (_1_r2);
          double e = fast_pow (_1_r, power);
          E += e;
          g1 -= (power * e * _1_r2) * r;
          g2 += (power * e * _1_r2) * r;

          if (bipolar) {
            r = d1+d2;
            _1_r2 = 1.0 / r.squaredNorm();
            _1_r = std::sqrt (_1_r2);
            e = fast_pow (_1_r, power);
            E += e;
            g1 -= (power * e * _1_r2) * r;
            g2 -= (power * e * _1_r2) * r;
          }

        }
      }

      // constrain gradients to lie tangent to unit sphere:
      for (size_t n = 0; n < ndirs; ++n)
        g.segment(3*n,3) -= x.segment(3*n,3).dot (g.segment(3*n,3)) * x.segment(3*n,3);

      return E;
    }



    // function executed per thread:
    void execute ()
    {
      size_t this_start = 0;
      while ((this_start = current_start++) < restarts) {
        INFO ("launching start " + str (this_start));
        double E = 0.0;

        for (power = 1; power <= target_power; power *= 2) {
          Math::GradientDescent<Energy,ProjectedUpdate> optim (*this, ProjectedUpdate());

          INFO ("start " + str(this_start) + ": setting power = " + str (power));
          optim.init();

          size_t iter = 0;
          for (; iter < niter; iter++) {
            if (!optim.iterate())
              break;

            DEBUG ("start " + str(this_start) + ": [ " + str (iter) + " ] (pow = " + str (power) + ") E = " + str (optim.value(), 8)
                + ", grad = " + str (optim.gradient_norm(), 8));

            std::lock_guard<std::mutex> lock (mutex);
            ++progress;
          }

          directions = optim.state();
          E = optim.value();
        }



        std::lock_guard<std::mutex> lock (mutex);
        if (E < best_E) {
          best_E = E;
          best_directions = directions;
        }
      }
    }


    static size_t restarts;
    static int target_power;
    static size_t niter;
    static double best_E;
    static Eigen::VectorXd best_directions;

  protected:
    ProgressBar& progress;
    size_t ndirs;
    bool bipolar;
    int power;
    Eigen::VectorXd directions;
    double E;

    static std::mutex mutex;
    static std::atomic<size_t> current_start;
};


size_t Energy::restarts (DEFAULT_RESTARTS);
int Energy::target_power (DEFAULT_POWER);
size_t Energy::niter (DEFAULT_NITER);
std::mutex Energy::mutex;
std::atomic<size_t> Energy::current_start (0);
double Energy::best_E = std::numeric_limits<double>::infinity();
Eigen::VectorXd Energy::best_directions;




void run ()
{
  Energy::restarts = get_option_value ("restarts", DEFAULT_RESTARTS);
  Energy::target_power = get_option_value ("power", DEFAULT_POWER);
  Energy::niter = get_option_value ("niter", DEFAULT_NITER);

  {
    ProgressBar progress ("Optimising directions up to power " + str(Energy::target_power) + " (" + str(Energy::restarts) + " restarts)");
    Energy energy_functor (progress);
    auto threads = Thread::run (Thread::multi (energy_functor), "energy function");
  }

  CONSOLE ("final energy = " + str(Energy::best_E));
  size_t ndirs = Energy::best_directions.size()/3;
  Eigen::MatrixXd directions_matrix (ndirs, 3);
  for (size_t n = 0; n < ndirs; ++n)
    directions_matrix.row (n) = Energy::best_directions.segment (3*n, 3);

  DWI::Directions::save (directions_matrix, argument[1], get_options ("cartesian").size());
}




