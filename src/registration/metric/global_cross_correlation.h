/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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

#ifndef __image_registration_metric_global_cross_correlation_h__
#define __image_registration_metric_global_cross_correlation_h__

#include "transform.h"
#include "algo/loop.h"
#include "algo/threaded_loop.h"
#include "image_helpers.h"
#include "transform.h"

#include "registration/linear.h"


namespace MR
{
    namespace Registration
    {
        namespace Metric
        {

            template <class ParamType>
            class GNCCPrecomputeFunctorMasked_Naive{ MEMALIGN(GNCCPrecomputeFunctorMasked_Naive)

            protected:

                std::shared_ptr<std::mutex> mutex;
                ParamType params;

                default_type& global_count;
                default_type& global_sf;
                default_type& global_sm;
                default_type& global_sff;
                default_type& global_smm;
                default_type& global_sfm;
                default_type& global_gncc;
                default_type local_sf;
                default_type local_sm;
                default_type local_sff;
                default_type local_smm;
                default_type local_sfm;
                default_type local_count;
                transform_type voxel2scanner;

            public:

                GNCCPrecomputeFunctorMasked_Naive (const ParamType& parameters, default_type& output_count, default_type& output_sf, default_type& output_sm,
                    default_type& output_sff, default_type& output_smm, default_type& output_sfm, default_type &output_gncc) :
                mutex (new std::mutex),
                params (parameters),
                global_count (output_count),
                global_sf(output_sf),
                global_sm(output_sm),
                global_sff(output_sff),
                global_smm(output_smm),
                global_sfm (output_sfm),
                global_gncc(output_gncc),
                local_sf(0.0),
                local_sm(0.0),
                local_sff(0.0),
                local_smm(0.0),
                local_sfm(0.0),
                local_count(0.0),
                voxel2scanner (MR::Transform(params.midway_image).voxel2scanner) {

                    if (params.robust_estimate_subset) {
                        assert (params.robust_estimate_subset_from.size() == 3);
                        transform_type i2s (params.midway_image.transform());
                        auto spacing = Eigen::DiagonalMatrix<default_type, 3> (params.midway_image.spacing(0), params.midway_image.spacing(1), params.midway_image.spacing(2));
                        for (size_t j = 0; j < 3; ++j)
                            for (size_t i = 0; i < 3; ++i)
                                i2s(i,3) += params.robust_estimate_subset_from[j] * params.midway_image.spacing(j) * i2s(i,j);
                        voxel2scanner = i2s * spacing;
                    }
                }

                ~GNCCPrecomputeFunctorMasked_Naive () {

                    std::lock_guard<std::mutex> lock (*mutex);

                    if (local_count > 0) {
                        global_count += local_count;
                        global_sf += local_sf;
                        global_sm += local_sm;
                        global_sff += local_sff;
                        global_smm += local_smm;
                        global_sfm += local_sfm;
                    }

                    if (global_count > 0) {
                        default_type sff = global_sff - (global_sf * global_sf / global_count);
                        default_type smm = global_smm - (global_sm * global_sm / global_count);
                        default_type sfm = global_sfm - (global_sf * global_sm / global_count);
                        global_gncc = -1.0 * sfm / sqrt(sff * smm);

                    }

                }

                void operator() (const Iterator& iter) {

                    Eigen::Vector3 voxel_pos ((default_type)iter.index(0), (default_type)iter.index(1), (default_type)iter.index(2));
                    Eigen::Vector3 midway_point = voxel2scanner * voxel_pos;

                    Eigen::Vector3 im2_point;
                    params.transformation.transform_half_inverse (im2_point, midway_point);

                    Eigen::Vector3 im1_point;
                    params.transformation.transform_half (im1_point, midway_point);

                    params.im1_image_interp->scanner (im1_point);
                    if (!(*params.im1_image_interp))
                        return;

                    params.im2_image_interp->scanner (im2_point);
                    if (!(*params.im2_image_interp))
                        return;

                    if (params.im2_mask_interp) {
                        params.im2_mask_interp->scanner (im2_point);
                        if (params.im2_mask_interp->value() < 0.5)
                            return;
                    }

                    if (params.im1_mask_interp) {
                        params.im1_mask_interp->scanner (im1_point);
                        if (params.im1_mask_interp->value() < 0.5)
                            return;
                    }

                    typename ParamType::Im1ValueType im1_value;
                    typename ParamType::Im2ValueType im2_value;

                    Eigen::Matrix<typename ParamType::Im1ValueType, 1, 3> grad1;
                    Eigen::Matrix<typename ParamType::Im2ValueType, 1, 3> grad2;

                    params.im1_image_interp->value_and_gradient_wrt_scanner(im1_value, grad1);
                    params.im2_image_interp->value_and_gradient_wrt_scanner(im2_value, grad2);

                    if (std::isnan (default_type (im1_value)))
                        return;

                    if (std::isnan (default_type (im2_value)))
                        return;

                    if (abs(im1_value) < 1.e-7 || abs(im2_value) < 1.e-7)
                        return;

                    local_sf += im1_value;
                    local_sm += im2_value;

                    local_sff += im1_value * im1_value;
                    local_smm += im2_value * im2_value;
                    local_sfm += im1_value * im2_value;

                    local_count++;

                    return;

                }


            };


            class GlobalCrossCorrelation { MEMALIGN(GlobalCrossCorrelation)

            private:

                Eigen::VectorXd mc_weights;
                bool weighted;
                const default_type min_value_threshold;

                default_type computed_global_cost;
                default_type computed_sf;
                default_type computed_sm;
                default_type computed_sff;
                default_type computed_smm;
                default_type computed_sfm;
                default_type computed_gncc;
                default_type computed_total_count;


            public:

                /** requires_precompute:
                type_trait to distinguish metric types that require a call to precompute before the operator() is called */
                using requires_precompute = int;

                GlobalCrossCorrelation ( ) : weighted (false), min_value_threshold (1.e-7) {}

                void set_weights (Eigen::VectorXd weights) {
                    mc_weights = weights;
                    weighted = mc_weights.rows() > 0;
                }


                template <class ParamType>
                default_type precompute(ParamType& params) {

                    default_type gncc = 0;
                    default_type total_count = 0;
                    default_type sf = 0;
                    default_type sm = 0;
                    default_type sff = 0;
                    default_type smm = 0;
                    default_type sfm = 0;

                    GNCCPrecomputeFunctorMasked_Naive<ParamType> compute_gncc (params, total_count, sf, sm, sff, smm, sfm, gncc);
                    ThreadedLoop (params.midway_image, 0, 3).run (compute_gncc);

                    if (total_count < 1) {
                        computed_sf = 0;
                        computed_sm = 0;
                        computed_gncc = 0;
                        computed_total_count = 0;
                        computed_sff = 0;
                        computed_smm = 0;
                        computed_sfm = 0;
                        computed_global_cost = 0;
                        return 0.0;
                    }

                    computed_sf = sf;
                    computed_sm = sm;
                    computed_gncc = gncc;
                    computed_total_count = total_count;
                    computed_sff = sff - sf * sf / total_count;
                    computed_smm = smm - sm * sm / total_count;
                    computed_sfm = sfm - sf * sm / total_count;
                    computed_global_cost = gncc;

                    return 0.0;

                }
                
                default_type get_gncc() {
                    return computed_gncc;
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

                    if (std::isnan (default_type (computed_global_cost)))
                        return 0.0;

                    if (params.im2_mask_interp) {
                        params.im2_mask_interp->scanner (im2_point);
                        if (params.im2_mask_interp->value() < 0.5)
                            return 0.0;
                    }

                    if (params.im1_mask_interp) {
                        params.im1_mask_interp->scanner (im1_point);
                        if (params.im1_mask_interp->value() < 0.5)
                            return 0.0;
                    }

                    const auto jacobian_vec = params.transformation.get_jacobian_vector_wrt_params (midway_point);

                    if (computed_total_count < 1 || (computed_smm * computed_sff) < min_value_threshold)
                        return 0.0;

                    if (abs(im1_value) < min_value_threshold || abs(im2_value) < min_value_threshold)
                        return 0.0;

                    Eigen::Vector3d g1 = (im1_value - computed_sf / computed_total_count) * im2_grad;
                    Eigen::Vector3d g2 = (im2_value - computed_sm / computed_total_count) * (computed_sfm / computed_smm) * im2_grad;

                    const Eigen::Vector3d g = (g1 - g2) * (1.0 / ((computed_sff * computed_smm)));

                    gradient.segment<4>(0) += g(0) * jacobian_vec;
                    gradient.segment<4>(4) += g(1) * jacobian_vec;
                    gradient.segment<4>(8) += g(2) * jacobian_vec;

                    return computed_global_cost;

                }

            };



            template <class ParamType>
            class GNCCPrecomputeFunctorMasked4D_Naive{ MEMALIGN(GNCCPrecomputeFunctorMasked4D_Naive)

            protected:

                std::shared_ptr<std::mutex> mutex;
                ParamType params;
                ssize_t volumes;
                transform_type voxel2scanner;

                Eigen::VectorXd& global_count;
                Eigen::VectorXd& global_sf;
                Eigen::VectorXd& global_sm;
                Eigen::VectorXd& global_sff;
                Eigen::VectorXd& global_smm;
                Eigen::VectorXd& global_sfm;
                Eigen::VectorXd& global_gncc;
                Eigen::VectorXd local_sf;
                Eigen::VectorXd local_sm;
                Eigen::VectorXd local_sff;
                Eigen::VectorXd local_smm;
                Eigen::VectorXd local_sfm;
                Eigen::VectorXd local_count;

            public:

                GNCCPrecomputeFunctorMasked4D_Naive (const ParamType& parameters, Eigen::VectorXd& output_count, Eigen::VectorXd& output_sf, Eigen::VectorXd& output_sm,
                    Eigen::VectorXd& output_sff, Eigen::VectorXd& output_smm, Eigen::VectorXd& output_sfm, Eigen::VectorXd &output_gncc) :
                mutex (new std::mutex),
                params (parameters),
                volumes(parameters.im1_image.size(3)),
                voxel2scanner (MR::Transform(params.midway_image).voxel2scanner),
                global_count (output_count),
                global_sf(output_sf),
                global_sm(output_sm),
                global_sff(output_sff),
                global_smm(output_smm),
                global_sfm (output_sfm),
                global_gncc(output_gncc) {

                    local_sf.setZero (volumes);
                    local_sm.setZero (volumes);
                    local_sff.setZero (volumes);
                    local_smm.setZero (volumes);
                    local_sfm.setZero (volumes);
                    local_count.setZero (volumes);

                    if (params.robust_estimate_subset) {
                        assert (params.robust_estimate_subset_from.size() == 3);
                        transform_type i2s (params.midway_image.transform());
                        auto spacing = Eigen::DiagonalMatrix<default_type, 3> (params.midway_image.spacing(0), params.midway_image.spacing(1), params.midway_image.spacing(2));
                        for (size_t j = 0; j < 3; ++j)
                            for (size_t i = 0; i < 3; ++i)
                                i2s(i,3) += params.robust_estimate_subset_from[j] * params.midway_image.spacing(j) * i2s(i,j);
                        voxel2scanner = i2s * spacing;
                    }
                }

                ~GNCCPrecomputeFunctorMasked4D_Naive () {

                    std::lock_guard<std::mutex> lock (*mutex);

                    global_count += local_count;
                    global_sf += local_sf;
                    global_sm += local_sm;
                    global_sff += local_sff;
                    global_smm += local_smm;
                    global_sfm += local_sfm;

                }

                void operator() (const Iterator& iter) {

                    Eigen::Vector3 voxel_pos ((default_type)iter.index(0), (default_type)iter.index(1), (default_type)iter.index(2));
                    Eigen::Vector3 midway_point = voxel2scanner * voxel_pos;

                    Eigen::Vector3 im1_point;
                    params.transformation.transform_half (im1_point, midway_point);

                    Eigen::Vector3 im2_point;
                    params.transformation.transform_half_inverse (im2_point, midway_point);

                    params.im1_image_interp->scanner (im1_point);
                    if (!(*params.im1_image_interp))
                        return;

                    params.im2_image_interp->scanner (im2_point);
                    if (!(*params.im2_image_interp))
                        return;

                    if (params.im2_mask_interp) {
                        params.im2_mask_interp->scanner (im2_point);
                        if (params.im2_mask_interp->value() < 0.5)
                        return;
                    }

                    if (params.im1_mask_interp) {
                        params.im1_mask_interp->scanner (im1_point);
                        if (params.im1_mask_interp->value() < 0.5)
                        return;
                    }

                    Eigen::Matrix<typename ParamType::Im1ValueType, Eigen::Dynamic, 3> im1_grad;
                    Eigen::Matrix<typename ParamType::Im1ValueType, Eigen::Dynamic, 1> im1_values;
                    if (im1_values.rows() != volumes) {
                        im1_values.resize (volumes);
                        im1_grad.resize (volumes, 3);
                    }

                    Eigen::Matrix<typename ParamType::Im2ValueType, Eigen::Dynamic, 3> im2_grad;
                    Eigen::Matrix<typename ParamType::Im2ValueType, Eigen::Dynamic, 1> im2_values;
                    if (im2_values.rows() != volumes) {
                        im2_values.resize (volumes);
                        im2_grad.resize (volumes, 3);
                    }

                    params.im1_image_interp->value_and_gradient_row_wrt_scanner (im1_values, im1_grad);
                    params.im2_image_interp->value_and_gradient_row_wrt_scanner (im2_values, im2_grad);

                    if (im1_values.hasNaN())
                        return;

                    if (im2_values.hasNaN())
                        return;

                    for (ssize_t i = 0; i < volumes; ++i) {

                        if (abs((default_type) im1_values[i]) > 0.0 && abs((default_type) im2_values[i]) > 0.0) {

                            local_count[i]++;
                            local_sf[i] = local_sf[i] + im1_values[i];
                            local_sm[i] = local_sm[i] + im2_values[i];
                            local_sff[i] = local_sff[i] + im1_values[i] * im1_values[i];
                            local_smm[i] = local_smm[i] + im2_values[i] * im2_values[i];
                            local_sfm[i] = local_sfm[i] + im1_values[i] * im2_values[i];
                        }
                    }

                    return;
                }

            };



            class GlobalCrossCorrelation4D { MEMALIGN(GlobalCrossCorrelation4D)

            private:

                Eigen::VectorXd mc_weights;
                bool weighted;
                default_type weight_sum;
                const default_type min_value_threshold;
                ssize_t volumes;

                default_type computed_global_cost;
                default_type computed_total_count;

                Eigen::VectorXd computed_sf;
                Eigen::VectorXd computed_sm;
                Eigen::VectorXd computed_sff;
                Eigen::VectorXd computed_smm;
                Eigen::VectorXd computed_sfm;
                Eigen::VectorXd computed_gncc;
                Eigen::VectorXd computed_count;

            public:

                /** requires_precompute:
                type_trait to distinguish metric types that require a call to precompute before the operator() is called */
                using requires_precompute = int;

                GlobalCrossCorrelation4D ( ) : weighted (false), weight_sum (0.0), min_value_threshold (1.e-7) { }

                void set_weights (Eigen::VectorXd weights) {

                    if (mc_weights.rows() != weights.rows()) {
                        mc_weights.resize (weights.rows());
                    }

                    mc_weights = weights;
                    weighted = mc_weights.rows() > 0;

                    weight_sum = 0.0;
                    if (mc_weights.rows() > 0) {
                        weight_sum = mc_weights.sum();
                    }

                }

                
                Eigen::VectorXd get_gncc() {
                    return computed_gncc;
                }
                

                template <class ParamType>
                default_type precompute(ParamType& params) {

                    computed_global_cost = 0.0;
                    computed_total_count = 0.0;

                    volumes = params.im1_image.size(3);

                    if (weight_sum < 1) {
                        weight_sum = (default_type) volumes;
                    }

                    computed_sf.resize (volumes);
                    computed_sm.resize (volumes);
                    computed_sff.resize (volumes);
                    computed_smm.resize (volumes);
                    computed_sfm.resize (volumes);
                    computed_count.resize (volumes);
                    computed_gncc.resize (volumes);

                    for (ssize_t i = 0; i < volumes; ++i) {
                        computed_sf[i] = 0;
                        computed_sm[i] = 0;
                        computed_gncc[i] = 0;
                        computed_count[i] = 0;
                        computed_sff[i] = 0;
                        computed_smm[i] = 0;
                        computed_sfm[i] = 0;
                    }

                    GNCCPrecomputeFunctorMasked4D_Naive<ParamType> compute_gncc (params, computed_count, computed_sf, computed_sm, computed_sff, computed_smm, computed_sfm, computed_gncc);
                    ThreadedLoop (params.midway_image, 0, 3).run (compute_gncc);

                    for (ssize_t i = 0; i < volumes; ++i) {

                        if (computed_count[i] > 0) {

                            computed_sff[i]  = computed_sff[i] - (computed_sf[i] * computed_sf[i] / computed_count[i]);
                            computed_smm[i] = computed_smm[i] - (computed_sm[i] * computed_sm[i] / computed_count[i]);
                            computed_sfm[i]= computed_sfm[i] - (computed_sf[i] * computed_sm[i] / computed_count[i]);
                            computed_gncc[i] = -1.0 * computed_sfm[i] / sqrt(computed_sff[i] * computed_smm[i]);

                        }
                    }

                    for (ssize_t i = 0; i < volumes; ++i) {

                        if (computed_count[i] > 0) {
                            computed_global_cost = computed_global_cost + computed_gncc[i];
                            computed_total_count = computed_total_count + computed_count[i];
                        }
                    }

                    return 0.0;

                }


                template <class Params>
                default_type operator() (Params& params,
                                         const Eigen::Vector3& im1_point,
                                         const Eigen::Vector3& im2_point,
                                         const Eigen::Vector3& midway_point,
                                         Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient) {

                    if (computed_total_count < 1)
                        return 0.0;

                    if (params.im2_mask_interp) {
                        params.im2_mask_interp->scanner (im2_point);
                        if (params.im2_mask_interp->value() < 0.5)
                            return 0.0;
                    }

                    if (params.im1_mask_interp) {
                        params.im1_mask_interp->scanner (im1_point);
                        if (params.im1_mask_interp->value() < 0.5)
                            return 0.0;
                    }

                    Eigen::Matrix<typename Params::Im1ValueType, Eigen::Dynamic, 3> im1_grad;
                    Eigen::Matrix<typename Params::Im1ValueType, Eigen::Dynamic, 1> im1_values;
                    if (im1_values.rows() != volumes) {
                        im1_values.resize (volumes);
                        im1_grad.resize (volumes, 3);
                    }

                    Eigen::Matrix<typename Params::Im2ValueType, Eigen::Dynamic, 3> im2_grad;
                    Eigen::Matrix<typename Params::Im2ValueType, Eigen::Dynamic, 1> im2_values;
                    if (im2_values.rows() != volumes) {
                        im2_values.resize (volumes);
                        im2_grad.resize (volumes, 3);
                    }

                    params.im1_image_interp->value_and_gradient_row_wrt_scanner (im1_values, im1_grad);
                    params.im2_image_interp->value_and_gradient_row_wrt_scanner (im2_values, im2_grad);

                    if (im1_values.hasNaN())
                        return 0.0;

                    if (im2_values.hasNaN())
                        return 0.0;

                    if (std::isnan (default_type (computed_global_cost)))
                        return 0.0;

                    const auto jacobian_vec = params.transformation.get_jacobian_vector_wrt_params (midway_point);

                    for (ssize_t i = 0; i < volumes; ++i) {

                        if (abs((default_type) im1_values[i]) > min_value_threshold || abs((default_type) im2_values[i]) > min_value_threshold) {

                            if ( computed_count[i] > 1 && (computed_smm[i]*computed_sff[i]) > min_value_threshold) {

                                Eigen::Vector3d g1 = (im1_values[i] - computed_sf[i] / computed_count[i]) * im2_grad.row(i);
                                Eigen::Vector3d g2 = (im2_values[i] - computed_sm[i] / computed_count[i]) * (computed_sfm[i] / computed_smm[i]) * im2_grad.row(i);

                                default_type current_volume_weight = 1 / ((default_type) volumes);
                                if (this->weighted)
                                    current_volume_weight = ((default_type) this->mc_weights(i)) / ((default_type) this->mc_weights.sum());

                                const Eigen::Vector3d g = current_volume_weight * (g1 - g2) * (computed_sfm[i] * computed_count[i] / ((computed_sff[i] * computed_smm[i])));

                                gradient.segment<4>(0) += g(0) * jacobian_vec;
                                gradient.segment<4>(4) += g(1) * jacobian_vec;
                                gradient.segment<4>(8) += g(2) * jacobian_vec;

                            }
                        }
                    }

                    return (computed_global_cost / ((default_type) volumes));

                }

            };

        
        
        
        
        template <class ParamType>
        class ACPrecomputeFunctorMasked4D_Naive{ MEMALIGN(ACPrecomputeFunctorMasked4D_Naive)

        protected:

            std::shared_ptr<std::mutex> mutex;
            ParamType params;
            size_t order;
            ssize_t volumes;
            transform_type voxel2scanner;
            
            default_type& global_count;
            default_type& global_ac;
            default_type local_ac;
            default_type local_count;
            
        public:

            ACPrecomputeFunctorMasked4D_Naive (const ParamType& parameters, size_t input_order, default_type& output_count, default_type& output_ac) :
            mutex (new std::mutex),
            params (parameters),
            order(input_order),
            volumes(parameters.im1_image.size(3)),
            voxel2scanner (MR::Transform(params.midway_image).voxel2scanner),
            global_count (output_count),
            global_ac(output_ac) {

                local_ac = 0;
                local_count = 0;

                if (params.robust_estimate_subset) {
                    assert (params.robust_estimate_subset_from.size() == 3);
                    transform_type i2s (params.midway_image.transform());
                    auto spacing = Eigen::DiagonalMatrix<default_type, 3> (params.midway_image.spacing(0), params.midway_image.spacing(1), params.midway_image.spacing(2));
                    for (size_t j = 0; j < 3; ++j)
                        for (size_t i = 0; i < 3; ++i)
                            i2s(i,3) += params.robust_estimate_subset_from[j] * params.midway_image.spacing(j) * i2s(i,j);
                    voxel2scanner = i2s * spacing;
                }
            }

            ~ACPrecomputeFunctorMasked4D_Naive () {

                std::lock_guard<std::mutex> lock (*mutex);

                global_count += local_count;
                global_ac += local_ac;

            }

            void operator() (const Iterator& iter) {

                Eigen::Vector3 voxel_pos ((default_type)iter.index(0), (default_type)iter.index(1), (default_type)iter.index(2));
                Eigen::Vector3 midway_point = voxel2scanner * voxel_pos;

                Eigen::Vector3 im1_point;
                params.transformation.transform_half (im1_point, midway_point);

                Eigen::Vector3 im2_point;
                params.transformation.transform_half_inverse (im2_point, midway_point);

                params.im1_image_interp->scanner (im1_point);
                if (!(*params.im1_image_interp))
                    return;

                params.im2_image_interp->scanner (im2_point);
                if (!(*params.im2_image_interp))
                    return;

                if (params.im2_mask_interp) {
                    params.im2_mask_interp->scanner (im2_point);
                    if (params.im2_mask_interp->value() < 0.5)
                    return;
                }

                if (params.im1_mask_interp) {
                    params.im1_mask_interp->scanner (im1_point);
                    if (params.im1_mask_interp->value() < 0.5)
                    return;
                }

                Eigen::Matrix<typename ParamType::Im1ValueType, Eigen::Dynamic, 3> im1_grad;
                Eigen::Matrix<typename ParamType::Im1ValueType, Eigen::Dynamic, 1> im1_values;
                if (im1_values.rows() != volumes) {
                    im1_values.resize (volumes);
                    im1_grad.resize (volumes, 3);
                }

                Eigen::Matrix<typename ParamType::Im2ValueType, Eigen::Dynamic, 3> im2_grad;
                Eigen::Matrix<typename ParamType::Im2ValueType, Eigen::Dynamic, 1> im2_values;
                if (im2_values.rows() != volumes) {
                    im2_values.resize (volumes);
                    im2_grad.resize (volumes, 3);
                }

                params.im1_image_interp->value_and_gradient_row_wrt_scanner (im1_values, im1_grad);
                params.im2_image_interp->value_and_gradient_row_wrt_scanner (im2_values, im2_grad);

                if (im1_values.hasNaN())
                    return;

                if (im2_values.hasNaN())
                    return;

                default_type current_sm = 0;
                default_type current_sf = 0;
                
                default_type current_sfm = 0;
                default_type current_sff = 0;
                default_type current_smm = 0;
                default_type current_ac = 0;
                default_type current_nvol = 0;
                
                for (size_t i = 1; i < order; i++) {
                    current_sf = current_sf + im1_values[i];
                    current_sm = current_sm + im2_values[i];
                    current_sfm = current_sfm + im1_values[i]*im2_values[i];
                    current_sff = current_sff + im1_values[i]*im1_values[i];
                    current_smm = current_smm + im2_values[i]*im2_values[i];
                    current_nvol = current_nvol + 1;
                }
                
                default_type min_value_threshold = 1.e-5;
                
                if (abs(current_sfm) > min_value_threshold) {
                    if (abs(current_smm*current_sff) > 0) {
                        default_type current_smmff = current_sff*current_smm;
                        current_ac = current_sfm / sqrt(current_smmff);
                        local_count++;
                    }
                }
                
                local_ac = local_ac + current_ac;

                return;
            }

        };


        class GlobalAngularCorrelation4D { MEMALIGN(GlobalAngularCorrelation4D)

        private:
            size_t order;
            default_type computed_ac;
            default_type computed_count;

        public:

            /** requires_precompute:
            type_trait to distinguish metric types that require a call to precompute before the operator() is called */
            using requires_precompute = int;

            GlobalAngularCorrelation4D ( ) { }

            default_type get_ac() {
                return computed_ac;
            }
            
            template <class ParamType>
            default_type precompute(ParamType& params, size_t input_order) {

                order = input_order;
                computed_count = 0;
                computed_ac = 0;
                
                ACPrecomputeFunctorMasked4D_Naive<ParamType> compute_ac (params, order, computed_count, computed_ac);
                ThreadedLoop (params.midway_image, 0, 3).run (compute_ac);

                computed_ac = computed_ac / computed_count;

                return 0.0;
            }

        };


        }
    }
}
#endif
