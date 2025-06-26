/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#pragma once

#include <memory>

#include "dwi/shells.h"

#include "dwi/directions/set.h"
#include "dwi/renderer.h"
#include "mrview/gui_image.h"
#include "mrview/tool/odf/type.h"

namespace MR {

class Header;

namespace GUI::MRView::Tool {

class ODF_Item {
public:
  ODF_Item(MR::Header &&H,
           const odf_type_t type,
           const float scale,
           const bool hide_negative,
           const bool color_by_direction);

  bool valid() const;

  MRView::Image image;
  odf_type_t odf_type;
  int lmax;
  float scale;
  bool hide_negative, color_by_direction;

  class DixelPlugin {
  public:
    enum dir_t { DW_SCHEME, HEADER, INTERNAL, NONE, FILE };

    DixelPlugin(const MR::Header &);

    void set_shell(size_t index);
    void set_header();
    void set_internal(const size_t n);
    void set_none();
    void set_from_file(const std::string &path);

    Eigen::VectorXf get_shell_data(const Eigen::VectorXf &values) const;

    size_t num_DW_shells() const;

    dir_t dir_type;
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> header_dirs;
    Eigen::Matrix<double, Eigen::Dynamic, 4> grad;
    std::unique_ptr<MR::DWI::Shells> shells;
    size_t shell_index;
    std::unique_ptr<MR::DWI::Directions::Set> dirs;
  };
  std::unique_ptr<DixelPlugin> dixel;
};

} // namespace GUI::MRView::Tool

} // namespace MR
