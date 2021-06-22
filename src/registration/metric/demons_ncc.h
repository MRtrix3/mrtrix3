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

#ifndef __registration_metric_demons_ncc_h__
#define __registration_metric_demons_ncc_h__

#include <mutex>
#include "image_helpers.h"

namespace MR
{
    namespace Registration
    {
        namespace Metric
        {

            template <class Im1ImageType, class Im2ImageType, class Im1MaskType, class Im2MaskType>
            class DemonsLNCC { MEMALIGN(DemonsLNCC<Im1ImageType, Im2ImageType, Im1MaskType, Im2MaskType>)

                protected:

                default_type& global_cost;
                size_t& global_voxel_count;
                ssize_t kernel_radius;

                Im1ImageType im1_proc;
                Im2ImageType im2_proc;

                Adapter::Gradient3D<Im1ImageType> im1_gradient;
                Adapter::Gradient3D<Im2ImageType> im2_gradient;

                Im1MaskType im1_mask;
                Im2MaskType im2_mask;

                default_type volume_weight;

                default_type thread_cost;
                size_t thread_voxel_count;
                std::shared_ptr<std::mutex> mutex;

                const default_type min_value_threshold;
                int current_dim;
                const bool flag_combine_updates;

                public:

                DemonsLNCC (default_type& global_energy, size_t& global_voxel_count, ssize_t& radius, const Im1ImageType im1_image, const Im2ImageType im2_image,
                    const Im1MaskType im1_mask, const Im2MaskType im2_mask, default_type& volume_weight, bool flag_combine_updates) :
                global_cost (global_energy),
                global_voxel_count (global_voxel_count),
                kernel_radius (radius),
                im1_proc (im1_image),
                im2_proc (im2_image),
                im1_gradient (im1_image, true),
                im2_gradient (im2_image, true),
                im1_mask (im1_mask),
                im2_mask (im2_mask),
                volume_weight (volume_weight),
                thread_cost (0.0),
                thread_voxel_count (0),
                mutex (new std::mutex),
                min_value_threshold (1.e-5),
                current_dim (0),
                flag_combine_updates (flag_combine_updates) {
                    if (im1_image.buffer->ndim() > 3)
                        current_dim = im1_image.index(3);
                }

                ~DemonsLNCC () {
                    std::lock_guard<std::mutex> lock (*mutex);
                    global_cost += thread_cost;
                    global_voxel_count += thread_voxel_count;
                }


                void operator() (const Im1ImageType& im1_image, const Im2ImageType& im2_image,
                                 Image<default_type>& im1_update, Image<default_type>& im2_update)
                {

                    if (im1_image.index(0) <= kernel_radius || im1_image.index(0) >= im1_image.size(0) - kernel_radius ||
                        im1_image.index(1) <= kernel_radius || im1_image.index(1) >= im1_image.size(1) - kernel_radius ||
                        im1_image.index(2) <= kernel_radius || im1_image.index(2) >= im1_image.size(2) - kernel_radius) {
                            return;
                    }

                    if (!flag_combine_updates) {
                        im1_update.row(3) = 0.0;
                        im2_update.row(3) = 0.0;
                    }

                    assign_pos_of (im1_image, 0, 3).to (im1_proc);
                    assign_pos_of (im2_image, 0, 3).to (im2_proc);

                    if (im1_image.buffer->ndim() > 3) {
                        im1_proc.index(3) = current_dim;
                        im2_proc.index(3) = current_dim;
                    }

                    typename Im1MaskType::value_type im1_mask_value = 1.0;
                    if (im1_mask.valid()) {
                        assign_pos_of (im1_image, 0, 3).to (im1_mask);
                        im1_mask_value = im1_mask.value();
                        if (im1_mask_value < 0.1) {
                            im1_update.row(3) = 0.0;
                            im2_update.row(3) = 0.0;
                            return;
                        }
                    }

                    typename Im2MaskType::value_type im2_mask_value = 1.0;
                    if (im2_mask.valid()) {
                        assign_pos_of (im2_image, 0, 3).to (im2_mask);
                        im2_mask_value = im2_mask.value();
                        if (im2_mask_value < 0.1) {
                            im1_update.row(3) = 0.0;
                            im2_update.row(3) = 0.0;
                            return;
                        }
                    }

                    typename Im1ImageType::value_type im1_value = im1_image.value();
                    typename Im2ImageType::value_type im2_value = im2_image.value();

                    if (abs(im1_value) < min_value_threshold || abs(im2_value) < min_value_threshold) {
                        return;
                    }

                    const ssize_t im1_i1 = im1_image.index(0);
                    const ssize_t im1_i2 = im1_image.index(1);
                    const ssize_t im1_i3 = im1_image.index(2);
                    const ssize_t im2_i1 = im2_image.index(0);
                    const ssize_t im2_i2 = im2_image.index(1);
                    const ssize_t im2_i3 = im2_image.index(2);

                    default_type local_sf, local_sm, local_sff, local_smm, local_sfm;
                    local_sf = 0; local_sm = 0; local_sff = 0; local_smm = 0; local_sfm = 0;
                    default_type local_count = 0;

                    ssize_t im1_j1, im1_j2, im1_j3;
                    ssize_t im2_j1, im2_j2, im2_j3;

                    for (ssize_t e_ind1 = -kernel_radius; e_ind1 <= kernel_radius; e_ind1++) {
                        for (ssize_t e_ind2 = -kernel_radius; e_ind2 <= kernel_radius; e_ind2++) {
                            for (ssize_t e_ind3 = -kernel_radius; e_ind3 <= kernel_radius; e_ind3++) {

                                im1_j1 = im1_i1 + e_ind1; im1_j2 = im1_i2 + e_ind2; im1_j3 = im1_i3 + e_ind3;
                                im2_j1 = im2_i1 + e_ind1; im2_j2 = im2_i2 + e_ind2; im2_j3 = im2_i3 + e_ind3;

                                im1_proc.index(0) = im1_j1; im1_proc.index(1) = im1_j2; im1_proc.index(2) = im1_j3;
                                im2_proc.index(0) = im2_j1; im2_proc.index(1) = im2_j2; im2_proc.index(2) = im2_j3;

                                const typename Im1ImageType::value_type im1_value_iter = im1_proc.value();
                                const typename Im2ImageType::value_type im2_value_iter = im2_proc.value();

                                if (abs(im1_value_iter) > min_value_threshold && abs(im2_value_iter) > min_value_threshold && !std::isnan(im1_value_iter) && !std::isnan(im2_value_iter)) {
                                    local_sf += im1_value_iter;
                                    local_sm += im2_value_iter;
                                    local_sff += im1_value_iter*im1_value_iter;
                                    local_smm += im2_value_iter*im2_value_iter;
                                    local_sfm += im1_value_iter*im2_value_iter;
                                    local_count += 1;
                                }
                            }
                        }
                    }

                    if (local_count > 0) {
                        local_sff -= local_sf * local_sf / local_count;
                        local_smm -= local_sm * local_sm / local_count;
                        local_sfm -= local_sf * local_sm / local_count;

                        const default_type denom = sqrt(local_sff * local_smm);

                        if (denom < min_value_threshold || local_smm < min_value_threshold || local_sff < min_value_threshold || std::isnan(denom))
                            return;

                        assign_pos_of (im1_image, 0, 3).to (im1_gradient, im2_gradient);

                        Eigen::Matrix<default_type, 3, 1> grad_im1 = im1_gradient.value();
                        Eigen::Matrix<default_type, 3, 1> grad_im2 = im2_gradient.value();

                        Eigen::Matrix<default_type, 3, 1>  dfm_im1;
                        Eigen::Matrix<default_type, 3, 1>  dfm_im2;

                        dfm_im1 = volume_weight * 2.0 * (local_sfm * local_count / (local_sff * local_smm)) *
                            ((im2_value - local_sm / local_count) - (im1_value - local_sf / local_count) * (local_sfm / local_sff)) * grad_im1;
                        dfm_im2 = volume_weight * 2.0 * (local_sfm * local_count / (local_sff * local_smm)) *
                            ((im1_value - local_sf / local_count) - (im2_value - local_sm / local_count) * (local_sfm / local_smm)) * grad_im2;

                        im1_update.row(3) += dfm_im1;
                        im2_update.row(3) += dfm_im2;

                        thread_cost -= local_sfm / denom;
                        thread_voxel_count++;
                    }
                }

            };



            template <class Im1ImageType, class Im2ImageType, class Im1MaskType, class Im2MaskType>
            class PrecomputeGNCC { MEMALIGN(PrecomputeGNCC<Im1ImageType,Im2ImageType,Im1MaskType,Im2MaskType>)

                protected:
                std::shared_ptr<std::mutex> mutex;

                Im1MaskType im1_mask_const;
                Im2MaskType im2_mask_const;
                Im1ImageType im1_image_const;
                Im2ImageType im2_image_const;

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
                const default_type min_value_threshold;

                public:

                PrecomputeGNCC (const Im1ImageType& im1_image_in, const Im2ImageType& im2_image_in,
                                const Im1MaskType im1_mask_in, const Im2MaskType im2_mask_in,
                                default_type& output_count, default_type& output_sf, default_type& output_sm,
                                default_type& output_sff, default_type& output_smm, default_type& output_sfm, default_type &output_gncc) :
                mutex (new std::mutex),
                im1_mask_const (im1_mask_in),
                im2_mask_const (im2_mask_in),
                im1_image_const (im1_image_in),
                im2_image_const (im2_image_in),
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
                min_value_threshold(1.e-5) { }


                ~PrecomputeGNCC () {
                    std::lock_guard<std::mutex> lock (*mutex);

                    if (local_count > 0 && local_count == local_count) {
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
                        if (sqrt(sff * smm) > 0) {
                            global_gncc = 1.0 * sfm / sqrt(sff * smm);
                        }
                    }
                }

                void operator() (const Im1ImageType& im1_image,
                                 const Im2ImageType& im2_image) {

                    if (im1_image.index(0) == 0 || im1_image.index(0) == im1_image.size(0) - 1 ||
                        im1_image.index(1) == 0 || im1_image.index(1) == im1_image.size(1) - 1 ||
                        im1_image.index(2) == 0 || im1_image.index(2) == im1_image.size(2) - 1) {
                        return;
                    }


                    typename Im1MaskType::value_type im1_mask_value = 1.0;
                    if (im1_mask_const.valid()) {
                        assign_pos_of (im1_image, 0, 3).to (im1_mask_const);
                        im1_mask_value = im1_mask_const.value();
                        if (im1_mask_value < 0.1)
                            return;
                    }

                    typename Im2MaskType::value_type im2_mask_value = 1.0;
                    if (im2_mask_const.valid()) {
                        assign_pos_of (im2_image, 0, 3).to (im2_mask_const);
                        im2_mask_value = im2_mask_const.value();
                        if (im2_mask_value < 0.1)
                            return;
                    }

                    typename Im1ImageType::value_type im1_value = im1_image.value();
                    typename Im2ImageType::value_type im2_value = im2_image.value();

                    if (abs(im1_value) < min_value_threshold || abs(im2_value) < min_value_threshold) {
                        return;
                    }

                    local_sf += im1_value;
                    local_sm += im2_value;

                    local_sff += im1_value * im1_value;
                    local_smm += im2_value * im2_value;
                    local_sfm += im1_value * im2_value;

                    local_count += 1;
                }
            };




            template <class Im1ImageType, class Im2ImageType, class Im1MaskType, class Im2MaskType>
            class DemonsGNCC { MEMALIGN(DemonsGNCC<Im1ImageType, Im2ImageType, Im1MaskType, Im2MaskType>)

                protected:

                default_type& global_cost;
                size_t& global_voxel_count;

                Im1ImageType im1_proc;
                Im2ImageType im2_proc;

                Adapter::Gradient3D<Im1ImageType> im1_gradient;
                Adapter::Gradient3D<Im2ImageType> im2_gradient;

                Im1MaskType im1_mask;
                Im2MaskType im2_mask;

                default_type volume_weight;

                default_type thread_cost;
                size_t thread_voxel_count;
                std::shared_ptr<std::mutex> mutex;

                const default_type min_value_threshold;
                int current_dim;
                const bool flag_combine_updates;

                default_type computed_global_cost;
                default_type computed_sf;
                default_type computed_sm;
                default_type computed_sff;
                default_type computed_smm;
                default_type computed_sfm;
                default_type computed_gncc;
                default_type computed_total_count;


                public:

                DemonsGNCC (default_type& global_energy, size_t& global_voxel_count, const Im1ImageType im1_image, const Im2ImageType im2_image,
                    const Im1MaskType im1_mask, const Im2MaskType im2_mask, default_type& volume_weight, bool flag_combine_updates) :
                global_cost (global_energy),
                global_voxel_count (global_voxel_count),
                im1_proc (im1_image),
                im2_proc (im2_image),
                im1_gradient (im1_image, true),
                im2_gradient (im2_image, true),
                im1_mask (im1_mask),
                im2_mask (im2_mask),
                volume_weight (volume_weight),
                thread_cost (0.0),
                thread_voxel_count (0),
                mutex (new std::mutex),
                min_value_threshold (1.e-5),
                current_dim (0),
                flag_combine_updates(flag_combine_updates) {
                    if (im1_image.buffer->ndim() > 3)
                    current_dim = im1_image.index(3);

                }

                void precompute () {

                    default_type gncc = 0;
                    default_type total_count = 0;
                    default_type sf = 0;
                    default_type sm = 0;
                    default_type sff = 0;
                    default_type smm = 0;
                    default_type sfm = 0;

                    Metric::PrecomputeGNCC<Im1ImageType, Im2ImageType, Im1MaskType, Im2MaskType> precompute (im1_proc, im2_proc, im1_mask, im2_mask, total_count, sf, sm, sff, smm, sfm, gncc);
                    ThreadedLoop (im1_proc, 0, 3).run (precompute, im1_proc, im2_proc);

                    if (total_count < 1) {
                        computed_sf = 0;
                        computed_sm = 0;
                        computed_gncc = 0;
                        computed_total_count = 0;
                        computed_sff = 0;
                        computed_smm = 0;
                        computed_sfm = 0;
                        computed_global_cost = 0;
                        return;
                    }

                    computed_sf = sf;
                    computed_sm = sm;
                    computed_gncc = gncc;
                    computed_total_count = total_count;
                    computed_sff = sff - sf * sf / total_count;
                    computed_smm = smm - sm * sm / total_count;
                    computed_sfm = sfm - sf * sm / total_count;

                    if (computed_sff > 0 && computed_smm > 0) {
                        computed_global_cost = computed_sfm / sqrt(computed_sff * computed_smm);
                    } else {
                        computed_global_cost = 0;
                    }

                }

                ~DemonsGNCC () {
                    std::lock_guard<std::mutex> lock (*mutex);
                    global_cost += thread_cost;
                    global_voxel_count += thread_voxel_count;
                }


                void operator () (const Im1ImageType& im1_image, const Im2ImageType& im2_image,
                                  Image<default_type>& im1_update, Image<default_type>& im2_update)
                {

                    if (!flag_combine_updates) {
                        im1_update.row(3) = 0.0;
                        im2_update.row(3) = 0.0;
                    }

                    if (im1_image.buffer->ndim() > 3) {
                        im1_proc.index(3) = current_dim;
                        im2_proc.index(3) = current_dim;
                    }

                    if (im1_image.index(0) == 0 || im1_image.index(0) == im1_image.size(0) - 1 ||
                        im1_image.index(1) == 0 || im1_image.index(1) == im1_image.size(1) - 1 ||
                        im1_image.index(2) == 0 || im1_image.index(2) == im1_image.size(2) - 1) {
                        return;
                    }

                    typename Im1MaskType::value_type im1_mask_value = 1.0;
                    if (im1_mask.valid()) {
                        assign_pos_of (im1_image, 0, 3).to (im1_mask);
                        im1_mask_value = im1_mask.value();
                        if (im1_mask_value < 0.1) {
                            im1_update.row(3) = 0.0;
                            im2_update.row(3) = 0.0;
                            return;
                        }
                    }

                    typename Im2MaskType::value_type im2_mask_value = 1.0;
                    if (im2_mask.valid()) {
                        assign_pos_of (im2_image, 0, 3).to (im2_mask);
                        im2_mask_value = im2_mask.value();
                        if (im2_mask_value < 0.1) {
                            im1_update.row(3) = 0.0;
                            im2_update.row(3) = 0.0;
                            return;
                        }
                    }


                    typename Im1ImageType::value_type im1_value = im1_image.value();
                    typename Im2ImageType::value_type im2_value = im2_image.value();

                    if (abs(im1_value) < min_value_threshold || abs(im2_value) < min_value_threshold) {
                        return;
                    }

                    if (computed_total_count == 0 || computed_smm < min_value_threshold || computed_sff < min_value_threshold)
                        return;

                    assign_pos_of (im1_image, 0, 3).to (im1_gradient, im2_gradient);

                    const Eigen::Matrix<default_type, 3, 1> grad_im1 = im1_gradient.value();
                    const Eigen::Matrix<default_type, 3, 1> grad_im2 = im2_gradient.value();

                    Eigen::Matrix<default_type, 3, 1>  dfm_im1;
                    Eigen::Matrix<default_type, 3, 1>  dfm_im2;

                    dfm_im1 = volume_weight * 2.0 * (computed_sfm * computed_total_count / ((computed_sff * computed_smm))) *
                        ((im2_value - computed_sm / computed_total_count) - (im1_value - computed_sf / computed_total_count) * (computed_sfm / computed_sff)) * grad_im1;
                    dfm_im2 = volume_weight * 2.0 * (computed_sfm * computed_total_count / ((computed_sff * computed_smm))) *
                        ((im1_value - computed_sf / computed_total_count) - (im2_value - computed_sm / computed_total_count) * (computed_sfm / computed_smm)) * grad_im2;

                    im1_update.row(3) += dfm_im1;
                    im2_update.row(3) += dfm_im2;

                    thread_cost -= computed_global_cost;
                    thread_voxel_count++;
                }
            };



            template <class Im1ImageType, class Im2ImageType, class Im1MaskType, class Im2MaskType>
            void run_DemonsLNCC_4D (default_type& cost_new, size_t& voxel_count, ssize_t kernel_radius,
                                    Im1ImageType& im1_image, Im2ImageType& im2_image,
                                    Im1MaskType& im1_mask, Im2MaskType& im2_mask,
                                    Image<default_type>& im1_update, Image<default_type>& im2_update,
                                    const vector<MultiContrastSetting>* contrast_settings = nullptr)
            {
                ssize_t nvols = im1_image.size(3);

                Eigen::VectorXd volume_weights;
                volume_weights.resize (nvols);

                default_type mc_sum = 0.0;

                if (contrast_settings and contrast_settings->size() > 1) {
                    for (const auto & mc : *contrast_settings) {
                        mc_sum += mc.weight;
                    }
                    for (const auto & mc : *contrast_settings) {
                        volume_weights.segment(mc.start, mc.nvols).fill(mc.weight / mc.nvols);
                        // TODO changed from mc.weight / (mc.nvols * contrast_settings->size()) to mc.weight to as
                        // TODO intended use of mc.weight is the weight for that specific contrast. @Alena unify with demons4D.h and don't normalise by nvols?
                    }
                } else {
                    volume_weights.fill(1.0 / default_type (nvols));  // TODO unify with demons4D.h and don't normalise by nvols?
                    mc_sum = default_type (nvols);
                }


                default_type local_cost = 0;
                size_t local_count = 0;

                bool flag_combine_updates = false;

                for (int i=0; i<nvols; i++) {  // ENH: speedup possible for vectorised metric instead of loop over volumes?
                    default_type volume_weight = volume_weights[i];
                    if (i==0) {
                        cost_new = 0;
                    }

                    im1_image.index(3) = i;
                    im2_image.index(3) = i;

                    local_cost = 0;
                    local_count = 0;

                    if (kernel_radius > 0) {
                        Metric::DemonsLNCC<Im1ImageType, Im2ImageType, Im1MaskType, Im2MaskType> metric (local_cost, local_count, kernel_radius, im1_image, im2_image,
                            im1_mask, im2_mask, volume_weight, flag_combine_updates);
                        ThreadedLoop (im1_image, 0, 3).run (metric, im1_image, im2_image, im1_update, im2_update);
                    } else {
                        Metric::DemonsGNCC<Im1ImageType, Im2ImageType, Im1MaskType, Im2MaskType> metric (local_cost, local_count, im1_image, im2_image,
                            im1_mask, im2_mask, volume_weight, flag_combine_updates);
                        metric.precompute ();
                        ThreadedLoop (im1_image, 0, 3).run (metric, im1_image, im2_image, im1_update, im2_update);
                    }

                    cost_new = cost_new + local_cost;
                    voxel_count = voxel_count + local_count;

                    flag_combine_updates = true;
                }

                im1_image.index(3) = 0;
                im2_image.index(3) = 0;

                return;
            }


        }
    }
}
#endif

