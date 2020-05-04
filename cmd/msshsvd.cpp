/* Copyright (c) 2017-2019 Daan Christiaens
 *
 * MRtrix and this add-on module are distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 */

#include "command.h"
#include "image.h"
#include "math/SH.h"

#include <Eigen/Dense>
#include <Eigen/SVD>


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "Daan Christiaens (daan.christiaens@kcl.ac.uk)";

  SYNOPSIS = "SH-SVD decomposition of multi-shell SH data.";

  DESCRIPTION
  + "This command expects a 5-D MSSH image (shells on 4th dimension; "
    "SH coefficients in the 5th). The command will compute the optimal "
    "orthonormal basis for representing the input data using the singular "
    "value decomposition across shells and SH frequency bands (SH-SVD)."

  + "Optionally, the command can output the low-rank projection of the input. "
    "The rank is set with the parameter -lmax. For lmax=4,2,0 (default), the data "
    "is projected onto components of order 4, 2, and 0, leading to a rank = 22.";

  ARGUMENTS
  + Argument ("in", "the input MSSH data.").type_image_in()

  + Argument ("rf", "the output basis functions.").type_file_out().allow_multiple();

  OPTIONS
  + Option ("mask", "image mask")
    + Argument ("m").type_file_in()

  + Option ("lmax", "maximum SH order per component (default = 4,2,0)")
    + Argument ("order").type_sequence_int()
    
  + Option ("lbreg", "Laplace-Beltrami regularisation weight (default = 0)")
    + Argument ("lambda").type_float(0.0)

  + Option ("weights", "vector of weights per shell (default = ones)")
    + Argument ("w").type_file_in()

  + Option ("proj", "output low-rank MSSH projection")
    + Argument ("mssh").type_image_out();

}

using value_type = float;

class SHSVDProject
{ MEMALIGN (SHSVDProject)
  public:
    SHSVDProject (const int l, const Eigen::MatrixXf& P)
      : l(l), P(P)
    { }

    void operator() (Image<value_type>& in, Image<value_type>& out)
    {
      for (size_t k = Math::SH::NforL(l-2); k < Math::SH::NforL(l); k++) {
        in.index(4) = k;
        out.index(4) = k;
        out.row(3) = P * Eigen::VectorXf(in.row(3));
      }
    }

  private:
    const int l;
    const Eigen::MatrixXf& P;
};


void run ()
{
  auto in = Image<value_type>::open(argument[0]).with_direct_io({3,4});

  auto mask = Image<bool>();
  auto opt = get_options("mask");
  if (opt.size()) {
    mask = Image<bool>::open(opt[0][0]);
    check_dimensions(in, mask, 0, 3);
  }

  int nshells = in.size(3);
  size_t nvox = in.size(0)*in.size(1)*in.size(2);

  vector<int> lmax;
  opt = get_options("lmax");
  if (opt.size()) {
    lmax = opt[0][0].as_sequence_int();
  } else {
    lmax = {4,2,0};
  }
  std::sort(lmax.begin(), lmax.end(), std::greater<int>());   // sort in place
  if (Math::SH::NforL(lmax[0]) > in.size(4)) 
    throw Exception("lmax too large for input dimension.");

  int nrf = lmax.size();
  if (argument.size() != nrf+1)
    throw Exception("no. output arguments does not match desired lmax.");
  if (nrf > nshells)
    throw Exception("no. basis functions can't exceed no. shells.");
  vector<Eigen::MatrixXf> rf;
  for (int l : lmax) {
    rf.push_back( Eigen::MatrixXf::Zero(nshells, l/2+1) );
  }

  Eigen::VectorXf W (nshells); W.setOnes();
  auto key = in.keyval().find("shellcounts");
  opt = get_options("weights");
  if (opt.size()) {
    W = load_vector<float>(opt[0][0]).cwiseSqrt();
    if (W.size() != nshells)
      throw Exception("provided weights do not match the no. shells.");
  } else if (key != in.keyval().end()) {
    size_t i = 0;
    for (auto w : parse_floats(key->second))
      W[i++] = std::sqrt(w);
  }
  W /= W.mean();

  // LB regularization
  float lam = get_option_value("lbreg", 0.0f);
  Eigen::VectorXf lb (nshells); lb.setZero();
  auto bvals = parse_floats(in.keyval().find("shells")->second);
  for (size_t i = 0; i < nshells; i++) {
    float b = bvals[i]/1000.f;
    lb[i] = (b < 1e-2) ? 0.0f : lam/(b*b) ;
  }

  auto proj = Image<value_type>();
  opt = get_options("proj");
  bool pout = opt.size();
  if (pout) {
    Header header (in);
    proj = Image<value_type>::create(opt[0][0], header);
  }

  // Compute SVD per SH order l
  for (int l = 0; l <= lmax[0]; l+=2)
  {
    // define LB filter
    Eigen::VectorXf lbfilt = Math::pow2(l*(l+1)) * lb;
    lbfilt.array() += 1.0f; lbfilt = lbfilt.cwiseInverse();
    // load data to matrix
    Eigen::MatrixXf Sl (nshells, nvox*(2*l+1)); Sl.setZero();
    auto loop = Loop(0,3);
    size_t i = 0;
    for (auto v = loop(in); v; v++) {
      if (mask.valid()) {
        assign_pos_of(in).to(mask);
        if (!mask.value()) continue;
      }
      for (size_t j = Math::SH::NforL(l-2); j < Math::SH::NforL(l); j++, i++) {
        in.index(4) = j;
        Sl.col(i) = lbfilt.asDiagonal() * Eigen::VectorXf(in.row(3));
      }
    }
    // low-rank SVD
    Eigen::JacobiSVD<Eigen::MatrixXf> svd (W.asDiagonal() * Sl.leftCols(i), Eigen::ComputeFullU);
    int rank = std::upper_bound(lmax.begin(), lmax.end(), l, std::greater<int>()) - lmax.begin();
    Eigen::MatrixXf U = svd.matrixU().leftCols(rank);
    // save basis functions
    for (int n = 0; n < rank; n++) {
      rf[n].col(l/2) = W.asDiagonal().inverse() * U.col(n);
    }
    // save low-rank projection
    if (pout) {
      Eigen::MatrixXf P = W.asDiagonal().inverse() * U * U.adjoint() * W.asDiagonal();
      SHSVDProject func (l, P);
      ThreadedLoop(in, 0, 3).run(func, in, proj);
    }
  }

  // Write basis functions to file
  for (int n = 0; n < nrf; n++) {
    save_matrix(rf[n], argument[n+1]);
  }

}

