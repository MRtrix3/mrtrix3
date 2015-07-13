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

#ifndef __dwi_noise_estimator_h__
#define __dwi_noise_estimator_h__

#include "image.h"
#include "dwi/gradient.h"
#include "math/least_squares.h"
#include "math/SH.h"
#include "dwi/Sn_scale_estimator.h"


namespace MR {
  namespace DWI {

    namespace {

      template <class InputImageType, class OutputImageType, class MatrixType>
        class NoiseEstimatorFunctor {
          public:

            NoiseEstimatorFunctor (const MatrixType& SH2amp_mapping, int axis, InputImageType& dwi, OutputImageType& noise) :
              dwi (dwi),
              noise (noise),
              H (SH2amp_mapping * Math::pinv (SH2amp_mapping)),
              S (H.cols(), dwi.size (axis)),
              R (S.cols(), S.rows()),
              leverage (H.rows()), 
              axis (axis) {
                for (ssize_t n = 0; n < leverage.size(); ++n) 
                  leverage[n] = H(n,n) < 1.0 ? 1.0 / std::sqrt (1.0 - H(n,n)) : 1.0;
              }

            void operator () (const Iterator& pos) {
              assign_pos_of (pos).to (dwi, noise);
              for (auto l = Loop (axis) (dwi); l; ++l)  
                for (auto l2 = Loop (3) (dwi); l2; ++l2) 
                  S(dwi.index(3), dwi.index(axis)) = dwi.value();

              R.noalias() = H.selfadjointView<Lower>() * S - S;

              for (auto l = Loop (axis) (noise); l; ++l) {
                R.col (noise.index (axis)).array() *= leverage.array();
                noise.value() = scale_estimator (R.col (noise.index (axis)));
              }
            }

          protected:
            InputImageType dwi;
            OutputImageType noise;
            Eigen::MatrixXd H, S, R;
            Eigen::VectorXd leverage;
            Sn_scale_estimator<default_type> scale_estimator;
            int axis;
        };
    }




    template <class InputImageType, class OutputImageType, class MatrixType> 
      inline void estimate_noise (InputImageType& dwi, OutputImageType& noise, const MatrixType& SH2amp_mapping) {
        ThreadedLoop loop ("estimating noise level...", dwi, 0, 3);
        NoiseEstimatorFunctor<InputImageType,OutputImageType,MatrixType> functor (SH2amp_mapping, loop.inner_axes()[0], dwi, noise);
        loop.run_outer (functor);
      } 


  }
}

#endif

