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

#ifndef __dwi_gradient_h__
#define __dwi_gradient_h__

#include "math/matrix.h"
#include "point.h"

namespace MR {
  namespace DWI {

    template <typename T> Math::Matrix<T>& normalise_grad (Math::Matrix<T>& grad)
    {
      if (grad.columns() != 4) throw Exception ("invalid gradient matrix dimensions");
      for (size_t i = 0; i < grad.rows(); i++) {
        T norm = grad(i,3) ? 1.0/sqrt(grad(i,0)*grad(i,0)+grad(i,1)*grad(i,1)+grad(i,2)*grad(i,2)) : 0.0;
        grad(i,0) *= norm;
        grad(i,1) *= norm;
        grad(i,2) *= norm;
      }
      return (grad);
    }


    template <typename T> inline Math::Matrix<T> grad2bmatrix (Math::Matrix<T> &bmat, const Math::Matrix<T> &grad)
    {
      bmat.allocate (grad.rows(),7);
      for (size_t i = 0; i < grad.rows(); i++) {
        bmat(i,0) = grad(i,3) * grad(i,0)*grad(i,0);
        bmat(i,1) = grad(i,3) * grad(i,1)*grad(i,1);
        bmat(i,2) = grad(i,3) * grad(i,2)*grad(i,2);
        bmat(i,3) = grad(i,3) * 2*grad(i,0)*grad(i,1);
        bmat(i,4) = grad(i,3) * 2*grad(i,0)*grad(i,2);
        bmat(i,5) = grad(i,3) * 2*grad(i,1)*grad(i,2);
        bmat(i,6) = -1.0;
      }
      return (bmat);
    }




    template <typename T> inline void dwi2tensor (const Math::Matrix<T>& binv, T *d)
    {
      T logs[binv.columns()];
      for (size_t i = 0; i < binv.columns(); i++) logs[i] = d[i] > 0.0 ? -log(d[i]) : 0.0;
      for (size_t i = 0; i < 6; i++) {
        d[i] = 0.0;
        for (size_t j = 0; j < binv.columns(); j++)
          d[i] += binv(i,j)*logs[j];
      }
    }


    template <typename T> inline void guess_DW_directions (std::vector<int>& dwi, std::vector<int>& bzero, const Math::Matrix<T>& grad)
    {
      if (grad.columns() != 4) throw Exception ("invalid gradient encoding matrix: expecting 4 columns.");
      dwi.clear();
      bzero.clear();
      for (size_t i = 0; i < grad.rows(); i++) {
        if (grad(i,3)) dwi.push_back (i);
        else bzero.push_back (i);
      }
    }




    template <typename T> inline Math::Matrix<T>& gen_direction_matrix (Math::Matrix<T>& dirs, const Math::Matrix<T>& grad, const std::vector<int>& dwi)
    {
      dirs.allocate (dwi.size(),2);
      for (size_t i = 0; i < dwi.size(); i++) {
        T norm = Point (grad(dwi[i],0), grad(dwi[i],1), grad(dwi[i],2)).norm();
        dirs(i,0) = atan2 (grad(dwi[i],1), grad(dwi[i],0));
        dirs(i,1) = acos (grad(dwi[i],2)/norm);
      }
      return (dirs);
    }


  }
}

#endif

