/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 12/01/09.

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

#ifndef __math_gradient_descent_h__
#define __math_gradient_descent_h__

//#define GRADIENT_DESCENT_LOG

#include <limits>
#include <iostream>
#include <fstream>
#include <deque>
#include <limits>

// #include <iterator>
#include "math/check_gradient.h"

namespace MR
{
  namespace Math
  {

    //! \addtogroup Optimisation
    // @{



    class LinearUpdate {
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

    //! Computes the minimum of a function using a gradient descent approach.
    template <class Function, class UpdateFunctor=LinearUpdate>
      class GradientDescent
      {
        public:
          typedef typename Function::value_type value_type;

          GradientDescent (Function& function, UpdateFunctor update_functor = LinearUpdate(), value_type step_size_upfactor = 3.0, value_type step_size_downfactor = 0.1, const size_t sliding_cost_window_size = 2) :
            func (function),
            update_func (update_functor),
            step_up (step_size_upfactor),
            step_down (step_size_downfactor),
            x (func.size()),
            x2 (func.size()),
            g (func.size()),
            g2 (func.size()),
            sliding_cost_window_size (sliding_cost_window_size) {  }


          value_type value () const throw () { return f; }
          const Eigen::Matrix<value_type, Eigen::Dynamic, 1>& state () const throw () { return x; }
          const Eigen::Matrix<value_type, Eigen::Dynamic, 1>& gradient () const throw ()  { return g; }
          value_type step_size () const { return dt; }
          value_type gradient_norm () const throw () { return normg; }
          int function_evaluations () const throw () { return nfeval; }

          void precondition (const Eigen::Matrix<value_type, Eigen::Dynamic, 1>& weights) {
            preconditioner_weights = weights;
          }

          void run (const int max_iterations = 1000,
                    const value_type grad_tolerance = 1e-6,
                    bool verbose = false,
                    const value_type step_length_tolerance = -1,
                    const value_type relative_cost_improvement_tolerance = 1e-10,
                    const value_type gradient_criterion_tolerance = 1e-10,
                    std::streambuf* log_stream = nullptr)
          {
            init (verbose);
            value_type gradient_tolerance = grad_tolerance * normg;
            gradient_criterion = std::numeric_limits<value_type>::quiet_NaN();
            f0 = std::numeric_limits<value_type>::quiet_NaN();
            relative_cost_improvement = std::numeric_limits<value_type>::quiet_NaN();

            #ifdef GRADIENT_DESCENT_LOG
              std::ostream log_os(log_stream? log_stream : std::cerr.rdbuf());
              std::string delim = ",";
              if (log_stream){
                log_os << "'iteration,cost,relative_cost_improvement,gradient_criterion_" +
                  str(sliding_cost_window_size) + ",normg,stepsize,grad_stop";
                for ( size_t a = 0 ; a < x.size() ; a++ )
                  log_os << delim + "param_" + str(a+1) ;
                log_os << + "\n";
                log_os.flush();
              }

              if (log_stream){
                  log_os << "0" + delim + str(f) + delim + str(relative_cost_improvement) + delim + str(gradient_criterion) + delim + str(normg) + delim
                    + str(step_unscaled*dt) + delim + str(normg/(gradient_tolerance/grad_tolerance)) + delim;
                  // std::copy(x.begin(),x.end() - 1, std::ostream_iterator<double>(log_os,delim.c_str()));
                  // log_os << str(x[x.size()-1]) + "\n";
                  for ( size_t a = 0 ; a < x.size() ; a++ )
                    log_os << delim + str(x[a]);
                  log_os << "\n";
                  log_os.flush();
              }
            #endif

            for (int niter = 1; niter < max_iterations; niter++) {
              // Math::check_function_gradient (func, x, 0.1, false, preconditioner_weights);
              bool retval = iterate (verbose);
              {
                std::string cost = std::to_string(f);
                INFO ("Gradient descent iteration: " + str(niter) + "; cost: " + cost);
                #ifdef GRADIENT_DESCENT_LOG
                  if (log_stream){
                    log_os << str(niter) + delim + cost + delim + str(relative_cost_improvement) + delim + str(gradient_criterion) + delim + str(normg) + delim
                      + str(step_unscaled*dt) + delim + str(normg/(gradient_tolerance/grad_tolerance)) + delim;
                    // std::copy(x.begin(),x.end() - 1, std::ostream_iterator<double>(log_os,delim.c_str()));
                    // log_os << str(x[x.size()-1]) + "\n";
                    for ( size_t a = 0 ; a < x.size() ; a++ )
                      log_os << delim + str(x[a]);
                    log_os << "\n";
                    log_os.flush();
                  }
                #endif
             }
              if (verbose){
                CONSOLE ("iteration " + str (niter) + ": f = " + str (f) + ", |g| = " + str (normg) + ":");
                CONSOLE ("  x = [ " + str(x.transpose()) + "]");
              }
               // std::cerr << std::endl;
               // std::cerr << std::fixed << App::NAME << ":   "<< "Gradient descent iteration: " << niter << "; cost: " << f << std::endl;
              DEBUG ("normg (" + str(normg) +"); gradient_tolerance ("+ str(gradient_tolerance) +")" );
              if (normg < gradient_tolerance){
                INFO ("normg (" + str(normg) +") < gradient_tolerance ("+ str(gradient_tolerance) +")" );
                return;
              }

              if (step_unscaled*dt < step_length_tolerance){
                DEBUG ("normg now: " + str(normg) +", normg initial: " + str(gradient_tolerance/grad_tolerance) );
                INFO ("step_length (" + str(step_unscaled*dt) +") < step_length_tolerance ("+ str(step_length_tolerance) +")" );
                return;
              }

              if (relative_cost_improvement < relative_cost_improvement_tolerance){
                INFO ("relative_cost_improvement (" + str(relative_cost_improvement) +") < relative_cost_improvement_tolerance ("+ str(relative_cost_improvement_tolerance) +")" );
                return;
              }

              if (sliding_cost_window_size > 0 && std::isfinite(gradient_criterion) ){
                if (gradient_criterion < gradient_criterion_tolerance){
                  INFO ("gradient_criterion (" + str(gradient_criterion) + ") < gradient_criterion_tolerance ("+ str(gradient_criterion_tolerance) +")" );
                  return;
                }
              }

              if (!retval){
                INFO("unchanged parameters");
                INFO ("normg now: " + str(normg) +", normg initial: " + str(gradient_tolerance/grad_tolerance) );
                INFO ("step_length (" + str(step_unscaled*dt) +"); step_length_tolerance ("+ str(step_length_tolerance) +")" );
                return;
              }
            }
          }


          void init (bool verbose = false) {
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
          }


          bool iterate (bool verbose = false) {
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
                dt *= quadratic_minimum;
                relative_cost_improvement = f/f2-1;
                if (sliding_cost_window_size > 1){
                  sliding_cost.push_back(f2);
                  if ( sliding_cost.size() > sliding_cost_window_size)
                    sliding_cost.pop_front();
                  if (sliding_cost.size() >= sliding_cost_window_size){
                    value_type poormansgradient = sliding_cost.back()-sliding_cost.front();
                    if (!std::isfinite(f0))
                      f0 = poormansgradient;
                    gradient_criterion = poormansgradient/f0; // 1.0 - std::exp(poormansgradient);
                  }
                }
                f = f2;
                x.swap (x2);
                g.swap (g2);
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
          Eigen::Matrix<value_type, Eigen::Dynamic, 1> x, x2, g, g2, preconditioner_weights;
          value_type f, dt, normg, step_unscaled;
          int nfeval;
          value_type relative_cost_improvement;
          value_type gradient_criterion;
          std::deque<value_type> sliding_cost;
          value_type f0;
          size_t sliding_cost_window_size;

          value_type evaluate_func (const Eigen::Matrix<value_type, Eigen::Dynamic, 1>& newx, Eigen::Matrix<value_type, Eigen::Dynamic, 1>& newg, bool verbose = false) {
            nfeval++;
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


          void compute_normg_and_step_unscaled () {
            normg = step_unscaled = g.norm();
            assert(std::isfinite(normg));
            if (normg > 0.0){
              if (preconditioner_weights.size()) {
                value_type g_projected = 0.0;
                step_unscaled = 0.0;
                for (ssize_t n = 0; n < g.size(); ++n) {
                  step_unscaled += std::pow(g[n], 2);
                  g_projected += preconditioner_weights[n] * std::pow(g[n], 2);
                  g[n] *= preconditioner_weights[n];
                }
                normg = g_projected / normg;
                assert(std::isfinite(normg));
                step_unscaled = std::sqrt (step_unscaled);
              }
            }
          }

      };
    //! @}
  }
}

#endif
