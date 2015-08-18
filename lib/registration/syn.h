/*
 Copyright 2012 Brain Research Institute, Melbourne, Australia

 Written by David Raffelt, 28/11/2012

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

#ifndef __registration_syn_h__
#define __registration_syn_h__

#include <vector>
#include "image.h"

namespace MR
{
    namespace Registration
    {

    class SyN
    {

      public:

        typedef float value_type;

        SyN (){}

//          run_masked
//
////
//          value_type smooth_grad =  template_header.vox(0) + template_header.vox(1) + template_header.vox(2);
//          if (opt.size())
//            smooth_grad = opt[0][0];
//
//          opt = get_options ("smooth_disp");
//          value_type smooth_disp =  (template_header.vox(0) + template_header.vox(1) + template_header.vox(2)) / 3.0;
//          void set_max_iter (const std::vector<int>& maxiter) {
//            for (size_t i = 0; i < maxiter.size (); ++i)
//              if (maxiter[i] < 0)
//                throw Exception ("the number of iterations must be positive");
//            max_iter = maxiter;
//          }
//
//          void set_scale_factor (const std::vector<float>& scalefactor) {
//            for (size_t i = 0; i < scale_factors.size(); ++i)
//              if (scale_factors[i] < 0)
//                throw Exception ("the multi-resolution scale factor must be positive");
//            scale_factor = scalefactor;
//          }
//
//        protected:
//          std::vector<int> max_iter;
//          std::vector<float> scale_factor;
//
//          Math::Matrix affine_transform;
//          Image::BufferScratch deformation;
//          Image::BufferScratch inverse_deformation;
//
//          std::vector<float> m_GradientDescentParameters;
//
//          float gradient_smoothing;
//          float deformation_smoothing;
//          float gradient_step;
//
//          std::vector<float> energy;
//          std::vector<float> last_energy;
//          std::vector<unsigned int> energy_bad;
//
//          Image::BufferScratch template_deformation;
//          Image::BufferScratch template_inverse_deformation;
//          Image::BufferScratch moving_deformation;
//          Image::BufferScratch moving_inverse_deformation;

//          unsigned int m_BSplineFieldOrder;
//          ArrayType m_GradSmoothingMeshSize;
//          ArrayType m_TotalSmoothingMeshSize;

    };
  }
}

#endif
