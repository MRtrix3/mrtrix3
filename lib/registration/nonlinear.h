/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see www.mrtrix.org
 *
 */


#ifndef __registration_nonlinear_h__
#define __registration_nonlinear_h__

#include <vector>
#include "image.h"
#include "filter/warp.h"
#include "filter/resize.h"
#include "registration/transform/reorient.h"
#include "registration/transform/compose.h"
#include "registration/transform/affine.h"
#include "registration/transform/convert.h"
#include "registration/transform/warp_utils.h"
#include "registration/transform/invert.h"
#include "registration/metric/demons.h"
#include "registration/metric/demons4D.h"
#include "registration/multi_resolution_lmax.h"
#include "image/average_space.h"

namespace MR
{
  namespace Registration
  {

    extern const App::OptionGroup nonlinear_options;


    class NonLinear
    {

      public:

        NonLinear ():
          is_initialised (false),
          max_iter (1, 50),
          scale_factor (3),
          update_smoothing (2.0),
          disp_smoothing (1.0),
          gradient_step (0.5),
          do_reorientation (false),
          fod_lmax (3) {
            scale_factor[0] = 0.25;
            scale_factor[1] = 0.5;
            scale_factor[2] = 1.0;
            fod_lmax[0] = 0;
            fod_lmax[1] = 2;
            fod_lmax[2] = 4;
        }

        template <class TransformType, class Im1ImageType, class Im2ImageType, class Im1MaskType, class Im2MaskType>
          void run (TransformType linear_transform,
                    Im1ImageType& im1_image,
                    Im2ImageType& im2_image,
                    Im1MaskType& im1_mask,
                    Im2MaskType& im2_mask) {

            if (!is_initialised) {
              im1_linear = linear_transform.get_transform_half();
              im2_linear = linear_transform.get_transform_half_inverse();

              INFO ("Estimating halfway space");
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
              midway_image_header = compute_minimum_average_header<default_type, Eigen::Transform<default_type, 3, Eigen::Projective>>(headers, 1.0, padding, init_transforms);
            } else {
              // if initialising only perform optimisation at the full resolution level
              scale_factor.resize (1);
              scale_factor[0] = 1.0;
            }

            if (max_iter.size() == 1)
              max_iter.resize (scale_factor.size(), max_iter[0]);
            else if (max_iter.size() != scale_factor.size())
              throw Exception ("the max number of non-linear iterations needs to be defined for each multi-resolution level (scale_factor)");

            if (do_reorientation and (fod_lmax.size() != scale_factor.size()))
              throw Exception ("the lmax needs to be defined for each multi-resolution level (scale factor)");
            else
              fod_lmax.resize (scale_factor.size(), 0);

            for (size_t level = 0; level < scale_factor.size(); level++) {
              if (is_initialised) {
                if (do_reorientation) {
                  CONSOLE ("scale factor (init warp resolution), lmax " + str(fod_lmax[level]));
                } else {
                  CONSOLE ("scale factor (init warp resolution)");
                }
              } else {
                if (do_reorientation) {
                  CONSOLE ("multi-resolution level " + str(level + 1) + ", scale factor " + str(scale_factor[level]) + ", lmax " + str(fod_lmax[level]));
                } else {
                  CONSOLE ("multi-resolution level " + str(level + 1) + ", scale factor " + str(scale_factor[level]));
                }
              }

              DEBUG ("Resizing midway image based on multi-resolution level");

              Filter::Resize resize_filter (midway_image_header);
              resize_filter.set_scale_factor (scale_factor[level]);
              resize_filter.set_interp_type (1);
              resize_filter.datatype() = DataType::Float64; // for saving debug output with save()

              Header midway_image_header_resized = resize_filter;
              midway_image_header_resized.set_ndim(3);

              default_type update_smoothing_mm = update_smoothing * ((midway_image_header_resized.spacing(0)
                                                                    + midway_image_header_resized.spacing(1)
                                                                    + midway_image_header_resized.spacing(2)) / 3.0);
              default_type disp_smoothing_mm = disp_smoothing * ((midway_image_header_resized.spacing(0)
                                                                + midway_image_header_resized.spacing(1)
                                                                + midway_image_header_resized.spacing(2)) / 3.0);

              auto im1_smoothed = Registration::multi_resolution_lmax (im1_image, scale_factor[level], do_reorientation, fod_lmax[level]);
              auto im2_smoothed = Registration::multi_resolution_lmax (im2_image, scale_factor[level], do_reorientation, fod_lmax[level]);

              DEBUG ("Initialising scratch images");
              Header warped_header (midway_image_header_resized);
              if (im1_image.ndim() == 4) {
                warped_header.set_ndim(4);
                warped_header.size(3) = im1_smoothed.size(3);
              }
              auto im1_warped = Image<default_type>::scratch (warped_header);
              auto im2_warped = Image<default_type>::scratch (warped_header);

              Header field_header (midway_image_header_resized);
              field_header.set_ndim(4);
              field_header.size(3) = 3;

              im1_disp_field_new = std::make_shared<Image<default_type>>(Image<default_type>::scratch (field_header));
              im2_disp_field_new = std::make_shared<Image<default_type>>(Image<default_type>::scratch (field_header));
              im1_update_field = std::make_shared<Image<default_type>>(Image<default_type>::scratch (field_header));
              im2_update_field = std::make_shared<Image<default_type>>(Image<default_type>::scratch (field_header));
              im1_update_field_new = std::make_shared<Image<default_type>>(Image<default_type>::scratch (field_header));
              im2_update_field_new = std::make_shared<Image<default_type>>(Image<default_type>::scratch (field_header));

              if (!is_initialised) {
                if (level == 0) {
                  im1_disp_field = std::make_shared<Image<default_type>>(Image<default_type>::scratch (field_header));
                  im2_disp_field = std::make_shared<Image<default_type>>(Image<default_type>::scratch (field_header));
                  im1_deform_field_inv = std::make_shared<Image<default_type>>(Image<default_type>::scratch (field_header));
                  im2_deform_field_inv = std::make_shared<Image<default_type>>(Image<default_type>::scratch (field_header));
                  Transform::displacement2deformation (*im1_deform_field_inv, *im1_deform_field_inv);  // init to identity
                  Transform::displacement2deformation (*im2_deform_field_inv, *im2_deform_field_inv);
                } else {
                  DEBUG ("Upsampling fields");
                  {
                    LogLevelLatch level(0);
                    im1_disp_field = reslice (*im1_disp_field, field_header);
                    im2_disp_field = reslice (*im2_disp_field, field_header);
                    im1_deform_field_inv = reslice (*im1_deform_field_inv, field_header);
                    im2_deform_field_inv = reslice (*im2_deform_field_inv, field_header);
                  }
                }
              }

              ssize_t iteration = 1;
              default_type grad_step_altered = gradient_step * (field_header.spacing(0) + field_header.spacing(1) + field_header.spacing(2)) / 3.0;
              default_type cost = std::numeric_limits<default_type>::max();
              bool converged = false;

              while (!converged) {
                if (iteration > 1) {
                  DEBUG ("smoothing update fields");
                  Filter::Smooth smooth_filter (*im1_update_field);
                  smooth_filter.set_stdev (update_smoothing_mm);
                  smooth_filter (*im1_update_field, *im1_update_field);
                  smooth_filter (*im2_update_field, *im2_update_field);
                }

                Image<default_type> im1_deform_field = Image<default_type>::scratch (field_header);
                Image<default_type> im2_deform_field = Image<default_type>::scratch (field_header);

                if (iteration > 1) {
                  DEBUG ("updating displacement field field");
                  Transform::compose_displacement (*im1_disp_field, *im1_update_field, *im1_disp_field_new, grad_step_altered);
                  Transform::compose_displacement (*im2_disp_field, *im2_update_field, *im2_disp_field_new, grad_step_altered);

                  DEBUG ("smoothing displacement field");
                  Filter::Smooth smooth_filter (*im1_disp_field_new);
                  smooth_filter.set_stdev (disp_smoothing_mm);
                  smooth_filter (*im1_disp_field_new, *im1_disp_field_new);
                  smooth_filter (*im2_disp_field_new, *im2_disp_field_new);

                  Registration::Transform::compose_linear_displacement (im1_linear, *im1_disp_field_new, im1_deform_field);
                  Registration::Transform::compose_linear_displacement (im2_linear, *im2_disp_field_new, im2_deform_field);
                } else {
                  Registration::Transform::compose_linear_displacement (im1_linear, *im1_disp_field, im1_deform_field);
                  Registration::Transform::compose_linear_displacement (im2_linear, *im2_disp_field, im2_deform_field);
                }

                DEBUG ("warping input images");
                {
                  LogLevelLatch level (0);
                  Filter::warp<Interp::Linear> (im1_smoothed, im1_warped, im1_deform_field, 0.0);
                  Filter::warp<Interp::Linear> (im2_smoothed, im2_warped, im2_deform_field, 0.0);
                }

                if (do_reorientation && fod_lmax[level]) {
                  DEBUG ("Reorienting FODs");
                  Registration::Transform::reorient_warp (im1_warped, im1_deform_field, aPSF_directions);
                  Registration::Transform::reorient_warp (im2_warped, im2_deform_field, aPSF_directions);
                }

//                save (im1_warped, "im1_warped_level" + str(level) + "_iter" + str(iteration) + ".mif");

                DEBUG ("warping mask images");
                Im1MaskType im1_mask_warped;
                if (im1_mask.valid()) {
                  im1_mask_warped = Im1MaskType::scratch (midway_image_header_resized);
                  LogLevelLatch level (0);
                  Filter::warp<Interp::Linear> (im1_mask, im1_mask_warped, im1_deform_field, 0.0);
                }
                Im1MaskType im2_mask_warped;
                if (im2_mask.valid()) {
                  im2_mask_warped = Im1MaskType::scratch (midway_image_header_resized);
                  LogLevelLatch level (0);
                  Filter::warp<Interp::Linear> (im2_mask, im2_mask_warped, im2_deform_field, 0.0);
                }

                DEBUG ("evaluating metric and computing update field");
                default_type cost_new = 0.0;
                size_t voxel_count = 0;

                if (im1_image.ndim() == 4) {
                  TRACE;
                  Metric::Demons4D<Im1ImageType, Im2ImageType, Im1MaskType, Im2MaskType> metric (cost_new, voxel_count, im1_warped, im2_warped, im1_mask_warped, im2_mask_warped);
                  ThreadedLoop (im1_warped, 0, 3).run (metric, im1_warped, im2_warped, *im1_update_field_new, *im2_update_field_new);
                } else {
                  Metric::Demons<Im1ImageType, Im2ImageType, Im1MaskType, Im2MaskType> metric (cost_new, voxel_count, im1_warped, im2_warped, im1_mask_warped, im2_mask_warped);
                  ThreadedLoop (im1_warped, 0, 3).run (metric, im1_warped, im2_warped, *im1_update_field_new, *im2_update_field_new);
                }

                cost_new /= static_cast<default_type>(voxel_count);

                // If cost is lower then keep new displacement fields and gradients
                if (cost_new < cost) {
                  cost = cost_new;
                  if (iteration > 1) {
                    std::swap (im1_disp_field_new, im1_disp_field);
                    std::swap (im2_disp_field_new, im2_disp_field);
                  }
                  std::swap (im1_update_field_new, im1_update_field);
                  std::swap (im2_update_field_new, im2_update_field);

                  // drag the inverse along for the ride
                  DEBUG ("inverting displacement field");
                  {
                    LogLevelLatch level (0);
                    Transform::invert_displacement_deformation (*im1_disp_field, *im1_deform_field_inv, true);
                    Transform::invert_displacement_deformation (*im2_disp_field, *im2_deform_field_inv, true);
                  }
                } else {
                  converged = true;
                }

                if (!converged)
                  INFO ("  iteration: " + str(iteration) + " cost: " + str(cost));

                if (++iteration > max_iter[level])
                  converged = true;
             }
             Transform::deformation2displacement (*im1_deform_field_inv, *im1_deform_field_inv);
             Transform::deformation2displacement (*im2_deform_field_inv, *im2_deform_field_inv); //convert to displacement field for output
           }

          }

          template <class InputWarpType>
          void initialise (InputWarpType& input_warps) {
            assert (input_warps.ndim() == 5);

            DEBUG ("reading linear transform from init warp field header");
            im1_linear = Registration::Transform::parse_linear_transform (input_warps, "linear1");
            im2_linear = Registration::Transform::parse_linear_transform (input_warps, "linear2");

            DEBUG ("loading initial warp fields");
            midway_image_header = input_warps;
            midway_image_header.set_ndim (3);
            Header field_header (input_warps);
            field_header.set_ndim (4);
            field_header.size(3) = 3;

            im1_disp_field = std::make_shared<Image<default_type>> (Image<default_type>::scratch (field_header));
            input_warps.index(4) = 0;
            threaded_copy (input_warps, *im1_disp_field, 0, 4);

            im1_deform_field_inv = std::make_shared<Image<default_type>> (Image<default_type>::scratch (field_header));
            input_warps.index(4) = 1;
            threaded_copy (input_warps, *im1_deform_field_inv, 0, 4);
            Transform::displacement2deformation (*im1_deform_field_inv, *im1_deform_field_inv);

            im2_disp_field = std::make_shared<Image<default_type>> (Image<default_type>::scratch (field_header));
            input_warps.index(4) = 2;
            threaded_copy (input_warps, *im2_disp_field, 0, 4);

            im2_deform_field_inv = std::make_shared<Image<default_type>> (Image<default_type>::scratch (field_header));
            input_warps.index(4) = 3;
            threaded_copy (input_warps, *im2_deform_field_inv, 0, 4);
            Transform::displacement2deformation (*im2_deform_field_inv, *im2_deform_field_inv);
            is_initialised = true;
          }


          void set_max_iter (const std::vector<int>& maxiter) {
            for (size_t i = 0; i < maxiter.size (); ++i)
              if (maxiter[i] < 0)
                throw Exception ("the number of iterations must be positive");
            max_iter = maxiter;
          }


          void set_scale_factor (const std::vector<default_type>& scalefactor) {
            for (size_t level = 0; level < scalefactor.size(); ++level) {
              if (scalefactor[level] <= 0 || scalefactor[level] > 1)
                throw Exception ("the non-linear registration scale factor for each multi-resolution level must be between 0 and 1");
            }
            scale_factor = scalefactor;
          }

          std::vector<default_type> get_scale_factor () const {
            return scale_factor;
          }

          void set_init_grad_step (const default_type step) {
            gradient_step = step;
          }

          void set_aPSF_directions (const Eigen::MatrixXd& dir) {
            aPSF_directions = dir;
            do_reorientation = true;
          }

          void set_update_smoothing (const default_type voxel_fwhm) {
            update_smoothing = voxel_fwhm;
          }

          void set_disp_smoothing (const default_type voxel_fwhm) {
            disp_smoothing = voxel_fwhm;
          }

          void set_lmax (const std::vector<int>& lmax) {
            for (size_t i = 0; i < lmax.size (); ++i)
              if (lmax[i] < 0 || lmax[i] % 2)
                throw Exception ("the input nonlinear lmax must be positive and even");
            fod_lmax = lmax;
          }

          //TODO
          std::shared_ptr<Image<default_type> > get_im1_disp_field() {
            return im1_disp_field;
          }

          std::shared_ptr<Image<default_type> > get_im2_disp_field() {
            return im2_disp_field;
          }

          std::shared_ptr<Image<default_type> > get_im1_disp_field_inv() {
            return im1_deform_field_inv;
          }

          std::shared_ptr<Image<default_type> > get_im2_disp_field_inv() {
            return im2_deform_field_inv;
          }

          transform_type get_im1_linear() const {
            return im1_linear;
          }

          transform_type get_im2_linear() const {
            return im2_linear;
          }

          Header get_output_warps_header () {
            Header output_header (*im1_disp_field);
            output_header.set_ndim (5);
            output_header.size(3) = 3;
            output_header.size(4) = 4;
            output_header.stride(0) = 1;
            output_header.stride(1) = 2;
            output_header.stride(2) = 3;
            output_header.stride(3) = 4;
            output_header.stride(4) = 5;

            output_header.keyval()["linear1"] = str(im1_linear.matrix());
            output_header.keyval()["linear2"] = str(im2_linear.matrix());

            output_header.keyval()["scale_factors"] = str(scale_factor);
            output_header.keyval()["max_iterations"] = str(max_iter);
            output_header.keyval()["update_smooth"] = str(update_smoothing);
            output_header.keyval()["displacement_smooth"] = str(disp_smoothing);
            output_header.keyval()["reorientation"] = str(do_reorientation);
            output_header.keyval()["gradient_step"] = str(gradient_step);
            if (do_reorientation)
              output_header.keyval()["fod"] = str(fod_lmax);

            return output_header;
          }

          template <class OutputType>
          void get_output_warps (OutputType& output_warps) {
            assert (output_warps.ndim() == 5);
            output_warps.index(4) = 0;
            threaded_copy (*im1_disp_field, output_warps, 0, 4);
            output_warps.index(4) = 1;
            threaded_copy (*im1_deform_field_inv, output_warps, 0, 4);
            output_warps.index(4) = 2;
            threaded_copy (*im2_disp_field, output_warps, 0, 4);
            output_warps.index(4) = 3;
            threaded_copy (*im2_deform_field_inv, output_warps, 0, 4);
          }

          Header get_midway_header () {
            return midway_image_header;
          }



        protected:

          std::shared_ptr<Image<default_type> > reslice (Image<default_type>& image, Header& header) {
            std::shared_ptr<Image<default_type> > temp = std::make_shared<Image<default_type> > (Image<default_type>::scratch (header));
            Filter::reslice<Interp::Linear> (image, *temp);
            return temp;
          }

          bool is_initialised;
          std::vector<int> max_iter;
          std::vector<default_type> scale_factor;
          default_type update_smoothing;
          default_type disp_smoothing;
          default_type gradient_step;
          Eigen::MatrixXd aPSF_directions;
          bool do_reorientation;
          std::vector<int> fod_lmax;

          transform_type im1_linear;
          transform_type im2_linear;
          Header midway_image_header;

          std::shared_ptr<Image<default_type> > im1_disp_field_new;
          std::shared_ptr<Image<default_type> > im2_disp_field_new;
          std::shared_ptr<Image<default_type> > im1_disp_field;  // Internally the warp is a displacement field for smoothing near the boundaries
          std::shared_ptr<Image<default_type> > im2_disp_field;
          std::shared_ptr<Image<default_type> > im1_deform_field_inv;
          std::shared_ptr<Image<default_type> > im2_deform_field_inv; // Note we store the inverses as deformations since the inverter works on deformations

          std::shared_ptr<Image<default_type> > im1_update_field;
          std::shared_ptr<Image<default_type> > im2_update_field;
          std::shared_ptr<Image<default_type> > im1_update_field_new;
          std::shared_ptr<Image<default_type> > im2_update_field_new;

    };
  }
}

#endif
