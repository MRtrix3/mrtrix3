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
#include "registration/transform/reorient.h"

namespace MR::Registration::Transform {
template void reorient<Image<double>>(Image<double> &input_fod_image,
                                      Image<double> &output_fod_image,
                                      const transform_type &transform,
                                      const Eigen::MatrixXd &directions,
                                      bool modulate,
                                      std::vector<MultiContrastSetting> multi_contrast_settings);

template void reorient<Image<float>>(Image<float> &input_fod_image,
                                     Image<float> &output_fod_image,
                                     const transform_type &transform,
                                     const Eigen::MatrixXd &directions,
                                     bool modulate,
                                     std::vector<MultiContrastSetting> multi_contrast_settings);

template void reorient<Image<double>>(const std::string progress_message,
                                      Image<double> &input_fod_image,
                                      Image<double> &output_fod_image,
                                      const transform_type &transform,
                                      const Eigen::MatrixXd &directions,
                                      bool modulate,
                                      std::vector<MultiContrastSetting> multi_contrast_settings);

template void reorient<Image<float>>(const std::string progress_message,
                                     Image<float> &input_fod_image,
                                     Image<float> &output_fod_image,
                                     const transform_type &transform,
                                     const Eigen::MatrixXd &directions,
                                     bool modulate,
                                     std::vector<MultiContrastSetting> multi_contrast_settings);

template void reorient_warp<Image<double>>(Image<double> &fod_image,
                                           Image<default_type> &warp,
                                           const Eigen::MatrixXd &directions,
                                           const bool modulate,
                                           std::vector<MultiContrastSetting> multi_contrast_settings);

template void reorient_warp<Image<float>>(Image<float> &fod_image,
                                          Image<default_type> &warp,
                                          const Eigen::MatrixXd &directions,
                                          const bool modulate,
                                          std::vector<MultiContrastSetting> multi_contrast_settings);

template void reorient_warp<Image<double>>(const std::string progress_message,
                                           Image<double> &fod_image,
                                           Image<default_type> &warp,
                                           const Eigen::MatrixXd &directions,
                                           const bool modulate,
                                           std::vector<MultiContrastSetting> multi_contrast_settings);

template void reorient_warp<Image<float>>(const std::string progress_message,
                                          Image<float> &fod_image,
                                          Image<default_type> &warp,
                                          const Eigen::MatrixXd &directions,
                                          const bool modulate,
                                          std::vector<MultiContrastSetting> multi_contrast_settings);

} // namespace MR::Registration::Transform
