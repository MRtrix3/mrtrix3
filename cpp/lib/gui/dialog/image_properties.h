/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#include "header.h"
#include "opengl/gl.h"

namespace MR {
class Header;

namespace GUI::Dialog {
class TreeModel;

class ImageProperties : public QDialog {
  Q_OBJECT

public:
  ImageProperties(QWidget *parent, const MR::Header &header);

private slots:
  void context_menu(const QPoint &point);
  void write_to_file();

private:
  const MR::Header &H;
  QTreeView *view;
  TreeModel *model;
  Eigen::MatrixXd save_data;
};

} // namespace GUI::Dialog

} // namespace MR
