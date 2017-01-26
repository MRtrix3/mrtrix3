/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */


#include "command.h"
#include "image.h"
#include "math/rng.h"


using namespace MR;
using namespace App;
using namespace Eigen;


void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  DESCRIPTION
  + "test new .row() method";

  REQUIRES_AT_LEAST_ONE_ARGUMENT = false;
}



void run ()
{
  Header H;
  H.ndim() = 4;
  H.size(0) = H.size(1) = H.size(2) = 16;
  H.size(3) = 3;
  
  auto a = Image<float>::scratch (H);
  auto b = Image<float>::scratch (H);
  auto c = Image<float>::scratch (H);

  Math::RNG::Normal<float> rng;

  for (auto l = Loop(a)(a, b); l; ++l) {
    a.value() = rng();
    b.value() = rng();
  }

#define TEST for (auto l = Loop("with row", a, 0, 3)(a, b, c); l; ++l) 
#define CHECK for (auto l = Loop("without row", a)(a, b, c); l; ++l) { float result = c.value(); 
#define END if (c.value() != result) { WARN ("mismatch: " + str(result) + " vs. " + str(c.value())); break; } }



  TEST { c.row(3) = a.row(3); }
  CHECK c.value() = a.value(); END

  TEST { c.row(3) = b.row(3); c.row(3) += a.row(3); }
  CHECK c.value() = b.value() + a.value(); END

  TEST { c.row(3) = Vector3f (b.row(3)) + Vector3f (a.row(3)); }
  CHECK c.value() = b.value() + a.value(); END


  Matrix3f M = MatrixXf::Random(3,3);

  TEST { c.row(3) = M * Vector3f (a.row(3)); }
  for (auto l = Loop(a,0,3)(a, b); l; ++l) {
    Vector3f x;
    for (auto m = Loop(3)(a); m; ++m)
      x[a.index(3)] = a.value();
    Vector3f y = M*x;
    for (auto m = Loop(3)(b); m; ++m)
      b.value() = y[b.index(3)];
  }
  CHECK c.value() = b.value(); END


  TEST { c.row(3) = b.row(3); c.row(3) += M * Vector3f (a.row(3)); }
  for (auto l = Loop(a,0,3)(a, b); l; ++l) {
    Vector3f x;
    for (auto m = Loop(3)(a); m; ++m)
      x[a.index(3)] = a.value();
    Vector3f y = M*x;
    for (auto m = Loop(3)(b); m; ++m)
      b.value() += y[b.index(3)];
  }
  CHECK c.value() = b.value(); END


  TEST { VectorXd x (b.row(3)); c.row(3) = Vector3d(a.row(3)) + M.cast<double>() * x; }
  for (auto l = Loop(a,0,3)(a, b); l; ++l) {
    Vector3d x;
    for (auto m = Loop(3)(b); m; ++m)
      x[b.index(3)] = b.value();
    Vector3d y = M.cast<double>()*x;
    for (auto m = Loop(3)(a,b); m; ++m)
      b.value() = a.value() + y[b.index(3)];
  }
  CHECK c.value() = b.value(); END

  
  TEST { c.row(3) = Vector3d(a.row(3)) - M.cast<double>() * Vector3d(b.row(3)); }
  for (auto l = Loop(a,0,3)(a, b); l; ++l) {
    Vector3d x;
    for (auto m = Loop(3)(b); m; ++m)
      x[b.index(3)] = b.value();
    Vector3d y = M.cast<double>()*x;
    for (auto m = Loop(3)(a,b); m; ++m)
      b.value() = a.value() - y[b.index(3)];
  }
  CHECK c.value() = b.value(); END
  
  TEST { Vector3f x; x.col(0) = a.row(3); c.row(3) = x; }
  CHECK c.value() = a.value(); END

}

