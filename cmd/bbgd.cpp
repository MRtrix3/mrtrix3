#include "command.h"
#include "debug.h"
#include "image.h"
#include "math/gradient_descent_bb.h"
#include "math/gradient_descent.h"

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
}

struct MVN {
    MVN(Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic>& covariance,
        Eigen::Matrix<default_type, Eigen::Dynamic, 1>& mu):
        S(covariance.inverse()),
        mu(mu),
        f(1.0 / (std::pow(std::sqrt(2 * Math::pi),mu.size()) * covariance.determinant())) {
            assert(covariance.cols() == mu.size());
            assert(covariance.rows() == mu.size());
            MAT(S);
            VEC(mu);
        }
    default_type operator() (const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x, Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {
      gradient.setZero();
      default_type cost = - f * std::exp(-0.5 * (x-mu).transpose() * S  * (x-mu) );
      gradient = - cost * S * (x - mu);
      return cost;
    }
    typedef default_type value_type;

    size_t size() const { return mu.size(); }
    double init (Eigen::VectorXd& x) { x.resize(mu.size()); x.fill(0.0); return 1.0; }
private:
    const Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic> S;
    const Eigen::Matrix<default_type, Eigen::Dynamic, 1> mu;
    const default_type f;
};

void run ()
{
    bool verbose = argument[0].as_bool();
    const size_t dim(4);
    Eigen::Matrix<default_type, Eigen::Dynamic, 1> ev(dim);
    ev << 1.0,10,100,1000;
    Eigen::Matrix<default_type, Eigen::Dynamic, 1> mu(dim);
    mu << -0.1,0.1,-1,1;

    Eigen::Matrix<default_type, Eigen::Dynamic, 1> weights(dim);
    weights = ev;

    Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic> cov(dim, dim);
    cov = Eigen::DiagonalMatrix<default_type, Eigen::Dynamic, Eigen::Dynamic>(ev);
    auto func = MVN(cov, mu);
    Eigen::Matrix<default_type, Eigen::Dynamic, 1> x(dim);
    func.init(x);
    auto optim = GradientDescent<MVN>(func);
    // optim.precondition (weights);
    optim.run(100000, 1e-30, verbose, -1, 1e-30);
    auto optim2 = GradientDescentBB<MVN>(func);
    optim2.be_verbose(verbose);
    // optim2.precondition (weights);
    if (verbose)
        optim2.run(100000, 1e-30, std::cout.rdbuf());
    else
        optim2.run(100000, 1e-30);
    CONSOLE("GradientDescentBB: n = " + str(optim2.function_evaluations()));
    CONSOLE("GradientDescentBB: f = " + str(optim2.value()));
    CONSOLE("GradientDescentBB: x = " + str(optim2.state().transpose()));
    CONSOLE("GradientDescent:   n = " + str(optim.function_evaluations()));
    CONSOLE("GradientDescent:   f = " + str(optim.value()));
    CONSOLE("GradientDescent:   x = " + str(optim.state().transpose()));
}
