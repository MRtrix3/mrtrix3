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

#ifndef __math_gradient_descent_bb_h__
#define __math_gradient_descent_bb_h__

#include <limits>
#include <iostream>
#include <fstream>
#include <deque>
#include <limits>
#include <fstream>
#include "math/check_gradient.h"

namespace MR
{
  namespace Math
  {

    //! \addtogroup Optimisation
    // @{

    class LinearUpdateBB {
      public:
        template <typename ValueType>
          inline bool operator() (Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& newx, const Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& x,
              const Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& g, ValueType step_size) {
            assert (newx.size() == x.size());
            assert (g.size() == x.size());
            newx = x - step_size * g;
            return !newx.isApprox(x);
          }
  };

    //! Computes the minimum of a function using a Barzilai Borwein gradient descent approach.
    template <class Function, class UpdateFunctor=LinearUpdateBB>
      class GradientDescentBB
      {
        public:
          using value_type = typename Function::value_type;

          GradientDescentBB (Function& function, UpdateFunctor update_functor = LinearUpdateBB(), bool verbose = false) :
            func (function),
            update_func (update_functor),
            x1 (func.size()),
            x2 (func.size()),
            x3 (func.size()),
            g1 (func.size()),
            g2 (func.size()),
            g3 (func.size()),
            nfeval (0),
            niter (0),
            verbose (verbose),
            delim (",") {  }

          value_type value () const { return f; }
          const Eigen::Matrix<value_type, Eigen::Dynamic, 1>& state () const { return x2; }
          const Eigen::Matrix<value_type, Eigen::Dynamic, 1>& gradient () const { return g2; }
          value_type step_size () const { return dt; }
          value_type gradient_norm () const { return normg; }
          int function_evaluations () const { return nfeval; }

          void be_verbose (bool v) { verbose = v; }
          void precondition (const Eigen::Matrix<value_type, Eigen::Dynamic, 1>& weights) {
            preconditioner_weights = weights;
          }

          void run (const size_t max_iterations = 1000,
                    const value_type grad_tolerance = 1e-6,
                    std::streambuf* log_stream = nullptr)
          {
            std::ostream log_os(log_stream? log_stream : nullptr);
            if (log_os){
              log_os << "#iteration" << delim << "feval" << delim << "cost" << delim << "stepsize";
              for ( ssize_t a = 0 ; a < x1.size() ; a++ )
                  log_os << delim + "x_" + str(a+1) ;
              for ( ssize_t a = 0 ; a < x1.size() ; a++ )
                  log_os << delim + "g_" + str(a+1) ;
              log_os << "\n" << std::flush;
            }
            init (log_os);

            const value_type gradient_tolerance (grad_tolerance * normg);

            DEBUG ("Gradient descent iteration: init; cost: " + str(f));

            while (niter < max_iterations) {
              bool retval = iterate (log_os);
              DEBUG ("Gradient descent iteration: " + str(niter) + "; cost: " + str(f));
              if (verbose){
                CONSOLE ("iteration " + str (niter) + ": f = " + str (f) + ", |g| = " + str (normg) + ":");
                CONSOLE ("  x  = [ " + str(x2.transpose()) + "]");
              }

              if (normg < gradient_tolerance) {
                if (verbose)
                  CONSOLE ("normg (" + str(normg) + ") < gradient tolerance (" + str(gradient_tolerance) + ")");
                return;
              }

              if (!retval){
                if (verbose)
                  CONSOLE ("unchanged parameters");
                return;
              }
            }
          }

          void init () {
            std::ostream dummy (nullptr);
            init (dummy);
          }

          void init (std::ostream& log_os) {
            dt = func.init (x1);
            f = evaluate_func (x1, g1, verbose);
            normg = g1.norm();
            assert(std::isfinite(normg)); assert(!std::isnan(normg));
            dt /= normg;
            if (verbose) {
              CONSOLE ("initialise: f = " + str (f) + ", |g| = " + str (normg) + ", step = " + str(dt) + ":");
              CONSOLE ("            x = [ " + str(x1.transpose()) + "]");
            }
            if (log_os) {
              log_os << niter << delim << nfeval << delim << str(f) << delim << str(dt);
              for (ssize_t i=0; i< x2.size(); ++i){ log_os << delim << str(x1(i)); }
              for (ssize_t i=0; i< x2.size(); ++i){ log_os << delim << str(g1(i)); }
              log_os << std::endl;
            }

            assert (std::isfinite(f)); assert (!std::isnan(f));
            assert (std::isfinite (normg)); assert (!std::isnan(normg));

            if (update_func (x2, x1, g1, dt)){
              f = evaluate_func (x2, g2, verbose);
            } else {
              dt = 0.0;
              return;
            }
            compute_normg_and_step ();
            assert (std::isfinite (f)); assert (!std::isnan(f));
            assert (std::isfinite(normg)); assert (!std::isnan(normg));
            if (log_os) {
              log_os << niter << delim << nfeval << delim << str(f) << delim << str(dt);
              for (ssize_t i=0; i< x2.size(); ++i){ log_os << delim << str(x2(i)); }
              for (ssize_t i=0; i< x2.size(); ++i){ log_os << delim << str(g2(i)); }
              log_os << std::endl;
            }
            if (verbose) {
              CONSOLE ("            f = " + str (f) + ", |g| = " + str (normg) + ", step = " + str(dt) + ":");
              CONSOLE ("            x = [ " + str(x2.transpose()) + "]");
            }
          }

          bool iterate () {
            std::ostream dummy (nullptr);
            return iterate (dummy);
          }

          bool iterate (std::ostream& log_os) {
            assert (std::isfinite (normg));
            if (!update_func (x3, x2, g2, dt))
              return false;

            f = evaluate_func (x3, g3, verbose);
            x2.swap(x3);
            x1.swap(x3);
            g2.swap(g3);
            g1.swap(g3);
            ++niter;
            if (log_os) {
              log_os << niter << delim << nfeval << delim << str(f) << delim << str(dt);
              for (ssize_t i=0; i< x2.size(); ++i){ log_os << delim << str(x2(i)); }
              for (ssize_t i=0; i< x2.size(); ++i){ log_os << delim << str(g2(i)); }
              log_os << std::endl;
            }
            compute_normg_and_step ();
            return true;
          }

        protected:
          Function& func;
          UpdateFunctor update_func;
          Eigen::Matrix<value_type, Eigen::Dynamic, 1> x1, x2, x3, g1, g2, g3, preconditioner_weights;
          value_type f, dt, normg;
          size_t nfeval;
          size_t niter;
          bool verbose;
          std::string delim;

          value_type evaluate_func (const Eigen::Matrix<value_type, Eigen::Dynamic, 1>& newx, Eigen::Matrix<value_type, Eigen::Dynamic, 1>& newg, bool verbose = false) {
            ++nfeval;
            value_type cost = func (newx, newg);
            if (!std::isfinite (cost))
              throw Exception ("cost function is NaN or Inf!");
            if (verbose){
              CONSOLE ("      << eval " + str(nfeval) + ", f = " + str (cost) + " >>");
              CONSOLE ("      << newx = [ " + str(newx.transpose()) + "]");
              CONSOLE ("      << newg = [ " + str(newg.transpose()) + "]");
            }
            return cost;
          }

          void compute_normg_and_step () {
            if (preconditioner_weights.size()) {
              // value_type g_projected = g2.squaredNorm();
              g2.array() *= preconditioner_weights.array();
            }
            normg = g2.norm();
            assert((g2-g1).norm()>0.0);
            dt = (x2-x1).norm()/(g2-g1).norm();
          }
      };
    //! @}
  }
}

#endif
