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

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "Daan Christiaens";

  SYNOPSIS = "Low-rank SH-SVD projection of multi-shell SH data.";

  DESCRIPTION
  + "This command takes multi-shell SH data and computes the reduced "
    "rank projection with SH-SVD.";

  ARGUMENTS
  + Argument ("in", "the input MSSH data.").type_image_in()
  + Argument ("out", "the output MSSH data.").type_file_out();

  OPTIONS
  + Option ("mask", "image mask")
    + Argument ("m").type_file_in()
  + Option ("lmax", "maximum SH order (default = " + str(DEFAULT_LMAX) + ")")
    + Argument ("order").type_integer(0, 30);

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
        out.row(3) = P * Eigen::Vector3f(in.row(3));
      }
    }

  private:
    const int l;
    const Eigen::MatrixXf& P;

};


void run ()
{
  auto in = Image<value_type>::open(argument[0]);

  Header header (in);
  auto out = Image<value_type>::create(argument[1], header);

  auto mask = Image<bool>();
  auto opt = get_options("mask");
  if (opt.size()) {
    mask = Image<bool>::open(opt[0][0]);
    check_dimensions(in, mask, 0, 3);
  }

  size_t nshells = in.size(3);

  int lmax = get_option_value("lmax", DEFAULT_LMAX);
  // TODO: check image dimensions...

  size_t nsub = 1000;
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
    // low-rank project
    Eigen::JacobiSVD<Eigen::MatrixXf> svd (Sl, Eigen::ComputeThinU | Eigen::ComputeThinV);
    int rank = (lmax-l)/2 + 1;
    Eigen::MatrixXf U = svd.matrixU().block(0,0,nshells,rank);
    Eigen::MatrixXf P = U * U.adjoint();
    // save to output
    SHSVDProject func (in, l, P);
    ThreadedLoop(in, {0, 1, 2}).run(func, in, out);
  }

}





