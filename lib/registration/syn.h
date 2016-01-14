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
#include "filter/warp.h"
#include "filter/resize.h"
#include "registration/transform/reorient.h"
#include "registration/transform/compose.h"
#include "registration/transform/affine.h"
#include "registration/transform/convert.h"
#include "registration/transform/normalise.h"
#include "registration/metric/syn_demons.h"
#include "image/average_space.h"

namespace MR
{
  namespace Registration
  {

    class SyN
    {

      public:

        SyN ():
          max_iter (1, 500),
          scale_factor (2),
          update_smoothing (3.0),
          disp_smoothing (0.5),
          gradient_step (0.2),
          fod_reorientation (false) {
            scale_factor[0] = 0.5;
            scale_factor[1] = 1;
        }




        template <class TransformType, class Im1ImageType, class Im2ImageType, class Im1MaskType, class Im2MaskType>
          void run_masked (TransformType linear_transform,
                           Im1ImageType& im1_image,
                           Im2ImageType& im2_image,
                           Im1MaskType& im1_mask,
                           Im2MaskType& im2_mask) {


            transform_type im1_affine = linear_transform.get_transform_half();
            transform_type im2_affine = linear_transform.get_transform_half_inverse();

            DEBUG ("Estimating halfway space");
            std::vector<Eigen::Transform<double, 3, Eigen::Projective> > init_transforms;
            // define transfomations that will be applied to the image header when the common space is calculated
            {
              Eigen::Transform<double, 3, Eigen::Projective> init_trafo_2 = linear_transform.get_transform_half();
              Eigen::Transform<double, 3, Eigen::Projective> init_trafo_1 = linear_transform.get_transform_half_inverse();
              init_transforms.push_back (init_trafo_2);
              init_transforms.push_back (init_trafo_1);
            }

            auto padding = Eigen::Matrix<default_type, 4, 1>(0.0, 0.0, 0.0, 0.0);
            std::vector<Header> headers;
            headers.push_back (im2_image.original_header());
            headers.push_back (im1_image.original_header());
            auto midway_image_header = compute_minimum_average_header<default_type, Eigen::Transform<default_type, 3, Eigen::Projective>>(headers, 1.0, padding, init_transforms);
            auto midway_image = Header::scratch (midway_image_header).get_image<default_type>();

            if (max_iter.size() == 1)
              max_iter.resize (scale_factor.size(), max_iter[0]);
            else if (max_iter.size() != scale_factor.size())
              throw Exception ("the max number of SyN iterations needs to be defined for each multi-resolution level");

            // TODO Normalise the input images??

            for (size_t level = 0; level < scale_factor.size(); level++) {
                CONSOLE ("SyN: multi-resolution level " + str(level + 1) + ", scale factor: " + str(scale_factor[level]));

                INFO ("Resizing midway image");
                // Resize midway image based on current level
                Filter::Resize resize_filter (midway_image);
                resize_filter.set_scale_factor (scale_factor[level]);
                resize_filter.set_interp_type (1);
                resize_filter.datatype() = DataType::Float64;
                auto midway_resized = Image<default_type>::scratch (resize_filter);

                INFO ("Smoothing imput images");
                // Smooth input images based on multi-resolution pyramid
                Filter::Smooth im1_smooth_filter (im1_image);
                im1_smooth_filter.set_stdev (1.0 / (2.0 * scale_factor[level]));
                auto im1_smoothed = Image<default_type>::scratch (im1_smooth_filter);
                Filter::Smooth im2_smooth_filter (im2_image);
                im2_smooth_filter.set_stdev (1.0 / (2.0 * scale_factor[level]));  // TODO compare this with SyNFOD line489
                auto im2_smoothed = Image<default_type>::scratch (im2_smooth_filter);
                {
                  LogLevelLatch log_level (0);
                  resize_filter (midway_image, midway_resized);
                  im1_smooth_filter (im1_image, im1_smoothed);
                  im2_smooth_filter (im2_image, im2_smoothed);
                }

                Header field_header (midway_resized);
                field_header.set_ndim(4);
                field_header.size(3) = 3;

                if (level == 0) {
                  INFO ("Initialising fields");
                  im1_disp_field = std::make_shared<Image<default_type>>(Image<default_type>::scratch (field_header));
                  im1_disp_field_inv = std::make_shared<Image<default_type>>(Image<default_type>::scratch (field_header));
                  im2_disp_field = std::make_shared<Image<default_type>>(Image<default_type>::scratch (field_header));
                  im2_disp_field_inv = std::make_shared<Image<default_type>>(Image<default_type>::scratch (field_header));
                } else {
                  INFO ("Upsampling fields");
                  im1_disp_field = reslice (*im1_disp_field, field_header);
                  im1_disp_field_inv = reslice (*im1_disp_field_inv, field_header);
                  im2_disp_field = reslice (*im2_disp_field, field_header);
                  im2_disp_field_inv = reslice (*im2_disp_field_inv, field_header);
                }

                INFO ("Initalising scratch buffers");
                Header warped_header (midway_resized);
                if (im1_image.ndim() == 4) {
                  warped_header.set_ndim(4);
                  warped_header.size(3) = im1_image.size(3);
                }
                auto im1_warped = Image<default_type>::scratch (midway_resized);
                auto im2_warped = Image<default_type>::scratch (midway_resized);
                auto im1_update_field = Image<default_type>::scratch (field_header);
                auto im2_update_field = Image<default_type>::scratch (field_header);

                std::vector<default_type> cost_function_vector;
                default_type grad_step_altered = gradient_step;
                //TODO alter grad step

                bool converged = false;
                ssize_t iteration = 0;
                default_type update_smoothing_mm = update_smoothing * ((midway_image.spacing(0) + midway_image.spacing(1) + midway_image.spacing(2)) / 3.0);
                default_type disp_smoothing_mm = disp_smoothing * ((midway_image.spacing(0) + midway_image.spacing(1) + midway_image.spacing(2)) / 3.0);


                while (!converged) {

                  CONSOLE ("Iteration: " + str(iteration));
                  // compose current displacement field with affine
                  Image<default_type> im1_deform_field = Image<default_type>::scratch (field_header);
                  Registration::Transform::compose_affine_displacement (im1_affine, *im1_disp_field, im1_deform_field);
                  Image<default_type> im2_deform_field = Image<default_type>::scratch (field_header);
                  Registration::Transform::compose_affine_displacement (im2_affine, *im2_disp_field, im2_deform_field);


                  INFO ("warping input images");
                  Filter::warp<Interp::Linear> (im1_smoothed, im1_warped, im1_deform_field, 0.0);
                  Filter::warp<Interp::Linear> (im2_smoothed, im2_warped, im2_deform_field, 0.0);
                  save (im1_warped, std::string("im1_warped_iter" + str(iteration) + ".mif"), false);
                  save (im2_warped, std::string("im2_warped_iter" + str(iteration) + ".mif"), false);

                  display (im1_warped);


                  if (fod_reorientation) {
                    INFO ("Reorienting FODs");
                    Registration::Transform::reorient_warp (im1_warped, im1_deform_field, aPSF_directions);
                    Registration::Transform::reorient_warp (im2_warped, im2_deform_field, aPSF_directions);
                  }

                  INFO ("warping mask images");
                  Im1MaskType im1_mask_warped;
                  if (im1_mask.valid()) {
                    im1_mask_warped = Im1MaskType::scratch (midway_resized);
                    Filter::warp<Interp::Linear> (im1_mask, im1_mask_warped, im1_deform_field, 0.0);
                    save (im1_mask_warped, std::string("im1_mask_warped_iter" + str(iteration) + ".mif"));
                  }
                  Im1MaskType im2_mask_warped;
                  if (im2_mask.valid()) {
                    im2_mask_warped = Im1MaskType::scratch (midway_resized);
                    Filter::warp<Interp::Linear> (im2_mask, im2_mask_warped, im2_deform_field, 0.0);
                    save (im2_mask_warped, std::string("im2_mask_warped_iter" + str(iteration) + ".mif"));
                  }

                  default_type global_cost = 0.0;
                  size_t global_voxel_count = 0;
                  Metric::SyNDemons<Im1ImageType, Im2ImageType, Im1MaskType, Im2MaskType> syn_metric (global_cost, global_voxel_count, im1_warped, im2_warped, im1_mask_warped, im2_mask_warped);
                  ThreadedLoop (midway_resized, 0, 3).run (syn_metric, im1_warped, im2_warped, im1_update_field, im2_update_field);
                  cost_function_vector.push_back (global_cost);


                  // TODO make sure the boundary is remains identity (check if this is only needed for neighbourhood metrics)
                  INFO ("smoothing update fields");
                  Filter::Smooth smooth_filter (im1_update_field);
                  smooth_filter.set_stdev (update_smoothing_mm);
                  smooth_filter (im1_update_field, im1_update_field);
                  smooth_filter (im2_update_field, im2_update_field);

                  // TODO Do we only needed if multiple metrics used? Or should we keep this to help make the step size invariant to the magnitude of the input intensities?
                  // Does it make better sense to normalise on the mean update within a max and not the max vector?
                  INFO ("normalising update field");
                  Transform::normalise_displacement (im1_update_field);
                  Transform::normalise_displacement (im2_update_field);

                  INFO ("composing displacement and update fields");
                  Transform::compose_displacement (*im1_disp_field, im1_update_field, *im1_disp_field, grad_step_altered);
                  Transform::compose_displacement (*im2_disp_field, im2_update_field, *im2_disp_field, grad_step_altered);

                  save (*im1_disp_field, std::string("disp_iter" + str(iteration) + ".mif"));

                  INFO ("smoothing displacement fields");
                  smooth_filter.set_stdev (disp_smoothing_mm);
                  smooth_filter (*im1_disp_field, *im1_disp_field);
                  smooth_filter (*im2_disp_field, *im2_disp_field);

                  save (*im1_disp_field, std::string("disp_smoothed_iter" + str(iteration) + ".mif"));

                  CONSOLE ("cost: " + str(global_cost));

//                  Invert fields x 2


                  if (++iteration > max_iter[level])
                    converged = true;


                  //track the energy profile to check for convergence.

                }
             }
          }


          void set_max_iter (const std::vector<int>& maxiter) {
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

          void set_fod_reorientation (const bool do_reorientation) {
            fod_reorientation = do_reorientation;
          }

          void set_aPSF_directions (const Eigen::MatrixXd& dir) {
            aPSF_directions = dir;
          }

          void set_update_smoothing (const default_type voxel_fwhm) {
            update_smoothing = voxel_fwhm;
          }

          void set_disp_smoothing (const default_type voxel_fwhm) {
            disp_smoothing = voxel_fwhm;
          }

//          std::shared_ptr<Image<default_type> > get_im1_disp_field() {
//          }



        protected:

          std::shared_ptr<Image<default_type> > reslice (Image<default_type>& image, Header& header) {
            std::shared_ptr<Image<default_type> > temp = std::make_shared<Image<default_type> > (Image<default_type>::scratch (header));
            Filter::reslice<Interp::Linear> (image, *temp);
            return temp;
          }

          std::vector<int> max_iter;
          std::vector<default_type> scale_factor;
          default_type update_smoothing;
          default_type disp_smoothing;
          default_type gradient_step;
          bool fod_reorientation;
          Eigen::MatrixXd aPSF_directions;

          std::shared_ptr<Image<default_type> > im1_disp_field;
          std::shared_ptr<Image<default_type> > im1_disp_field_inv;
          std::shared_ptr<Image<default_type> > im2_disp_field;
          std::shared_ptr<Image<default_type> > im2_disp_field_inv;

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
