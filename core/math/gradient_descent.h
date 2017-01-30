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


#ifndef __math_gradient_descent_h__
#define __math_gradient_descent_h__

#include <limits>

namespace MR
{
  namespace Math
  {

    //! \addtogroup Optimisation
    // @{


    namespace {

      class LinearUpdate { NOMEMALIGN
        public:
          template <typename ValueType>
            inline bool operator() (Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& newx, const Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& x,
                const Eigen::Matrix<ValueType, Eigen::Dynamic, 1>& g, ValueType step_size) {
              bool changed = false;
              for (ssize_t n = 0; n < x.size(); ++n) {
                newx[n] = x[n] - step_size * g[n];
                if (newx[n] != x[n])
                  changed = true;
              }
              return changed;
            }
      };

    }

    //! Computes the minimum of a function using a gradient descent approach.
    template <class Function, class UpdateFunctor=LinearUpdate>
      class GradientDescent
      { MEMALIGN(GradientDescent<Function,UpdateFunctor>)
        public:
          typedef typename Function::value_type value_type;

          GradientDescent (Function& function, UpdateFunctor update_functor = LinearUpdate(), value_type step_size_upfactor = 3.0, value_type step_size_downfactor = 0.1, bool verbose = false) :
            func (function),
            update_func (update_functor),
            step_up (step_size_upfactor),
            step_down (step_size_downfactor),
            verbose (verbose),
            delim (","),
            niter (0),
            x (func.size()),
            x2 (func.size()),
            g (func.size()),
            g2 (func.size()) { }


          value_type value () const throw () { return f; }
          const Eigen::Matrix<value_type, Eigen::Dynamic, 1>& state () const throw () { return x; }
          const Eigen::Matrix<value_type, Eigen::Dynamic, 1>& gradient () const throw ()  { return g; }
          value_type step_size () const { return dt; }
          value_type gradient_norm () const throw () { return normg; }
          int function_evaluations () const throw () { return nfeval; }

          void be_verbose(bool v) { verbose = v; }
          void precondition (const Eigen::Matrix<value_type, Eigen::Dynamic, 1>& weights) {
            preconditioner_weights = weights;
          }

          void run (const size_t max_iterations = 1000,
            const value_type grad_tolerance = 1e-6,
            std::streambuf* log_stream = nullptr) {
            std::ostream log_os(log_stream? log_stream : nullptr);
            if (log_os){
              log_os << "#iteration" << delim << "feval" << delim << "cost" << delim << "stepsize";
              for ( ssize_t a = 0 ; a < x.size() ; a++ )
                  log_os << delim + "x_" + str(a+1) ;
              for ( ssize_t a = 0 ; a < x.size() ; a++ )
                  log_os << delim + "g_" + str(a+1) ;
              log_os << "\n" << std::flush;
            }
            init (log_os);

            const value_type gradient_tolerance (grad_tolerance * normg);

            DEBUG ("Gradient descent iteration: init; cost: " + str(f));

            while (niter < max_iterations) {
              bool retval = iterate (log_os);
              DEBUG ("Gradient descent iteration: " + str(niter) + "; cost: " + str(f));
              if (verbose) {
                CONSOLE ("iteration " + str (niter) + ": f = " + str (f) + ", |g| = " + str (normg) + ":");
                CONSOLE ("  x = [ " + str(x.transpose()) + "]");
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
            dt = func.init (x);
            nfeval = 0;
            f = evaluate_func (x, g, verbose);
            compute_normg_and_step_unscaled ();
            assert(std::isfinite(g.norm()));
            assert(!std::isnan(g.norm()));
            dt /= g.norm();
            if (verbose) {
              CONSOLE ("initialise: f = " + str (f) + ", |g| = " + str (normg) + ":");
              CONSOLE ("  x = [ " + str(x.transpose()) + "]");
            }
            assert (std::isfinite (f));
            assert (!std::isnan(f));
            assert (std::isfinite (normg));
            assert (!std::isnan(normg));
            if (log_os) {
              log_os << niter << delim << nfeval << delim << str(f) << delim << str(dt);
              for (ssize_t i=0; i< x.size(); ++i){ log_os << delim << str(x(i)); }
              for (ssize_t i=0; i< x.size(); ++i){ log_os << delim << str(g(i)); }
              log_os << std::endl;
            }
          }

          bool iterate () {
            std::ostream dummy (nullptr);
            return iterate (dummy);
          }

          bool iterate (std::ostream& log_os) {
            // assert (normg != 0.0);
            assert (std::isfinite (normg));


            while (normg != 0.0) {
              if (!update_func (x2, x, g, dt))
                return false;

              value_type f2 = evaluate_func (x2, g2, verbose);

              // quadratic minimum:
              value_type step_length = step_unscaled*dt;
              value_type denom = 2.0 * (normg*step_length + f2 - f);
              value_type quadratic_minimum = denom > 0.0 ? normg * step_length / denom : step_up;

              if (quadratic_minimum < step_down) quadratic_minimum = step_down;
              if (quadratic_minimum > step_up) quadratic_minimum = step_up;

              if (f2 < f) {
                ++niter;
                dt *= quadratic_minimum;
                f = f2;
                x.swap (x2);
                g.swap (g2);
                if (log_os) {
                  log_os << niter << delim << nfeval << delim << str(f) << delim << str(dt);
                  for (ssize_t i=0; i< x.size(); ++i){ log_os << delim << str(x(i)); }
                  for (ssize_t i=0; i< x.size(); ++i){ log_os << delim << str(g(i)); }
                  log_os << std::endl;
                }
                compute_normg_and_step_unscaled ();
                return true;
              }

              if (quadratic_minimum >= 1.0)
                quadratic_minimum = 0.5;
              dt *= quadratic_minimum;

              if (dt <= 0.0)
                return false;
            }
            return false;
          }

        protected:
          Function& func;
          UpdateFunctor update_func;
          const value_type step_up, step_down;
          bool verbose;
          std::string delim;
          size_t niter;
          Eigen::Matrix<value_type, Eigen::Dynamic, 1> x, x2, g, g2, preconditioner_weights;
          value_type f, dt, normg, step_unscaled;
          size_t nfeval;

          value_type evaluate_func (const Eigen::Matrix<value_type, Eigen::Dynamic, 1>& newx,
                                    Eigen::Matrix<value_type, Eigen::Dynamic, 1>& newg,
                                    bool verbose = false) {
            nfeval++;
            value_type cost = func (newx, newg);
            if (!std::isfinite (cost))
              throw Exception ("cost function is NaN or Inf!");
            if (verbose)
              CONSOLE ("      << eval " + str(nfeval) + ", f = " + str (cost) + " >>");
            return cost;
          }


          void compute_normg_and_step_unscaled () {
            normg = step_unscaled = g.norm();
            assert(std::isfinite(normg));
            if (normg > 0.0){
              if (preconditioner_weights.size()) {
                value_type g_projected = 0.0;
                for (ssize_t n = 0; n < g.size(); ++n) {
                  g_projected += preconditioner_weights[n] * Math::pow2(g[n]);
                }
                g.array() *= preconditioner_weights.array();
                normg = g_projected / normg;
                assert(std::isfinite(normg));
              }
            }
          }

      };
    //! @}
  }
}

#endif

