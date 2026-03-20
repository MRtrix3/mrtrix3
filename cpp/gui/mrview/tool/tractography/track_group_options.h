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

#pragma once

#include <vector>

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace MR::GUI::MRView {
class Window;
} // namespace MR::GUI::MRView

namespace MR::GUI::MRView::Tool {
class Tractogram;
class Tractography;

// Panel that appears when a TRX tractogram with groups is selected.
// Provides per-group visibility toggles, color swatches, and multi-group
// priority policy.  Shown/hidden by Tractography::update_scalar_options().
class TrackGroupOptions : public QGroupBox {
  Q_OBJECT

public:
  TrackGroupOptions(Tractography *parent);

  void set_tractogram(Tractogram *t);
  void update_UI();

private slots:
  void multi_policy_changed(int index);
  void show_ungrouped_changed(int state);

private:
  void rebuild_rows();

  Tractography *tool;
  Tractogram *tractogram;

  QScrollArea *scroll_area;
  QWidget *rows_widget;
  QVBoxLayout *rows_layout;
  QCheckBox *show_ungrouped_box;
  QComboBox *multi_policy_combo;

  std::vector<QWidget *> row_widgets;

  Window &window() const;
};

} // namespace MR::GUI::MRView::Tool
