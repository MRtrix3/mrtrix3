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

      template <class Set, typename T> 
        inline Math::Matrix<T>& set_default (Math::Matrix<T>& M, const Set& ds)
        {
          M.allocate(4,4);
          M.identity();
          M(0,3) = -0.5 * (ds.dim(0)-1) * ds.vox(0);
          M(1,3) = -0.5 * (ds.dim(1)-1) * ds.vox(1);
          M(2,3) = -0.5 * (ds.dim(2)-1) * ds.vox(2);
          return (M);
        }


      template <class Set, typename T> 
        inline Math::Matrix<T>& voxel2image (Math::Matrix<T>& M, const Set& ds) 
        {
          M.allocate(4,4);
          M.zero();
          M(0,0) = ds.vox(0);
          M(1,1) = ds.vox(1);
          M(2,2) = ds.vox(2);
          M(3,3) = 1.0;
          return (M);
        }


      template <class Set, typename T> 
        inline Math::Matrix<T>& image2voxel (Math::Matrix<T>& M, const Set& ds) 
        {
          M.allocate(4,4);
          M.zero();
          M(0,0) = 1.0/ds.vox(0);
          M(1,1) = 1.0/ds.vox(1);
          M(2,2) = 1.0/ds.vox(2);
          M(3,3) = 1.0;
          return (M);
        }


      template <class Set, typename T> 
        inline Math::Matrix<T>& scanner2image (Math::Matrix<T>& M, const Set& ds) 
        {
          M.copy (ds.transform());
          return (M);
        }


      template <class Set, typename T> 
        inline Math::Matrix<T>& image2scanner (Math::Matrix<T>& M, const Set& ds) 
        {
          int signum;
          Math::Permutation p (4);
          Math::Matrix<T> D;
          D.copy (ds.transform());
          Math::LU::decomp (D, p, signum);
          Math::LU::inv (M, D, p);
          M(3,0) = M(3,1) = M(3,2) = 0.0; M(3,3) = 1.0;
          return (M);
        }


      template <class Set, typename T> 
        inline Math::Matrix<T>& voxel2scanner (Math::Matrix<T>& M, const Set& ds) 
        {
          M.copy (ds.transform());
          for (size_t i = 0; i < 3; i++) 
            for (size_t j = 0; j < 3; j++) 
              M(i,j) *= ds.vox(i);
          return (M);
        }


      template <class Set, typename T> 
        inline Math::Matrix<T>& scanner2voxel (Math::Matrix<T>& M, const Set& ds) 
        {
          int signum;
          Math::Permutation p (4);
          Math::Matrix<T> D (4,4);
          voxel2scanner (D, ds);
          Math::LU::decomp (D, p, signum);
          Math::LU::inv (M, D, p);
          M(3,0) = M(3,1) = M(3,2) = 0.0; M(3,3) = 1.0;
          return (M);
        }

      template <typename T, class P1, class P2> 
        inline void apply (P1& y, const Math::Matrix<T>& M, const P2& x)
        {
          y[0] = M(0,0)*x[0] + M(0,1)*x[1] + M(0,2)*x[2] + M(0,3);
          y[1] = M(1,0)*x[0] + M(1,1)*x[1] + M(1,2)*x[2] + M(1,3);
          y[2] = M(2,0)*x[0] + M(2,1)*x[1] + M(2,2)*x[2] + M(2,3);
        }


    }
    //! @}
  }
}

#endif

