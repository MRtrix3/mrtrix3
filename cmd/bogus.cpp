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
SET_AUTHOR (NULL);
SET_COPYRIGHT (NULL);

DESCRIPTION = {
  "this is used to test stuff.",
  NULL
};

ARGUMENTS = { Argument::End }; 

OPTIONS = { Option::End };





typedef float T;

EXECUTE {

  // create instance of Matrix:
  Math::Matrix<float> M (10,10);

  // set all elements to zero:
  M = 0.0;

  // set diagonal elements to 1 (this uses the diagonal() function,
  // which returns a Matrix::View):
  M.diagonal() = 1.0;
  VAR (M);

  // create instance of Vector from data file:
  Math::Vector<double> V ("mydatafile.txt");
  VAR (V);


  // set every other element of the bottom row to the contents of V
  // (in this case, this assumes that V has size 5):
  M.row(9).sub(0,10,2) = V;

  VAR (M);

  M.sub (0,4,6,10) = 3.0;

  VAR(M);

  M.sub(6,10,0,4).diagonal() = 5.0;

  VAR(M);

  // in the above, M.row(9) returns the 9th row as a Vector::View,
  // and the .sub(0,10,2) return a Vector::View of this, skipping
  // every other element (i.e. stride 2)

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

