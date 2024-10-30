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

#include "mrtrix.h"
#include "opengl/gl.h"

namespace MR::GUI::MRView {

class ComboBoxWithErrorMsg : public QComboBox {
  Q_OBJECT

public:
  ComboBoxWithErrorMsg(QWidget *parent, const QString &msg);

  void setError();
  void clearError(int);
  void clearError() { clearError(-1); } // Guaranteed to clear the error entry

protected slots:
  void onSetIndex(int);

protected:
  const QString error_message;
  int error_index;
};

} // namespace MR::GUI::MRView
