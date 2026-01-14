/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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
#include "twi_stats.h"

#include <string>
#include <vector>

namespace MR::DWI::Tractography::Mapping {

const std::unordered_map<contrast_t, Strings> contrast_names{
    {contrast_t::TDI, {"tdi", "track density"}},
    {contrast_t::LENGTH, {"length", "track length"}},
    {contrast_t::INVLENGTH, {"invlength", "inverse track length"}},
    {contrast_t::SCALAR_MAP, {"scalar_map", "sampled scalar map"}},
    {contrast_t::SCALAR_MAP_COUNT, {"scalar_map_count", "scalar-map-thresholded tdi"}},
    {contrast_t::FOD_AMP, {"fod_amp", "FOD amplitude"}},
    {contrast_t::CURVATURE, {"curvature", "curvature}"}},
    {contrast_t::VECTOR_FILE, {"vector_file", "external-file-based"}}};

const std::vector<std::string> contrasts = {
    "tdi", "length", "invlength", "scalar_map", "scalar_map_count", "fod_amp", "curvature", "vector_file"};

// TODO Remove "V_" prefix
const std::unordered_map<vox_stat_t, std::string> voxel_statistic_names{
    {vox_stat_t::SUM, "sum"}, {vox_stat_t::MIN, "mininum"}, {vox_stat_t::MEAN, "mean"}, {vox_stat_t::MAX, "maximum"}};
const std::vector<std::string> voxel_statistics = {"sum", "min", "mean", "max"};

const std::unordered_map<tck_stat_t, Strings> track_statistic_names{
    {tck_stat_t::SUM, {"sum", "sum"}},
    {tck_stat_t::MIN, {"min", "minimum"}},
    {tck_stat_t::MEAN, {"mean", "mean"}},
    {tck_stat_t::MAX, {"max", "maximum"}},
    {tck_stat_t::MEDIAN, {"median", "median"}},
    {tck_stat_t::MEAN_NONZERO, {"mean_nonzero", "mean of non-zero values"}},
    {tck_stat_t::GAUSSIAN, {"gaussian", "gaussian-smoothed"}},
    {tck_stat_t::ENDS_MIN, {"ends_min", "minimum-of-endpoints"}},
    {tck_stat_t::ENDS_MEAN, {"ends_mean", "mean-of-endpoints"}},
    {tck_stat_t::ENDS_MAX, {"ends_max", "maximum-of-endpoints"}},
    {tck_stat_t::ENDS_PROD, {"ends_prod", "product-of-endpoints"}},
    {tck_stat_t::ENDS_CORR, {"ends_corr", "correlation-between-endpoints"}}};

// Note: ENDS_CORR not provided as a command-line option
const std::vector<std::string> track_statistics = {"sum",
                                                   "min",
                                                   "mean",
                                                   "max",
                                                   "median",
                                                   "mean_nonzero",
                                                   "gaussian",
                                                   "ends_min",
                                                   "ends_mean",
                                                   "ends_max",
                                                   "ends_prod"};

} // namespace MR::DWI::Tractography::Mapping
