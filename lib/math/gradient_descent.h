#ifndef __math_gradient_descent_h__
#define __math_gradient_descent_h__

#include <limits>

#include "math/vector.h"

namespace MR
{
  namespace Math
  {

    //! \addtogroup Optimisation
    // @{


    namespace {

      class LinearUpdate {
        public:
          template <typename ValueType>
            inline bool operator() (Math::Vector<ValueType>& newx, const Math::Vector<ValueType>& x, 
                const Math::Vector<ValueType>& g, ValueType step_size) { 
              bool changed = false;
              for (size_t n = 0; n < x.size(); ++n) {
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
      {
        public:
          typedef typename Function::value_type value_type;

          GradientDescent (Function& function, UpdateFunctor update_functor = LinearUpdate(), value_type step_size_upfactor = 3.0, value_type step_size_downfactor = 0.1) :
            func (function),
            update_func (update_functor),
            step_up (step_size_upfactor),
            step_down (step_size_downfactor),
            x (func.size()),
            x2 (func.size()),
            g (func.size()),
            g2 (func.size()) { }


          value_type value () const throw () { return f; }
          const Vector<value_type>& state () const throw () { return x; }
          const Vector<value_type>& gradient () const throw ()  { return g; }
          value_type gradient_norm () const throw () { return normg; }
          int function_evaluations () const throw () { return nfeval; }

          void precondition (const Math::Vector<value_type>& weights) {
            preconditioner_weights = weights;
          }

          void run (const int max_iterations = 1000, const value_type grad_tolerance = 1e-6, bool verbose = false) {
            init (verbose);

            value_type gradient_tolerance = grad_tolerance * normg;

            for (int niter = 0; niter < max_iterations; niter++) {
              bool retval = iterate (verbose);
              if (verbose) {
                CONSOLE ("iteration " + str (niter) + ": f = " + str (f) + ", |g| = " + str (normg) + ":");
                CONSOLE ("  x = [ " + str(x) + "]");
              }

              if (normg < gradient_tolerance)
                return;

              if (!retval)
                return;
            }
          }


          void init (bool verbose = false) {
            dt = func.init (x);
            nfeval = 0;
            f = evaluate_func (x, g, verbose);
            compute_normg_and_step_unscaled ();
            dt /= norm(g);
            if (verbose) {
              CONSOLE ("initialise: f = " + str (f) + ", |g| = " + str (normg) + ":");
              CONSOLE ("  x = [ " + str(x) + "]");
            }
            assert (std::isfinite (f));
            assert (std::isfinite (normg));
          }


          bool iterate (bool verbose = false) {
            assert (normg != 0.0);

            while (true) {
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
                f = f2;
                x.swap (x2);
                g.swap (g2);
                compute_normg_and_step_unscaled ();
                return true;
              }

              if (quadratic_minimum >= 1.0)
                quadratic_minimum = 0.5;
              dt *= quadratic_minimum;
            }
          }

        protected:
          Function& func;
          UpdateFunctor update_func;
          const value_type step_up, step_down;
          Vector<value_type> x, x2, g, g2, preconditioner_weights;
          value_type f, dt, normg, step_unscaled;
          int nfeval;

          value_type evaluate_func (const Vector<value_type>& newx, Vector<value_type>& newg, bool verbose = false) {
            nfeval++;
            value_type cost = func (newx, newg);
            if (!std::isfinite (cost))
              throw Exception ("cost function is NaN or Inf!");
            if (verbose)
              CONSOLE ("      << eval " + str(nfeval) + ", f = " + str (cost) + " >>");
            return cost;
          }


          void compute_normg_and_step_unscaled () {
            normg = step_unscaled = norm (g);
            if (preconditioner_weights.size()) {
              value_type g_projected = 0.0;
              step_unscaled = 0.0;
              for (size_t n = 0; n < g.size(); ++n) {
                step_unscaled += Math::pow2 (g[n]);
                g_projected += preconditioner_weights[n] * Math::pow2 (g[n]);
                g[n] *= preconditioner_weights[n];
              }
              normg = g_projected / normg;
              step_unscaled = Math::sqrt (step_unscaled);
            }
          }

      };
    //! @}
  }
}

#endif

