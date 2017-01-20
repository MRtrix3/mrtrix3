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
#include "progressbar.h"
#include "datatype.h"

#include "image.h"
#include "algo/threaded_loop.h"
 

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "compare two peak images for differences, within specified tolerance.";

  ARGUMENTS
  + Argument ("peaks1", "a peaks image.").type_image_in ()
  + Argument ("peaks2", "another peaks image.").type_image_in ()
  + Argument ("tolerance", "the dot product difference to consider acceptable").type_float (0.0);
}


void run ()
{
  auto in1 = Image<double>::open (argument[0]);
  auto in2 = Image<double>::open (argument[1]);
  check_dimensions (in1, in2);
  if (in1.ndim() != 4)
    throw Exception ("images \"" + in1.name() + "\" and \"" + in2.name() + "\" are not 4D");
  if (in1.size(3) % 3)
    throw Exception ("images \"" + in1.name() + "\" and \"" + in2.name() + "\" do not contain XYZ peak directions");    
  for (size_t i = 0; i < in1.ndim(); ++i) {
    if (std::isfinite (in1.size(i)))
      if (in1.size(i) != in2.size(i))
        throw Exception ("images \"" + in1.name() + "\" and \"" + in2.name() + "\" do not have matching voxel spacings " +
                                       str(in1.size(i)) + " vs " + str(in2.size(i)));
  }
  for (size_t i  = 0; i < 3; ++i) {
    for (size_t j  = 0; j < 4; ++j) {
      if (std::abs (in1.transform().matrix()(i,j) - in2.transform().matrix()(i,j)) > 0.0001)
        throw Exception ("images \"" + in1.name() + "\" and \"" + in2.name() + "\" do not have matching header transforms "
                           + "\n" + str(in1.transform().matrix()) + "vs \n " + str(in2.transform().matrix()) + ")");
    }
  }


  double tol = argument[2];

  ThreadedLoop (in1, 0, 3)
  .run ([&tol] (decltype(in1)& a, decltype(in2)& b)
  {
    for (size_t i = 0; i != size_t(a.size(3)); i += 3) {
      Eigen::Vector3 veca, vecb;
      for (size_t axis = 0; axis != 3; ++axis) {
        a.index(3) = b.index(3) = i + axis;
        veca[axis] = a.value();
        vecb[axis] = b.value();
      }
      const double norma = veca.norm(), normb = vecb.norm();
      veca.normalize(); vecb.normalize();
      const double dp = std::abs (veca.dot (vecb));
      if (1.0 - dp > tol || std::abs (norma - normb) > tol)
        throw Exception ("images \"" + a.name() + "\" and \"" + b.name() + "\" do not match within specified precision of " + str(tol) + " ( [" + str(veca.transpose().cast<float>()) + "] vs [" + str(vecb.transpose().cast<float>()) + "], norms [" + str(norma) + " " + str(normb) + "], dot product = " + str(dp) + ")");
    }
  }, in1, in2);

  CONSOLE ("data checked OK");
}

