#include "command.h"
#include "debug.h"
#include "image.h"
#include "math/gradient_descent_bb.h"
#include "math/gradient_descent.h"
#include "math/rng.h"

using namespace MR;
using namespace App;
using namespace Math;


void usage ()
{
  AUTHOR = "Joe Bloggs (joe.bloggs@acme.org)";

  DESCRIPTION
  + "test";

  ARGUMENTS
  + Argument ("verbose", "yesno").type_bool();

  OPTIONS
  + Option ( "precondition", " " )
  + Option ( "mvn", " " )
  + Option ( "noise", " " )
    + Argument ("std", "float").type_float();
}

struct MVN {
    MVN (Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic>& covariance,
        Eigen::Matrix<default_type, Eigen::Dynamic, 1>& mu,
        default_type noise = 0.0):
        S (covariance.inverse()),
        mu (mu),
        noise (noise),
        f(1.0 / (std::pow(std::sqrt(2 * Math::pi),mu.size()) * covariance.determinant())) {
            assert(covariance.cols() == mu.size());
            assert(covariance.rows() == mu.size());
            MAT(S);
            VEC(mu);
        }
    default_type operator() (const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x, Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {
      gradient.setZero();
      if (noise > 0.0) {
        xn = x;
        for (size_t i=0; i<x.size(); ++i) { xn(i) += noise * rnd(); }
        diff = xn-mu;
      } else {
        diff = x-mu;
      }
      default_type cost = - f * std::exp(-0.5 * (diff).transpose() * S  * (diff) );
      gradient = - cost * S * (diff);
      return cost;
    }
    typedef default_type value_type;

    size_t size() const { return mu.size(); }
    double init (Eigen::VectorXd& x) { x.resize(mu.size()); x.fill(0.0); return 1.0; }
private:
    const Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic> S;
    const Eigen::Matrix<default_type, Eigen::Dynamic, 1> mu;
    const default_type noise, f;
    Eigen::Matrix<default_type, Eigen::Dynamic, 1> diff, xn;
    Math::RNG::Uniform<default_type> rnd;
};

// f(x) = Sum_i a_i^-1 * (x_i - centre_i)^2 + 1.0
struct QuadraticProblem {
    QuadraticProblem (
        const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& a,
        const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& centre,
        default_type noise = 0.0):
        a (a.cwiseInverse()),
        centre (centre),
        noise (noise) {
            VEC(a);
            VEC(centre);
        }
    default_type operator() (const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x, Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {

      gradient.setZero();
      if (noise > 0.0) {
        xn = x;
        for (ssize_t i=0; i<x.size(); ++i) { xn(i) += noise * rnd(); }
        diff = xn - centre;
      } else {
        diff = x - centre;
      }
      gradient = 2.0 * a.array() * diff.array();
      default_type cost = (a.array() * diff.array().square()).sum() + 1.0;
      return cost;
    }
    typedef default_type value_type;

    size_t size() const { return centre.size(); }
    double init (Eigen::VectorXd& x) { x.resize(centre.size()); x.fill(0.0); return 1.0; }
private:
    const Eigen::Matrix<default_type, Eigen::Dynamic, 1> a, centre;
    const default_type noise;
    Eigen::Matrix<default_type, Eigen::Dynamic, 1> diff, xn;
    Math::RNG::Uniform<default_type> rnd;
};

template <class FunType, class OptimType>
void optimise (FunType func, OptimType& optim, bool verbose,
      const Eigen::Matrix<default_type,
      Eigen::Dynamic, 1>& weights) {
    Eigen::Matrix<default_type, Eigen::Dynamic, 1> x;
    func.init(x);
    optim.be_verbose(verbose);
    if (weights.size()){
        INFO("preconditioning");
        optim.precondition (weights);
    }
    if (verbose)
        optim.run(100000, 1e-30, std::cout.rdbuf());
    else
        optim.run(100000, 1e-30);
}

void run ()
{
    bool verbose = argument[0].as_bool();
    auto opt = get_options ("precondition");
    bool precondition = opt.size();
    bool mvn = get_options ("mvn").size();

    default_type noise (0.0);
    opt = get_options ("noise");
    if (opt.size()) noise = parse_floats(opt[0][0])[0];
    VAR(noise);



    const size_t dim(2);
    Eigen::Matrix<default_type, Eigen::Dynamic, 1> ev(dim);
    ev << 1.0,30.0;
    Eigen::Matrix<default_type, Eigen::Dynamic, 1> mu(dim);
    mu << -10.1,100;
    Eigen::Matrix<default_type, Eigen::Dynamic, 1> weights;
    if (precondition){
        weights = ev;
        INFO("weights: " + str(weights.transpose()));
    }

    if (mvn) {
        Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic> cov(dim, dim);
        cov = Eigen::DiagonalMatrix<default_type, Eigen::Dynamic, Eigen::Dynamic>(ev);
        auto func = MVN(cov, mu, noise);
        auto optim = GradientDescent<decltype(func)>(func, Math::LinearUpdate(), 10.0, 0.1);
        optimise (func, optim, verbose, weights);
        CONSOLE("GradientDescent:   n = " + str(optim.function_evaluations()));
        CONSOLE("GradientDescent:   f = " + str(optim.value()));
        CONSOLE("GradientDescent:   x = " + str(optim.state().transpose()));

        auto optim2 = GradientDescentBB<decltype(func)>(func);
        optimise (func, optim2, verbose, weights);
        CONSOLE("GradientDescentBB: n = " + str(optim2.function_evaluations()));
        CONSOLE("GradientDescentBB: f = " + str(optim2.value()));
        CONSOLE("GradientDescentBB: x = " + str(optim2.state().transpose()));
    } else {
        auto func = QuadraticProblem(ev, mu, noise);
        auto optim = GradientDescent<decltype(func)>(func, Math::LinearUpdate(), 10.0, 0.1);
        optimise (func, optim, verbose, weights);
        CONSOLE("GradientDescent:   n = " + str(optim.function_evaluations()));
        CONSOLE("GradientDescent:   f = " + str(optim.value()));
        CONSOLE("GradientDescent:   x = " + str(optim.state().transpose()));

        auto optim2 = GradientDescentBB<decltype(func)>(func);
        optimise (func, optim2, verbose, weights);
        CONSOLE("GradientDescentBB: n = " + str(optim2.function_evaluations()));
        CONSOLE("GradientDescentBB: f = " + str(optim2.value()));
        CONSOLE("GradientDescentBB: x = " + str(optim2.state().transpose()));
    }
}
