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
#include "filter/resize.h"
#include "registration/transform/reorient.h"
#include "registration/transform/compose.h"
#include "registration/transform/convert.h"
#include "filter/warp.h"

namespace MR
{
  namespace Registration
  {

    class SyN
    {

      public:

        typedef float value_type;

        SyN ():
          max_iter (1, 500),
          scale_factor (2),
          grad_smoothing (0.0),
          disp_smoothing (0.0),
          gradient_step (0.2) {
            scale_factor[0] = 0.5;
            scale_factor[1] = 1;
        }




        template <class MidWayImageType, class MetricType, class TransformType, class Im1ImageType, class Im2ImageType, class Im1MaskType, class Im2MaskType>
        void run_masked (
          MidWayImageType& midway_image,
          MetricType& metric,
          transform_type im1_affine,
          transform_type im2_affine,
          Im1ImageType& im1_image,
          Im2ImageType& im2_image,
          Im1MaskType& im1_mask,
          Im2MaskType& im2_mask) {


            if (max_iter.size() == 1)
              max_iter.resize (scale_factor.size(), max_iter[0]);
            else if (max_iter.size() != scale_factor.size())
              throw Exception ("the max number of SyN iterations needs to be defined for each multi-resolution level");

            for (size_t level = 0; level < scale_factor.size(); level++) {
                CONSOLE ("SyN: multi-resolution level " + str(level + 1) + ", scale factor: " + str(scale_factor[level]));

                // Resize midway image
                Filter::Resize resize_filter (midway_image);
                resize_filter.set_scale_factor (scale_factor[level]);
                resize_filter.set_interp_type (1);
                auto midway_resized = Image<typename Im1ImageType::value_type>::scratch (resize_filter);

                // Smooth input images
                Filter::Smooth im1_smooth_filter (im1_image);
                im1_smooth_filter.set_stdev (1.0 / (2.0 * scale_factor[level]));
                auto im1_smoothed = Image<typename Im1ImageType::value_type>::scratch (im1_smooth_filter);

                Filter::Smooth im2_smooth_filter (im2_image);
                im2_smooth_filter.set_stdev (1.0 / (2.0 * scale_factor[level]));  // TODO compare this with SyNFOD line489
                auto im2_smoothed = Image<typename Im1ImageType::value_type>::scratch (im2_smooth_filter);

                {
                  LogLevelLatch log_level (0);
                  resize_filter (midway_image, midway_resized);
                  im1_smooth_filter (im1_image, im1_smoothed);
                  im2_smooth_filter (im2_image, im2_smoothed);
                }

                if (grad_smoothing == 0.0)
                  grad_smoothing = midway_resized.size(0) + midway_resized.size(1) + midway_resized.size(2); //TODO check this with ANTS

                if (disp_smoothing == 0.0)
                  grad_smoothing = (midway_resized.size(0) + midway_resized.size(1) + midway_resized.size(2)) / 3.0;

                Header field_header (midway_resized);
                field_header.set_ndim(4);
                field_header.size(3) = 3;

                //Initialise fields
                if (level == 0) {
                  im1_disp_field.reset (new Image<float> (Image<float>::scratch (field_header)));
                  im1_disp_field_inv.reset (new Image<float> (Image<float>::scratch (field_header)));
                  im2_disp_field.reset (new Image<float> (Image<float>::scratch (field_header)));
                  im2_disp_field_inv.reset (new Image<float> (Image<float>::scratch (field_header)));
                //Upsample
                } else {
                  im1_disp_field = std::move (reslice (*im1_disp_field, field_header));
                  im1_disp_field_inv = std::move (reslice (*im1_disp_field_inv, field_header));
                  im2_disp_field = std::move (reslice (*im2_disp_field, field_header));
                  im2_disp_field_inv = std::move (reslice (*im2_disp_field_inv, field_header));
                }

                //Setup energy curve
                //Grad_Step_altered = grad_step.

//                Registration::Metric::SynDemons<Interp::Linear> metric (im1_smoothed, im2_smoothed, im1_mask, im2_mask);
                bool converged = false;
                while (!converged) {





                  // TODO look at composing on the fly (not sure what is faster with FOD reorientation)
//                  Image<float> im1_deform_field = Image<float>::scratch (field_header);
//                  Registration::Transform::compose_affine_displacement (im1_affine, im1_disp_field, im1_deform_field);

//                  Image<float> im2_deform_field = Image<float>::scratch (field_header);
//                  Registration::Transform::compose_affine_displacement (im2_affine, im2_disp_field, im2_deform_field);

//                  Image<float> im1_update_field = Image<float>::scratch (field_header);
//                  Image<float> im2_update_field = Image<float>::scratch (field_header);

//                  default_type energy = metric (im1_deform_field, im1_deform_field, im1_update_field, im1_update_field);


//                  store energy of current interation (from metric)

//                  Smooth update field (making sure boundary is still identity)

//                  Normalise updated field
//                  Normalise updatedInv field

//                  Compose updated and displacements transforms

//                  Smooth displacement field

//                  Invert fields x 2

                    //current_iteration++

                    //track the energy profile to check for convergence.

                }

            }
          }


          void set_max_iter (const std::vector<size_t>& maxiter) {
            for (size_t i = 0; i < maxiter.size (); ++i)
              if (maxiter[i] < 0)
                throw Exception ("the number of iterations must be positive");
            max_iter = maxiter;
          }


          void set_scale_factor (const std::vector<default_type>& scalefactor) {
            for (size_t i = 0; i < scalefactor.size(); ++i)
              if (scalefactor[i] < 0)
                throw Exception ("the multi-resolution scale factor must be positive");
            scale_factor = scalefactor;
          }


        protected:

          std::unique_ptr<Image<float> > reslice (Image<float>& image, Header& header) {
            std::unique_ptr<Image<float> > temp (new Image<float> (Image<float>::scratch (header)));
            Filter::reslice<Interp::Cubic> (image, *temp);
            return temp;
          }

          std::vector<size_t> max_iter;
          std::vector<default_type> scale_factor;
          default_type grad_smoothing;
          default_type disp_smoothing;
          default_type gradient_step;

          std::unique_ptr<Image<float> > im1_disp_field;
          std::unique_ptr<Image<float> > im1_disp_field_inv;
          std::unique_ptr<Image<float> > im2_disp_field;
          std::unique_ptr<Image<float> > im2_disp_field_inv;


          std::vector<default_type> gradientDescentParameters;
          std::vector<default_type> energy;
          std::vector<default_type> last_energy;
          std::vector<size_t> energy_bad;


//          unsigned int m_BSplineFieldOrder;
//          ArrayType m_GradSmoothingMeshSize;
//          ArrayType m_TotalSmoothingMeshSize;

    };
  }
}

#endif
