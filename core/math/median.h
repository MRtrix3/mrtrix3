/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __math_median_h__
#define __math_median_h__

#include <algorithm>
#include <limits>

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


    // Weiszfeld median
    template <class MatrixType = Eigen::Matrix<default_type, 3, Eigen::Dynamic>, class  VectorType = Eigen::Matrix<default_type, 3, 1>>
    bool median_weiszfeld(const MatrixType& X, VectorType& median, const size_t numIter = 300, const default_type precision = 0.00001) {
      assert(X.cols() >= 2 && "cannot compute weiszfeld median for less than two points");
      assert(X.rows() >= 2 && "weisfeld median for one dimensional data is not unique. did you mean the median?");
      const size_t dimensionality = X.rows();

      // initialise to the centroid
      assert(median.rows() >= 2);
      assert(median.rows() == X.rows());
      median = X.transpose().colwise().mean();
      size_t m = X.cols();
      // If the init point is in the set of points, we shift it:
      size_t n = (X.colwise() - median).colwise().squaredNorm().nonZeros();
      while (n != m){ // (X.colwise() - median).colwise().squaredNorm().transpose().colwise().minCoeff() > 0.000001;
        median(0) += 10 * precision;
        n = (X.colwise() - median).colwise().squaredNorm().nonZeros();
      }

      bool convergence = false;
      vector<default_type> dist(numIter);

      // Minimizing the sum of the squares of the distances between each point in 'X' and the median.
      size_t i = 0;
      Eigen::Matrix<default_type, Eigen::Dynamic, 1> s1;
      s1.resize(dimensionality,1);
      while ( !convergence && (i < numIter) ) {
        default_type norm = 0.0;
        s1.setZero();
        default_type denum = 0.0;
        default_type sdist = 0.0;
        for (size_t j = 0; j < m; ++j){
          norm = (X.col(j) - median).norm();
          s1 += X.col(j) / norm;
          denum += 1.0 / norm;
          sdist += norm;
        }
        dist[i] = sdist;
        if (denum == 0.0 or std::isnan(denum)){
          WARN( "Couldn't compute geometric median!" );
          break;
        }

        median = s1 / denum;
        if (i > 3){
          convergence=(std::abs(dist[i]-dist[i-2])<precision);
        }
        ++i;
      }
      if (i == numIter)
        WARN( "Weiszfeld's median algorithm did not converge after "+str(numIter)+" iterations");
      // std::cerr << str(dist) << std::endl;
      return convergence;
    }
  }
}


#endif

