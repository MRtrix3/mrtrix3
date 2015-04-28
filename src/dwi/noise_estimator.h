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
#include "math/matrix.h"
#include "math/least_squares.h"
#include "math/SH.h"
#include "dwi/Sn_scale_estimator.h"


namespace MR {
  namespace DWI {

    namespace {

      template <class InputImageType, class OutputImageType, typename ValueType>
        class NoiseEstimatorFunctor {
          public:

            NoiseEstimatorFunctor (const Math::Matrix<ValueType>& SH2amp_mapping, int axis, InputImageType& dwi, OutputImageType& noise) :
              dwi (dwi),
              noise (noise),
              axis (axis) {

                Math::Matrix<ValueType> iSH = Math::pinv (SH2amp_mapping);
                Math::mult (H, SH2amp_mapping, iSH);

                S.allocate (H.columns(), dwi.size (axis));
                R.allocate (S);

                leverage.allocate (H.rows());
                for (size_t n = 0; n < leverage.size(); ++n) 
                  leverage[n] = H(n,n) < 1.0 ? 1.0 / std::sqrt (1.0 - H(n,n)) : 1.0;

              }

            void operator () (const Iterator& pos) {
              assign_pos_of (pos).to (dwi, noise);
              for (auto l = Loop (axis, axis+1) (dwi); l; ++l)  
                for (auto l2 = Loop (3) (dwi); l2; ++l2) 
                  S(dwi.index(3), dwi.index(axis)) = dwi.value();

              Math::mult (R, CblasLeft, ValueType(0.0), ValueType(1.0), CblasUpper, H, S);
              R -= S;

              for (auto l = Loop (axis, axis+1) (noise); l; ++l) {
                R.column (noise.index (axis)) *= leverage;
                noise.value() = scale_estimator (R.column (noise.index (axis)));
              }
            }

          protected:
            InputImageType dwi;
            OutputImageType noise;
            Math::Matrix<ValueType> H, S, R;
            Math::Vector<ValueType> leverage;
            Sn_scale_estimator<ValueType> scale_estimator;
            int axis;
        };
    }




    template <class InputImageType, class OutputImageType, class ValueType> 
      inline void estimate_noise (InputImageType& dwi, OutputImageType& noise, const Math::Matrix<ValueType>& SH2amp_mapping) {
        ThreadedLoop loop ("estimating noise level...", dwi, 0, 3);
        NoiseEstimatorFunctor<InputImageType,OutputImageType,ValueType> functor (SH2amp_mapping, loop.inner_axes()[0], dwi, noise);
        loop.run_outer (functor);
      } 


  }
}

#endif

