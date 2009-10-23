/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 24/05/09.

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

#ifndef __dataset_transform_h__
#define __dataset_transform_h__

#include "math/matrix.h"
#include "math/LU.h"

namespace MR {
  namespace DataSet {
    
    //! \addtogroup DataSet
    // @{

    namespace Transform {

      template <class Set, typename T> inline Math::MatrixView<T>& P2I (Math::MatrixView<T>& M, const Set& ds) 
      {
        assert (M.rows() == 4 && M.columns() == 4);
        M.zero();
        M(0,0) = ds.vox(0);
        M(1,1) = ds.vox(1);
        M(2,2) = ds.vox(2);
        M(3,3) = 1.0;
        return (M);
      }
      template <class Set, typename T> inline Math::Matrix<T>& P2I (Math::Matrix<T>& M, const Set& ds) 
      { M.allocate(4,4); P2I(M.view(), ds); return (M); }


      template <class Set, typename T> inline Math::MatrixView<T>& I2P (Math::MatrixView<T>& M, const Set& ds) {
        assert (M.rows() == 4 && M.columns() == 4);
        M.zero();
        M(0,0) = 1.0/ds.vox(0);
        M(1,1) = 1.0/ds.vox(1);
        M(2,2) = 1.0/ds.vox(2);
        M(3,3) = 1.0;
        return (M);
      }
      template <class Set, typename T> inline Math::Matrix<T>& I2P (Math::Matrix<T>& M, const Set& ds) 
      { M.allocate(4,4); I2P(M.view(), ds); return (M); }


      template <class Set, typename T> inline Math::MatrixView<T>& R2I (Math::MatrixView<T>& M, const Set& ds) {
        assert (M.rows() == 4 && M.columns() == 4);
        assert (ds.transform().rows() == 4 && ds.transform().columns() == 4);
        M = ds.transform();
        return (M);
      }
      template <class Set, typename T> inline Math::Matrix<T>& R2I (Math::Matrix<T>& M, const Set& ds) 
      { M.allocate(4,4); R2I(M.view(), ds); return (M); }


      template <class Set, typename T> inline Math::MatrixView<T>& I2R (Math::MatrixView<T>& M, const Set& ds) {
        assert (M.rows() == 4 && M.columns() == 4);
        assert (ds.transform().rows() == 4 && ds.transform().columns() == 4);
        int signum;
        Math::Permutation p (4);
        Math::Matrix<T> D (ds.transform());
        Math::LU::decomp (D.view(), p, signum);
        Math::LU::inv (M, D, p);
        M(3,0) = M(3,1) = M(3,2) = 0.0; M(3,3) = 1.0;
        return (M);
      }
      template <class Set, typename T> inline Math::Matrix<T>& I2R (Math::Matrix<T>& M, const Set& ds) 
      { M.allocate(4,4); I2R(M.view(), ds); return (M); }


      template <class Set, typename T> inline Math::MatrixView<T>& P2R (Math::MatrixView<T>& M, const Set& ds) {
        assert (M.rows() == 4 && M.columns() == 4);
        assert (ds.transform().rows() == 4 && ds.transform().columns() == 4);
        M = ds.transform();
        for (size_t i = 0; i < 3; i++) 
          for (size_t j = 0; j < 3; j++) 
            M(i,j) *= ds.vox(i);
        return (M);
      }
      template <class Set, typename T> inline Math::Matrix<T>& P2R (Math::Matrix<T>& M, const Set& ds) 
      { M.allocate(4,4); P2R(M.view(), ds); return (M); }


      template <class Set, typename T> inline Math::MatrixView<T>& R2P (Math::MatrixView<T>& M, const Set& ds) {
        assert (M.rows() == 4 && M.columns() == 4);
        assert (ds.transform().rows() == 4 && ds.transform().columns() == 4);
        int signum;
        Math::Permutation p (4);
        Math::Matrix<T> D (ds.transform());
        Math::LU::decomp (D.view(), p, signum);
        Math::LU::inv (M, D, p);
        for (size_t i = 0; i < 3; i++) 
          for (size_t j = 0; j < 3; j++) 
            M(i,j) /= ds.vox(j);
        M(3,0) = M(3,1) = M(3,2) = 0.0; M(3,3) = 1.0;
        return (M);
      }
      template <class Set, typename T> inline Math::Matrix<T>& R2P (Math::Matrix<T>& M, const Set& ds) 
      { M.allocate(4,4); R2P(M.view(), ds); return (M); }

    }
    //! @}
  }
}

#endif

