/* Copyright (c) 2008-2021 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __image_registration_metric_local_cross_correlation_h__
#define __image_registration_metric_local_cross_correlation_h__

#include "transform.h"
#include "algo/loop.h"
#include "algo/threaded_loop.h"
#include "adapter/reslice.h"
#include "filter/reslice.h"


namespace MR
{
    namespace Registration
    {
        namespace Metric
        {
            class LocalCrossCorrelation { MEMALIGN(LocalCrossCorrelation)

            protected:

                Eigen::VectorXd mc_weights;
                bool weighted;
                const default_type min_value_threshold;

            public:

                LocalCrossCorrelation ( ) : weighted (false), min_value_threshold (1.e-7) { }

                void set_weights (const Eigen::VectorXd & weights) {
                    mc_weights = weights;
                    weighted = mc_weights.rows() > 0;
                }

                template <class ParamType>
                default_type precompute(ParamType& params) {
                    return 0.0;
                }

                template <class Params>
                default_type operator() (Params& params,
                                         const Eigen::Vector3& im1_point,
                                         const Eigen::Vector3& im2_point,
                                         const Eigen::Vector3& midway_point,
                                         Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {

                    typename Params::Im1ValueType im1_value;
                    typename Params::Im2ValueType im2_value;
                    Eigen::Matrix<typename Params::Im1ValueType, 1, 3> im1_grad;
                    Eigen::Matrix<typename Params::Im2ValueType, 1, 3> im2_grad;

                    params.im1_image_interp->value_and_gradient_wrt_scanner (im1_value, im1_grad);
                    if (std::isnan (default_type (im1_value)))
                        return 0.0;

                    params.im2_image_interp->value_and_gradient_wrt_scanner (im2_value, im2_grad);
                    if (std::isnan (default_type (im2_value)))
                        return 0.0;

                     if (abs(im1_value) < min_value_threshold || abs(im2_value) < min_value_threshold)
                         return 0.0;

                    const vector<size_t> kernel_radius = params.get_radius();

                    default_type local_sf, local_sm, local_sff, local_smm, local_sfm;
                    local_sf = 0; local_sm = 0; local_sff = 0; local_smm = 0; local_sfm = 0;
                    int local_count = 0;


                    transform_type mi_v2s = MR::Transform (params.midway_image).voxel2scanner;
                    transform_type mi_s2v = MR::Transform (params.midway_image).scanner2voxel;
                    Eigen::Vector3 mi_voxel_pos = mi_s2v * midway_point;

                    default_type mi_i1 = mi_voxel_pos(0);
                    default_type mi_i2 = mi_voxel_pos(1);
                    default_type mi_i3 = mi_voxel_pos(2);

                    default_type mi_j1, mi_j2, mi_j3;

                    for (ssize_t e_i1 = -((ssize_t) kernel_radius[0]); e_i1 <= ((ssize_t) kernel_radius[0]); e_i1++) {
                        for (ssize_t e_i2 = -((ssize_t) kernel_radius[1]); e_i2 <= ((ssize_t) kernel_radius[1]); e_i2++) {
                            for (ssize_t e_i3 = -((ssize_t) kernel_radius[2]); e_i3 <= ((ssize_t) kernel_radius[2]); e_i3++) {

                                mi_j1 = mi_i1 + e_i1;
                                mi_j2 = mi_i2 + e_i2;
                                mi_j3 = mi_i3 + e_i3;

                                Eigen::Vector3 mi_voxel_pos_iter ((default_type)(mi_j1), (default_type)(mi_j2), (default_type)(mi_j3));
                                Eigen::Vector3 mi_scanner_pos_iter1 = mi_v2s * mi_voxel_pos_iter;
                                Eigen::Vector3 mi_scanner_pos_iter2 = mi_v2s * mi_voxel_pos_iter;

                                Eigen::Vector3 im1_scanner_pos_iter;
                                Eigen::Vector3 im2_scanner_pos_iter;

                                params.transformation.transform_half (im1_scanner_pos_iter, mi_scanner_pos_iter1);
                                params.im1_image_interp->scanner (im1_scanner_pos_iter);

                                if (params.im1_mask_interp) {
                                    params.im1_mask_interp->scanner (im1_scanner_pos_iter);
                                    if (params.im1_mask_interp->value() < 0.5)
                                      continue;
                                }

                                params.transformation.transform_half_inverse (im2_scanner_pos_iter, mi_scanner_pos_iter2);
                                params.im2_image_interp->scanner (im2_scanner_pos_iter);

                                if (params.im2_mask_interp) {
                                    params.im2_mask_interp->scanner (im2_scanner_pos_iter);
                                    if (params.im2_mask_interp->value() < 0.5)
                                      continue;
                                }


                                    Eigen::Matrix<typename Params::Im1ValueType, 1, 3> im1_grad_iter;
                                    typename Params::Im1ValueType im1_value_iter;
                                    params.im1_image_interp->value_and_gradient_wrt_scanner (im1_value_iter, im1_grad_iter);

                                    Eigen::Matrix<typename Params::Im2ValueType, 1, 3> im2_grad_iter;
                                    typename Params::Im2ValueType im2_value_iter;
                                    params.im2_image_interp->value_and_gradient_wrt_scanner (im2_value_iter, im2_grad_iter);


                                    if (abs(im1_value_iter) > min_value_threshold && abs(im1_value_iter) == abs(im1_value_iter) &&
                                        abs(im2_value_iter) > min_value_threshold && abs(im2_value_iter) == abs(im2_value_iter) ) {
                                        local_sf = local_sf + im1_value_iter;
                                        local_sm = local_sm + im2_value_iter;
                                        local_sff = local_sff + im1_value_iter * im1_value_iter;
                                        local_smm = local_smm + im2_value_iter * im2_value_iter;
                                        local_sfm = local_sfm + im1_value_iter * im2_value_iter;

                                        local_count++;
                                    }

                            }
                        }
                    }


                    if (local_count > 0) {
                        local_sff = local_sff - local_sf * local_sf / local_count;
                        local_smm = local_smm - local_sm * local_sm / local_count;
                        local_sfm = local_sfm - local_sf * local_sm / local_count;

                        if (local_smm < min_value_threshold || local_sff < min_value_threshold) {
                            return 0.0;
                        }

                        Eigen::Vector3d g1 = (im1_value - local_sf / local_count) * (im2_grad);
                        Eigen::Vector3d g2 = (im2_value - local_sm / local_count) * (local_sfm / local_smm) * (im2_grad);

                        const Eigen::Vector3d g = (g1 - g2) * (local_sfm * local_count / ((local_sff * local_smm)));

                        default_type computed_local_cost = -1.0 * local_sfm / sqrt(local_sff * local_smm);

                        const auto jacobian_vec = params.transformation.get_jacobian_vector_wrt_params (midway_point);

                        gradient.segment<4>(0) += g(0) * jacobian_vec;
                        gradient.segment<4>(4) += g(1) * jacobian_vec;
                        gradient.segment<4>(8) += g(2) * jacobian_vec;

                        return computed_local_cost;

                    } else {
                        return 0.0;
                    }

                }

            };




            class LocalCrossCorrelation4D { MEMALIGN(LocalCrossCorrelation4D)

            protected:

                Eigen::VectorXd mc_weights;
                bool weighted;
                const default_type min_value_threshold;
                default_type weight_sum;

            public:

                LocalCrossCorrelation4D ( ) : weighted (false), min_value_threshold (1.e-7), weight_sum (0.0) { }

                void set_weights (const Eigen::VectorXd & weights) {
                    mc_weights = weights;
                    weighted = mc_weights.rows() > 0;

                    if (mc_weights.rows() > 0) {
                        weight_sum = mc_weights.sum();
                    }
                }

                template <class ParamType>
                default_type precompute(ParamType& params) {

                    if (weight_sum < 1) {
                        weight_sum = (default_type) (params.im1_image.size(3));
                    }

                    return 0.0;
                }

                template <class Params>
                default_type operator() (Params& params,
                                         const Eigen::Vector3& im1_point,
                                         const Eigen::Vector3& im2_point,
                                         const Eigen::Vector3& midway_point,
                                         Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {


                    Eigen::Matrix<typename Params::Im1ValueType, Eigen::Dynamic, 1> im1_values;
                    Eigen::Matrix<typename Params::Im2ValueType, Eigen::Dynamic, 1> im2_values;
                    Eigen::Matrix<typename Params::Im1ValueType, Eigen::Dynamic, 3> im1_grad;
                    Eigen::Matrix<typename Params::Im2ValueType, Eigen::Dynamic, 3> im2_grad;

                    Eigen::VectorXd sf_values;
                    Eigen::VectorXd sm_values;
                    Eigen::VectorXd sff_values;
                    Eigen::VectorXd sfm_values;
                    Eigen::VectorXd smm_values;
                    Eigen::VectorXd count_values;

                    const vector<size_t> kernel_radius = params.get_radius();

                    Header im1_header (params.im1_image);
                    Header im2_header (params.im2_image);

                    ssize_t volumes = params.im1_image.size(3);

                    if (im1_values.rows() != volumes) {
                        im1_values.resize (volumes);
                        im1_grad.resize (volumes, 3);
                        im2_values.resize (volumes);
                        im2_grad.resize (volumes, 3);

                        sf_values.resize (volumes);
                        sm_values.resize (volumes);
                        sff_values.resize (volumes);
                        sfm_values.resize (volumes);
                        smm_values.resize (volumes);
                        count_values.resize(volumes);
                    }

                    params.im1_image_interp->value_and_gradient_row_wrt_scanner (im1_values, im1_grad);
                    if (im1_values.hasNaN())
                        return 0.0;

                    params.im2_image_interp->value_and_gradient_row_wrt_scanner (im2_values, im2_grad);
                    if (im2_values.hasNaN())
                        return 0.0;

                    default_type computed_local_cost = 0.0;

                    transform_type mi_v2s = MR::Transform (params.midway_image).voxel2scanner;
                    transform_type mi_s2v = MR::Transform (params.midway_image).scanner2voxel;
                    Eigen::Vector3 mi_voxel_pos = mi_s2v * midway_point;

                    default_type mi_i1 = mi_voxel_pos(0);
                    default_type mi_i2 = mi_voxel_pos(1);
                    default_type mi_i3 = mi_voxel_pos(2);

                    default_type mi_j1, mi_j2, mi_j3;

                    sf_values.setZero();
                    sm_values.setZero();
                    sff_values.setZero();
                    smm_values.setZero();
                    sfm_values.setZero();
                    count_values.setZero();

                    for (ssize_t e_i1 = -((ssize_t) kernel_radius[0]); e_i1 <= ((ssize_t) kernel_radius[0]); e_i1++) {
                        for (ssize_t e_i2 = -((ssize_t) kernel_radius[1]); e_i2 <= ((ssize_t) kernel_radius[1]); e_i2++) {
                            for (ssize_t e_i3 = -((ssize_t) kernel_radius[2]); e_i3 <= ((ssize_t) kernel_radius[2]); e_i3++) {

                                mi_j1 = mi_i1 + e_i1;
                                mi_j2 = mi_i2 + e_i2;
                                mi_j3 = mi_i3 + e_i3;

                                Eigen::Vector3 mi_voxel_pos_iter ((default_type)(mi_j1), (default_type)(mi_j2), (default_type)(mi_j3));
                                Eigen::Vector3 mi_scanner_pos_iter1 = mi_v2s * mi_voxel_pos_iter;
                                Eigen::Vector3 mi_scanner_pos_iter2 = mi_v2s * mi_voxel_pos_iter;

                                Eigen::Vector3 im1_scanner_pos_iter;
                                Eigen::Vector3 im2_scanner_pos_iter;

                                params.transformation.transform_half (im1_scanner_pos_iter, mi_scanner_pos_iter1);
                                params.transformation.transform_half_inverse (im2_scanner_pos_iter, mi_scanner_pos_iter2);

                                params.im1_image_interp->scanner (im1_scanner_pos_iter);
                                params.im2_image_interp->scanner (im2_scanner_pos_iter);

                                bool within_mask = true;

                                if (params.im1_mask_interp) {
                                    params.im1_mask_interp->scanner (im1_scanner_pos_iter);
                                    if (params.im1_mask_interp->value() < 0.5)
                                    within_mask = false;
                                }

                                if (params.im2_mask_interp) {
                                    params.im2_mask_interp->scanner (im2_scanner_pos_iter);
                                    if (params.im2_mask_interp->value() < 0.5)
                                    within_mask = false;
                                }

                                if (within_mask) {

                                    Eigen::Matrix<typename Params::Im1ValueType, Eigen::Dynamic, 3> im1_grad_iter;
                                    Eigen::Matrix<typename Params::Im1ValueType, Eigen::Dynamic, 1> im1_values_iter;
                                    if (im1_values_iter.rows() != volumes) {
                                        im1_values_iter.resize (volumes);
                                        im1_grad_iter.resize (volumes, 3);
                                    }
                                    params.im1_image_interp->value_and_gradient_row_wrt_scanner (im1_values_iter, im1_grad_iter);

                                    Eigen::Matrix<typename Params::Im2ValueType, Eigen::Dynamic, 3> im2_grad_iter;
                                    Eigen::Matrix<typename Params::Im2ValueType, Eigen::Dynamic, 1> im2_values_iter;
                                    if (im2_values_iter.rows() != volumes) {
                                        im2_values_iter.resize (volumes);
                                        im2_grad_iter.resize (volumes, 3);
                                    }
                                    params.im2_image_interp->value_and_gradient_row_wrt_scanner (im2_values_iter, im2_grad_iter);

                                    for (ssize_t i = 0; i < volumes; ++i) {
                                        if (abs(im1_values_iter[i]) > 0 && !std::isnan(im1_values_iter[i]) && abs(im2_values_iter[i]) > 0 && !std::isnan(im2_values_iter[i])) {
                                            sf_values[i] = sf_values[i] + im1_values_iter[i];
                                            sm_values[i] = sm_values[i] + im2_values_iter[i];
                                            sff_values[i] = sff_values[i] + im1_values_iter[i] * im1_values_iter[i];
                                            smm_values[i] = smm_values[i] + im2_values_iter[i] * im2_values_iter[i];
                                            sfm_values[i] = sfm_values[i] + im1_values_iter[i] * im2_values_iter[i];
                                            count_values[i]++;
                                        }
                                    }

                                }
                            }
                        }
                    }


                    for (ssize_t i = 0; i < volumes; ++i) {

                            default_type local_sf, local_sm, local_sff, local_smm, local_sfm;
                            local_sf = 0; local_sm = 0; local_sff = 0; local_smm = 0; local_sfm = 0;
                            int local_count = 0;

                            local_sf = sf_values[i];
                            local_sm = sm_values[i];
                            local_sff = sff_values[i];
                            local_smm = smm_values[i];
                            local_sfm = sfm_values[i];
                            local_count = count_values[i];

                            if (local_count > 1) {

                                local_sff = local_sff - local_sf * local_sf / local_count;
                                local_smm = local_smm - local_sm * local_sm / local_count;
                                local_sfm = local_sfm - local_sf * local_sm / local_count;

                                if (local_smm > min_value_threshold && local_sff > min_value_threshold) {

                                    Eigen::Vector3d g1 = (im1_values[i] - local_sf / local_count) * im2_grad.row(i);
                                    Eigen::Vector3d g2 = (im2_values[i] - local_sm / local_count) * (local_sfm / local_smm) * im2_grad.row(i);

                                    default_type current_volume_weight = 1 / ( (default_type) volumes );
                                    if (this->weighted)
                                        current_volume_weight = ((default_type) this->mc_weights(i)) / ((default_type) this->mc_weights.sum());

                                    const Eigen::Vector3d g = current_volume_weight * (g1 - g2) * (local_sfm * local_count / ((local_sff * local_smm)));

                                    computed_local_cost += -1.0 * local_sfm / sqrt(local_sff * local_smm);

                                    const auto jacobian_vec = params.transformation.get_jacobian_vector_wrt_params (midway_point);

                                    gradient.segment<4>(0) += g(0) * jacobian_vec;
                                    gradient.segment<4>(4) += g(1) * jacobian_vec;
                                    gradient.segment<4>(8) += g(2) * jacobian_vec;

                                }
                            }
                    }

                    return computed_local_cost;

                }

            };

        }
    }
}
#endif

