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

#include "image/buffer_preload.h"
#include "image/voxel.h"
#include "image/threaded_loop.h"
#include "dwi/gradient.h"
#include "math/matrix.h"
#include "math/least_squares.h"
#include "math/SH.h"
#include "dwi/Sn_scale_estimator.h"


namespace MR {
  namespace DWI {

    template <class InputVoxelType, class OutputVoxelType, class ValueType>
      class NoiseEstimatorFunctor {
      public:

        typedef ValueType value_type;

        NoiseEstimatorFunctor (InputVoxelType& dwi, OutputVoxelType& noise, const Math::Matrix<value_type>& SH2amp_mapping, int axis) :
          dwi (dwi),
          noise (noise),
          axis (axis) {

            Math::Matrix<value_type> iSH = Math::pinv (SH2amp_mapping);
            Math::mult (H, SH2amp_mapping, iSH);

            S.allocate (H.columns(), dwi.dim(axis));
            R.allocate (S);

            leverage.allocate (H.rows());
            for (size_t n = 0; n < leverage.size(); ++n) 
              leverage[n] = H(n,n) < 1.0 ? 1.0 / Math::sqrt (1.0 - H(n,n)) : 1.0;

          }

        void operator () (const Image::Iterator& pos) {
          Image::voxel_assign (dwi, pos);
          for (dwi[axis] = 0; dwi[axis] < dwi.dim(axis); ++dwi[axis]) 
            for (dwi[3] = 0; dwi[3] < dwi.dim(3); ++dwi[3]) 
              S(dwi[3],dwi[axis]) = dwi.value();

          Math::mult (R, CblasLeft, value_type(0.0), value_type(1.0), CblasUpper, H, S);
          R -= S;

          Image::voxel_assign (noise, pos);
          for (noise[axis] = 0; noise[axis] < noise.dim(axis); ++noise[axis]) {
            R.column(noise[axis]) *= leverage;
            noise.value() = scale_estimator (R.column (noise[axis]));
          }
        }

      protected:
        InputVoxelType dwi;
        OutputVoxelType noise;
        Math::Matrix<value_type> H, S, R;
        Math::Vector<value_type> leverage;
        Sn_scale_estimator<value_type> scale_estimator;
        int axis;
    };




    class NoiseEstimator : public Image::ConstInfo 
    {
      public:
        template <class InfoType>
        NoiseEstimator (const InfoType& dwi_info) :
          Image::ConstInfo (dwi_info) { 
            Image::Info::set_ndim (3);
          }

        template <class InputVoxelType, class OutputVoxelType, class ValueType> 
          inline void operator() (InputVoxelType& dwi, OutputVoxelType& noise, const Math::Matrix<ValueType>& SH2amp_mapping) {
            Image::ThreadedLoop loop ("estimating noise level...", dwi, 1, 0, 3);
            NoiseEstimatorFunctor<InputVoxelType,OutputVoxelType,ValueType> functor (dwi, noise, SH2amp_mapping, loop.inner_axes()[0]);
            loop.run_outer (functor);
          } 

    };

  }
}

#endif

