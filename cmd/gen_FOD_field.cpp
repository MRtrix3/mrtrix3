/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 28/10/09.

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


#include "app.h"
#include "point.h"
#include "math/SH.h"
#include "image/voxel.h"
#include "math/vector.h"
#include "dataset/loop.h"


using namespace MR; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "generate simulated FOD field.",
  NULL
};

ARGUMENTS = {
  Argument ("dim", "dimensions", "the image dimensions, as comma-separated 3-vector of ints.").type_string(),
  Argument ("coefs", "SH coefficients", "even, m=0 SH coefficients of the profile for a fibre population oriented along the z-axis.").type_file(),
  Argument ("FOD", "output FOD image", "the output image containing the SH coefficients of the simulated FOD field.").type_image_out(),
  Argument::End
};


OPTIONS = {
  Option ("lmax", "lmax", "maximum harmonic order (default: determined from coefficients provided).")
    .append (Argument ("num", "number", "harmonic order.").type_integer (0, 8, 20)),

  Option::End
};


class Kernel {
  public:
    Kernel (const Math::Vector<float>& coef, size_t num_per_side) :
      RH (coef.size()), N (num_per_side), lmax (2*(coef.size()-1)), nSH (Math::SH::NforL (lmax)), SH (nSH), V (nSH) {
      Math::SH::SH2RH (RH, coef);
    }

    size_t size () const { return (nSH); }

    void operator () (Image::Voxel<float>& D) { 
      SH.view() = 0.0;
      float xp = N*(D.dim(0)/2.0 - D.pos(0) - 1) + 0.5;
      float yp = N*(D.pos(1)-0.5) + 0.5;
      for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
          Point dir (y+yp, x+xp, 0.0);
          dir.normalise();
          Math::SH::delta (V.view(), dir, lmax);
          Math::SH::sconv (V.view(), RH, V);
          SH.view() += V;
        }
      }
      for (D.pos(3,0); D.pos(3) < D.dim(3); D.move(3,1)) D.value (SH[D.pos(3)]/float(Math::pow2(N)));
    }

  private:
    Math::Vector<float> RH;
    int N, lmax, nSH;
    Math::Vector<float> SH, V;
};


EXECUTE {
  std::vector<int> D = parse_ints (argument[0].get_string()); 
  if (D.size() != 3) throw Exception ("number of dimensions must be 3");
  for (size_t i = 0; i < 3; ++i) 
    if (D[i] < 1) throw Exception ("dimensions must be greater than zero");

  Math::Vector<float> coef (argument[1].get_string());
  std::vector<OptBase> opt = get_options (0);
  if (opt.size()) {
    int n = (opt[0][0].get_int()/2)+1;
    if (size_t(n) < coef.size()) coef.resize(n);
  }
  Kernel kernel (coef, 10);

  Image::Header header;
  header.axes.ndim() = 4;
  header.axes.dim(0) = D[0];
  header.axes.dim(1) = D[1];
  header.axes.dim(2) = D[2];
  header.axes.dim(3) = kernel.size();
  header.axes.vox(0) = header.axes.vox(1) = header.axes.vox(2) = 2.0;
  header.axes.order(0) = 1; header.axes.forward(0) = true;
  header.axes.order(1) = 2; header.axes.forward(1) = true;
  header.axes.order(2) = 3; header.axes.forward(2) = true;
  header.axes.order(3) = 0; header.axes.forward(3) = true;


  const Image::Header FOD_header = argument[2].get_image (header);
  Image::Voxel<float> vox (FOD_header);

  DataSet::loop1 ("generating FOD field...", kernel, vox, 0, 3);
}

