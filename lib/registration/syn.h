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
#include "registration/transform/norm.h"
#include "registration/transform/invert.h"
#include "registration/metric/syn_demons.h"
#include "image/average_space.h"

namespace MR
{
  namespace Registration
  {

    extern const App::OptionGroup syn_options;


    class SyN
    {

      public:

        SyN ():
          is_initialised (false),
          max_iter (1, 50),
          scale_factor (3),
          update_smoothing (2.0),
          disp_smoothing (1.0),
          gradient_step (1.0),
          fod_reorientation (false) {
            scale_factor[0] = 0.25;
            scale_factor[1] = 0.5;
            scale_factor[2] = 1.0;
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
              midway_image_header.set_ndim (im1_image.ndim());
              if (im1_image.ndim() > 3)
                midway_image_header.size(3) = im1_image.size(3);
            }

            if (max_iter.size() == 1)
              max_iter.resize (scale_factor.size(), max_iter[0]);
            else if (max_iter.size() != scale_factor.size())
              throw Exception ("the max number of SyN iterations needs to be defined for each multi-resolution level");

            for (size_t level = 0; level < scale_factor.size(); level++) {
                CONSOLE ("SyN: multi-resolution level " + str(level + 1) + ", scale factor: " + str(scale_factor[level]));

                DEBUG ("Resizing midway image based on multi-resolution level");

                Filter::Resize resize_filter (midway_image_header);
                resize_filter.set_scale_factor (scale_factor[level]);
                resize_filter.set_interp_type (1);
                resize_filter.datatype() = DataType::Float64; // for saving debug output
                Header midway_image_header_resized = resize_filter;

                default_type update_smoothing_mm = update_smoothing * ((midway_image_header_resized.spacing(0)
                                                                      + midway_image_header_resized.spacing(1)
                                                                      + midway_image_header_resized.spacing(2)) / 3.0);
                default_type disp_smoothing_mm = disp_smoothing * ((midway_image_header_resized.spacing(0)
                                                                  + midway_image_header_resized.spacing(1)
                                                                  + midway_image_header_resized.spacing(2)) / 3.0);

                DEBUG ("Smoothing imput images based on multi-resolution pyramid");
                Filter::Smooth im1_smooth_filter (im1_image);
                im1_smooth_filter.set_stdev (1.0 / (2.0 * scale_factor[level]));
                auto im1_smoothed = Image<default_type>::scratch (im1_smooth_filter);

                Filter::Smooth im2_smooth_filter (im2_image);
                im2_smooth_filter.set_stdev (1.0 / (2.0 * scale_factor[level]));  // TODO compare this with SyNFOD line 489
                auto im2_smoothed = Image<default_type>::scratch (im2_smooth_filter);

                {
                  LogLevelLatch log_level (0);
                  im1_smooth_filter (im1_image, im1_smoothed);
                  im2_smooth_filter (im2_image, im2_smoothed);
                }

                DEBUG ("Initialising scratch images");
                Header warped_header (midway_image_header_resized);
                if (im1_image.ndim() == 4) {
                  warped_header.set_ndim(4);
                  warped_header.size(3) = im1_image.size(3);
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
                  save (im1_warped, std::string("im1_warped_level_" + str(level+1) + "_iter" + str(iteration) + ".mif"), false);
                  save (im2_warped, std::string("im2_warped_level_" + str(level+1) + "_iter" + str(iteration) + ".mif"), false);

                  if (fod_reorientation) {
                    DEBUG ("Reorienting FODs");
                    Registration::Transform::reorient_warp (im1_warped, im1_deform_field, aPSF_directions);
                    Registration::Transform::reorient_warp (im2_warped, im2_deform_field, aPSF_directions);
                  }

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
                  Metric::SyNDemons<Im1ImageType, Im2ImageType, Im1MaskType, Im2MaskType> syn_metric (cost_new, voxel_count, im1_warped, im2_warped, im1_mask_warped, im2_mask_warped);
                  ThreadedLoop (im1_warped, 0, 3).run (syn_metric, im1_warped, im2_warped, *im1_update_field_new, *im2_update_field_new);

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

                  // check displacement field difference to detect convergence.
                  std::cerr << "\r  iteration: " + str(iteration) + " cost: " + str(cost) << std::flush;

                  if (++iteration > max_iter[level])
                      converged = true;
               }
               Transform::deformation2displacement (*im1_deform_field_inv, *im1_deform_field_inv);
               Transform::deformation2displacement (*im2_deform_field_inv, *im2_deform_field_inv); //convert to displacement field for output
               std::cerr << std::endl;
             }

          }

          template <class InputWarpType>
          void initialise (InputWarpType& input_warps) {
            assert (input_warps.ndim() == 5);

            DEBUG ("reading linear transform from init warp field header");
            parse_linear_transform (input_warps, im1_linear, "linear1");
            parse_linear_transform (input_warps, im2_linear, "linear2");

            DEBUG ("loading initial warp fields");
            midway_image_header = input_warps;
            Header field_header (input_warps);
            field_header.set_ndim (4);
            field_header.size(3) = 3;

            im1_disp_field = std::make_shared<Image<default_type>>(Image<default_type>::scratch (field_header));
            im2_disp_field = std::make_shared<Image<default_type>>(Image<default_type>::scratch (field_header));
            im1_deform_field_inv = std::make_shared<Image<default_type>>(Image<default_type>::scratch (field_header));
            im2_deform_field_inv = std::make_shared<Image<default_type>>(Image<default_type>::scratch (field_header));

            input_warps.index(4) = 0;
            threaded_copy (input_warps, *im1_disp_field, 0, 4);
            input_warps.index(4) = 1;
            threaded_copy (input_warps, *im1_deform_field_inv, 0, 4);

            input_warps.index(4) = 2;
            threaded_copy (input_warps, *im2_disp_field, 0, 4);
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
            for (size_t i = 0; i < scalefactor.size(); ++i)
              if (scalefactor[i] < 0)
                throw Exception ("the multi-resolution scale factor must be positive");
            scale_factor = scalefactor;
          }

          std::vector<default_type> get_scale_factor () const {
            return scale_factor;
          }

          void set_init_grad_step (const default_type step) {
            gradient_step = step;
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
            return im2_deform_field_inv; // TODO
          }

          transform_type get_im1_linear() const {
            return im1_linear;
          }

          transform_type get_im2_linear() const {
            return im2_linear; // TODO
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

            //TODO add syn parameters to output comments
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

          template <class InputWarpType>
          void parse_linear_transform (InputWarpType& input_warps, transform_type& linear, std::string name) {
            const auto it = input_warps.original_header().keyval().find (name);
            if (it != input_warps.original_header().keyval().end()) {
              const auto lines = split_lines (it->second);
              if (lines.size() != 3)
                throw Exception ("linear transform in initialisation syn warps image headerdoes not contain 3 rows");
              for (size_t row = 0; row < 3; ++row) {
                const auto values = MR::split (lines[row], " ");
                if (values.size() != 4)
                  throw Exception ("linear transform in initialisation syn warps image header does not contain 4 rows");
                for (size_t col = 0; col < 4; ++col)
                  linear (row, col) = std::stod (values[col]);
              }
            } else {
              throw Exception ("no linear transform found in initialisation syn warps image header");
            }
          }

          bool is_initialised;
          std::vector<int> max_iter;
          std::vector<default_type> scale_factor;
          default_type update_smoothing;
          default_type disp_smoothing;
          default_type gradient_step;
          bool fod_reorientation;
          Eigen::MatrixXd aPSF_directions;

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
