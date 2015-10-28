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

#ifndef __math_median_h__
#define __math_median_h__

#include <vector>
#include <limits>

#include <algorithm>

#include "types.h"


namespace MR
{
  namespace Math
  {

    namespace {
      template <typename X>
        inline bool not_a_number (X x) {
          return false;
        }

      template <> inline bool not_a_number (float x) { return std::isnan (x); }
      template <> inline bool not_a_number (double x) { return std::isnan (x); }
    }



    template <class Container>
    inline typename Container::value_type median (Container& list)
    {
        size_t num = list.size();
        // remove NaNs:
        for (size_t n = 0; n < num; ++n) {
          while (not_a_number (list[n]) && n < num) {
            --num;
            //std::swap (list[n], list[num]);
            // Commented std::swap to provide bool compatibility
            typename Container::value_type temp = list[num]; list[num] = list[n]; list[n] = temp;
          }
        }
        if (!num)
          return std::numeric_limits<typename Container::value_type>::quiet_NaN();

        size_t middle = num/2;
        std::nth_element (list.begin(), list.begin()+middle, list.begin()+num);
        typename Container::value_type med_val = list[middle];
        if (!(num&1U)) {
          --middle;
          std::nth_element (list.begin(), list.begin()+middle, list.begin()+middle+1);
          med_val = (med_val + list[middle])/2.0;
        }
        return med_val;
    }
    template <class MatrixType = Eigen::Matrix<default_type, 3, Eigen::Dynamic>, class  VectorType = Eigen::Matrix<default_type, 3, 1>>
    void geometric_median3(const MatrixType& X, VectorType& median, size_t numIter = 300) {
      // Weiszfeld median
      // initialise to the centroid
      assert(X.rows() == 3);
      assert(X.cols() >= 2);
      assert(median.rows() >= 3);
      median = X.transpose().colwise().mean();
      size_t m = X.cols();
      // If the init point is in the set of points, we shift it:
      size_t n = (X.colwise() - median).colwise().squaredNorm().nonZeros();
      while (n != m){ // (X.colwise() - median).colwise().squaredNorm().transpose().colwise().minCoeff() > 0.000001;
        median(0) += 0.0001;
        median(1) += 0.0001;
        median(2) += 0.0001;
        n = (X.colwise() - median).colwise().squaredNorm().nonZeros();
      }

      bool convergence = false;
      std::vector<default_type> dist(numIter);

      // Minimizing the sum of the squares of the distances between each points in 'X' and the median.
      size_t i = 0;
      while ( !convergence && (i < numIter) ) {
        default_type norm = 0.0;
        Eigen::Matrix<default_type, 3, 1> s1(0.0, 0.0, 0.0);
        default_type denum = 0.0;
        default_type sdist = 0.0;
        for (size_t j = 0; j < m; ++j){
          norm = (X.col(j) - median).norm();
          s1 += X.col(j) / norm;
          denum += 1.0 / norm;
          sdist += norm;
        }
        dist[i] = sdist;
        if (denum == 0.0){
          WARN( "Couldn't compute geometric median!" );
          return;
        }

        median = s1 / denum;
        if (i > 3){
          convergence=(std::abs(dist[i]-dist[i-2])<0.00001);
        }
        ++i;
      }
      if (i == numIter)
        throw Exception( "Weiszfeld's median did not converge after "+str(numIter)+" iterations");
      // VAR(dist);
      return;
    }


  }
}


#endif

