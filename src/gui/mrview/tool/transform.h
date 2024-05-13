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

#include "gui/mrview/mode/base.h"
#include "gui/mrview/tool/base.h"

namespace MR::GUI::MRView::Tool {

class Transform : public Base, public Tool::CameraInteractor {
  Q_OBJECT
public:
  Transform(Dock *parent);

  bool slice_move_event(const ModelViewProjection &projection, float inc) override;
  bool pan_event(const ModelViewProjection &projection) override;
  bool panthrough_event(const ModelViewProjection &projection) override;
  bool tilt_event(const ModelViewProjection &projection) override;
  bool rotate_event(const ModelViewProjection &projection) override;

protected:
  QPushButton *activate_button;
  virtual void showEvent(QShowEvent *event) override;
  virtual void closeEvent(QCloseEvent *event) override;
  virtual void hideEvent(QHideEvent *event) override;

  void setActive(bool onoff);

protected slots:
  void onActivate(bool);
};

} // namespace MR::GUI::MRView::Tool
