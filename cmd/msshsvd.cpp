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


#include "command.h"
#include "image.h"
#include "math/SH.h"

#include <Eigen/Dense>
#include <Eigen/SVD>

#define DEFAULT_LMAX 4
#define DEFAULT_NSUB 10000

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
    "The rank is set with the parameter -lmax. For lmax=4 (default), the data "
    "is projected onto components of order 4, 2, and 0, leading to a rank = 22.";

  ARGUMENTS
  + Argument ("in", "the input MSSH data.").type_image_in()

  + Argument ("rf", "the output basis functions.").type_file_out().allow_multiple();

  OPTIONS
  + Option ("mask", "image mask")
    + Argument ("m").type_file_in()

  + Option ("lmax", "maximum SH order (default = " + str(DEFAULT_LMAX) + ")")
    + Argument ("order").type_integer(0, 30)

  + Option ("nsub", "number of voxels in subsampling (default = " + str(DEFAULT_NSUB) + ")")
    + Argument ("vox").type_integer(0)

  + Option ("weights", "vector of weights per shell (default = ones)")
    + Argument ("w").type_file_in()

  + Option ("proj", "output low-rank MSSH projection")
    + Argument ("mssh").type_image_out();

}

using value_type = float;

Eigen::Vector3i getRandomPosInMask(const Image<value_type>& in, Image<bool>& mask)
{
  Eigen::Vector3i pos;
  while (true) {
    pos[0] = std::rand() % in.size(0);
    pos[1] = std::rand() % in.size(1);
    pos[2] = std::rand() % in.size(2);
    if (mask.valid()) {
      assign_pos_of(pos).to(mask);
      if (!mask.value()) continue;
    }
    return pos;
  }
}

class SHSVDProject
{ MEMALIGN (SHSVDProject)
  public:
    SHSVDProject (const Image<value_type>& in, const int l, const Eigen::MatrixXf& P)
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
  auto in = Image<value_type>::open(argument[0]);

  auto mask = Image<bool>();
  auto opt = get_options("mask");
  if (opt.size()) {
    mask = Image<bool>::open(opt[0][0]);
    check_dimensions(in, mask, 0, 3);
  }

  int nshells = in.size(3);

  int lmax = get_option_value("lmax", DEFAULT_LMAX);
  if (Math::SH::NforL(lmax) > in.size(4)) {
    throw Exception("lmax too large for input image dimension.");
  }

  int nrf = std::min(lmax/2 + 1, nshells);
  if (argument.size() != nrf+1)
    throw Exception("no. output arguments does not match desired lmax.");
  vector<Eigen::MatrixXf> rf;
  for (int n = 0; n < nrf; n++) {
    rf.push_back( Eigen::MatrixXf::Zero(nshells, lmax/2+1-n) );
  }

  opt = get_options("weights");
  Eigen::VectorXf W (nshells);
  W.setOnes();
  if (opt.size()) {
    W = load_vector<float>(opt[0][0]).cwiseSqrt();
    if (W.size() != nshells)
      throw Exception("provided weights do not match the no. shells.");
  }

  auto proj = Image<value_type>();
  opt = get_options("proj");
  bool pout = opt.size();
  if (pout) {
    Header header (in);
    proj = Image<value_type>::create(opt[0][0], header);
  }

  // Select voxel subset
  size_t nsub = get_option_value("nsub", DEFAULT_NSUB);
  vector<Eigen::Vector3i> pos;
  for (size_t i = 0; i < nsub; i++) {
    pos.push_back(getRandomPosInMask(in, mask));
  }

  // Compute SVD per SH order l
  Eigen::MatrixXf Sl;
  for (int l = 0; l <= lmax; l+=2)
  {
    // load data to matrix
    Sl.resize(nshells, nsub*(2*l+1));
    size_t i = 0;
    for (auto p : pos) {
      assign_pos_of(p).to(in);
      for (size_t j = Math::SH::NforL(l-2); j < Math::SH::NforL(l); j++, i++) {
        in.index(4) = j;
        Sl.col(i) = Eigen::VectorXf(in.row(3));
      }
    }
    // low-rank SVD
    Eigen::JacobiSVD<Eigen::MatrixXf> svd (W.asDiagonal() * Sl, Eigen::ComputeThinU | Eigen::ComputeThinV);
    int rank = std::min((lmax-l)/2 + 1, nshells);
    Eigen::MatrixXf U = svd.matrixU().block(0,0,nshells,rank);
    // save basis functions
    for (int n = 0; n < rank; n++) {
      rf[n].col(l/2) = W.asDiagonal().inverse() * U.col(n);
    }
    // save low-rank projection
    if (pout) {
      Eigen::MatrixXf P = W.asDiagonal().inverse() * U * U.adjoint() * W.asDiagonal();
      SHSVDProject func (in, l, P);
      ThreadedLoop(in, {0, 1, 2}).run(func, in, proj);
    }
  }

  // Write basis functions to file
  for (int n = 0; n < nrf; n++) {
    save_matrix(rf[n], argument[n+1]);
  }

}





