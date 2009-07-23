/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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
#include "image/voxel.h"
#include "math/rng.h"
#include "math/cholesky.h"
#include "math/complex.h"
#include "math/least_squares.h"

using namespace MR; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "this is used to test stuff.",
  NULL
};


ARGUMENTS = { Argument::End }; 
OPTIONS = { Option::End };

typedef float T;

EXECUTE {
  Image::Header header;
  header.axes.resize (3);
  header.axes[0].dim = 1024;
  header.axes[1].dim = 1024;
  header.axes[2].dim = 1024;

  header.axes[0].vox = 1.0;
  header.axes[1].vox = 1.0;
  header.axes[2].vox = 1.0;

  header.axes[0].order = 0;
  header.axes[1].order = 1;
  header.axes[2].order = 2;

  VAR (header.data_type.description());
  //header.data_type = DataType::UInt8;

  Image::Object obj;
  obj.create ("poo.mif", header);
  
  Image::Voxel vox (obj);
  vox[0] = 1023;
  vox[1] = 1023;
  vox[2] = 1023;

  vox.value() = 0.0;


  /*
  Math::RNG rng (1);

  Math::Matrix<T> M (4,4);
  for (size_t j = 0; j < M.columns(); j++) 
    for (size_t i = 0; i < M.rows(); i++) 
      M(i,j) = rng.normal();

  Math::Matrix<T> M2 (4,4);
  for (size_t j = 0; j < M2.columns(); j++) 
    for (size_t i = 0; i < M2.rows(); i++) 
      M2(i,j) = rng.normal();

  std::cout << "M = [ " << M << "];\n\n";
  std::cout << "M2 = [ " << M2 << "];\n\n";

  Math::Matrix<T> M3;
  std::cout << "M*M2 = [ " << Math::mult (M3, M, M2) << "];\n\n";

  Math::Matrix<T> M4 (M);
  std::cout << "LU::inv(M) = [ " << Math::LU::inv (M3.view(), M4) << "];\n\n";

  std::cout << "inv(M)*M = [ " << Math::mult (M2, M3, M) << "];\n\n";

  Math::Matrix<T> S;
  std::cout << "M*M^T = [ " << Math::mult (S, T(1.0), CblasNoTrans, M, CblasTrans, M) << "];\n\n";

  Math::Matrix<T> IS (S);
  std::cout << "Cholesky::inv(M*M^T) = [ " << Math::Cholesky::inv (IS) << "];\n\n";

  std::cout << "Cholesky::inv(M*M^T)*M*M^T = [ " << Math::mult (M3, IS, S) << "];\n\n";

  M.allocate (10,5);
  for (size_t j = 0; j < M.columns(); j++) 
    for (size_t i = 0; i < M.rows(); i++) 
      M(i,j) = rng.normal();

  std::cout << "M = [ " << M << "];\n\n";

  std::cout << "pinv(M) = [ " << Math::pinv (M2,M) << "];\n\n";
  std::cout << "pinv(M)*M = [ " << Math::mult (M3, M2, M) << "];\n\n";


  Math::Matrix<cfloat> C (4,4);
  for (size_t j = 0; j < C.columns(); j++) {
    for (size_t i = 0; i < C.rows(); i++) {
      C(i,j).real() = rng.normal();
      C(i,j).imag() = rng.normal();
    }
  }

  
  std::cout << "C = [ " << C << "];\n\n";
  */
}

