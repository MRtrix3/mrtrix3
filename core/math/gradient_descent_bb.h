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

#ifndef __math_gradient_descent_bb_h__
#define __math_gradient_descent_bb_h__

#include <functional>
#include <limits>
#include <iostream>
#include <fstream>
#include <deque>
#include <limits>
#include <fstream>
#include "math/check_gradient.h"
#include "math/gradient_descent.h"

namespace MR
{
  namespace Math
  {

    //! \addtogroup Optimisation
    // @{

    class LinearUpdateBB { NOMEMALIGN
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

    //! selects the expression used by GradientDescentBB to derive its step size from the
    //  successive parameter and gradient differences dx = x2-x1, dg = g2-g1. SHORT and LONG
    //  are the two canonical Barzilai-Borwein steps (dx.dg/dg.dg and dx.dx/dx.dg respectively);
    //  LEGACY reproduces the expression used throughout the 3.0.x series, the unsigned
    //  magnitude dx.norm()/dg.norm().
    enum class bbgd_step_size_t { SHORT, LONG, LEGACY };

    //! Computes the minimum of a function using a Barzilai Borwein gradient descent approach. ENH: implement stabilised version https://arxiv.org/abs/1907.06409
    template <class Function, class UpdateFunctor=LinearUpdateBB>
      class GradientDescentBB
      { MEMALIGN(GradientDescentBB<Function,UpdateFunctor>)
        public:
          using value_type = typename Function::value_type;

          GradientDescentBB (Function& function, bbgd_step_size_t step_type = bbgd_step_size_t::LEGACY,
                              UpdateFunctor update_functor = LinearUpdateBB(), bool verbose = false) :
            func (function),
            step_type (step_type),
            update_func (update_functor),
            x1 (func.size()),
            x2 (func.size()),
            x3 (func.size()),
            g1 (func.size()),
            g2 (func.size()),
            g3 (func.size()),
            nfeval (0),
            niter (0),
            reason (GDTerminationReason::max_iterations),
            verbose (verbose),
            step_clamped (false),
            delim (",") {  }

          value_type value () const { return f; }
          const Eigen::Matrix<value_type, Eigen::Dynamic, 1>& state () const { return x2; }
          const Eigen::Matrix<value_type, Eigen::Dynamic, 1>& gradient () const { return g2; }
          value_type step_size () const { return dt; }
          value_type gradient_norm () const { return normg; }
          int function_evaluations () const { return nfeval; }
          GDTerminationReason termination_reason () const { return reason; }

          //! true if the most recent step_size() was substituted by the always-positive
          //  fallback because the local curvature estimate dx.dg was non-positive; always
          //  false for step_type LEGACY, which never carries a sign to lose in the first place.
          bool step_was_clamped () const { return step_clamped; }

          //! the update functor instance actually driving this run, as distinct from the
          //  caller's own instance passed into the constructor: UpdateFunctor is stored (and
          //  therefore mutated) by value, so a caller wanting to inspect functor state that
          //  accumulates across iterations (e.g. a convergence check) must read it from here.
          const UpdateFunctor& update_functor () const { return update_func; }

          void be_verbose (bool v) { verbose = v; }
          void precondition (const Eigen::Matrix<value_type, Eigen::Dynamic, 1>& weights) {
            preconditioner_weights = weights;
          }

          //! Register a functor invoked once after every accepted iteration, once the
          //  updated state (state(), gradient(), step_size()) is available for inspection.
          void set_iteration_callback (std::function<void()> callback) {
            iteration_callback = std::move (callback);
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
                reason = GDTerminationReason::gradient_tolerance;
                return;
              }

              if (!retval){
                if (verbose)
                  CONSOLE ("unchanged parameters");
                reason = GDTerminationReason::update_rejected;
                return;
              }
            }
            reason = GDTerminationReason::max_iterations;
          }

          void init () {
            std::ostream dummy (nullptr);
            init (dummy);
          }

          void init (std::ostream& log_os) {
            dt = func.init (x1);
            f = evaluate_func (x1, g1, verbose);
            normg = g1.norm();
            if (normg == 0.0) {
              x2 = x1;
              g2 = g1;
              return;
            }
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
            if (iteration_callback)
              iteration_callback();
          }

          bool iterate () {
            std::ostream dummy (nullptr);
            return iterate (dummy);
          }

          bool iterate (std::ostream& log_os) {
            assert (std::isfinite (normg));
            if ((normg == 0.0) or !update_func (x3, x2, g2, dt))
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
            if (iteration_callback)
              iteration_callback();
            return true;
          }

        protected:
          Function& func;
          bbgd_step_size_t step_type;
          UpdateFunctor update_func;
          Eigen::Matrix<value_type, Eigen::Dynamic, 1> x1, x2, x3, g1, g2, g3, preconditioner_weights;
          value_type f, dt, normg;
          size_t nfeval;
          size_t niter;
          GDTerminationReason reason;
          std::function<void()> iteration_callback;
          bool verbose;
          bool step_clamped;
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
              g2.array() *= preconditioner_weights.array();
            }
            normg = g2.norm();
            const Eigen::Matrix<value_type, Eigen::Dynamic, 1> dx = x2 - x1;
            const Eigen::Matrix<value_type, Eigen::Dynamic, 1> dg = g2 - g1;
            assert (dg.norm() > 0.0);
            const value_type dxdg = dx.dot (dg);
            // dxdg is the local curvature estimate along the step just taken; SHORT and LONG
            // are undefined in sign when it is non-positive (they would then propose stepping
            // uphill). Clamp both to the always-positive magnitude shared with LEGACY rather
            // than propagate an indeterminate sign, and record that the clamp fired so it is
            // distinguishable in the diagnostics.
            step_clamped = (step_type != bbgd_step_size_t::LEGACY) && !(dxdg > 0.0);
            if (step_clamped) {
              dt = dx.norm() / dg.norm();
            } else {
              switch (step_type) {
                case bbgd_step_size_t::SHORT:
                  dt = dxdg / dg.squaredNorm();
                  break;
                case bbgd_step_size_t::LONG:
                  dt = dx.squaredNorm() / dxdg;
                  break;
                case bbgd_step_size_t::LEGACY:
                default:
                  dt = dx.norm() / dg.norm();
                  break;
              }
            }
          }
      };
    //! @}
  }
}

#endif
