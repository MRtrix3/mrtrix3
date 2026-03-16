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

#include "mrview/tool/tractography/track_group_options.h"

#include <QColorDialog>
#include <QHBoxLayout>
#include <QLabel>

#include "mrview/tool/tractography/tractogram.h"
#include "mrview/tool/tractography/tractography.h"
#include "mrview/window.h"

namespace MR::GUI::MRView::Tool {

TrackGroupOptions::TrackGroupOptions(Tractography *parent)
    : QGroupBox("TRX Groups", parent), tool(parent), tractogram(nullptr) {

  auto *main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(4, 4, 4, 4);
  main_layout->setSpacing(4);

  auto *policy_row = new QHBoxLayout;
  policy_row->addWidget(new QLabel("Multi-group:"));
  multi_policy_combo = new QComboBox(this);
  multi_policy_combo->addItem("First match");
  multi_policy_combo->addItem("Last match");
  connect(multi_policy_combo, SIGNAL(activated(int)), this, SLOT(multi_policy_changed(int)));
  policy_row->addWidget(multi_policy_combo);
  main_layout->addLayout(policy_row);

  rows_widget = new QWidget;
  rows_layout = new QVBoxLayout(rows_widget);
  rows_layout->setContentsMargins(0, 0, 0, 0);
  rows_layout->setSpacing(1);
  rows_layout->addStretch(1);

  scroll_area = new QScrollArea(this);
  scroll_area->setWidgetResizable(true);
  scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scroll_area->setMaximumHeight(160);
  scroll_area->setWidget(rows_widget);
  main_layout->addWidget(scroll_area);

  show_ungrouped_box = new QCheckBox("Show ungrouped", this);
  show_ungrouped_box->setChecked(true);
  connect(show_ungrouped_box, SIGNAL(stateChanged(int)), this, SLOT(show_ungrouped_changed(int)));
  main_layout->addWidget(show_ungrouped_box);

  setVisible(false);
}

Window &TrackGroupOptions::window() const { return *Window::main; }

void TrackGroupOptions::set_tractogram(Tractogram *t) { tractogram = t; }

void TrackGroupOptions::update_UI() {
  if (!tractogram || !tractogram->is_trx() || tractogram->trx_group_names().empty()) {
    setVisible(false);
    return;
  }
  setVisible(true);

  if (tractogram->group_states.empty())
    tractogram->init_group_states();

  multi_policy_combo->blockSignals(true);
  multi_policy_combo->setCurrentIndex(tractogram->group_multi_policy == GroupMultiPolicy::FirstMatch ? 0 : 1);
  multi_policy_combo->blockSignals(false);

  show_ungrouped_box->blockSignals(true);
  show_ungrouped_box->setChecked(tractogram->show_ungrouped);
  show_ungrouped_box->blockSignals(false);

  rebuild_rows();
}

void TrackGroupOptions::rebuild_rows() {
  for (auto *w : row_widgets) {
    rows_layout->removeWidget(w);
    delete w;
  }
  row_widgets.clear();

  if (!tractogram)
    return;

  for (const auto &name : tractogram->group_order) {
    auto git = tractogram->group_states.find(name);
    if (git == tractogram->group_states.end())
      continue;
    const auto &gs = git->second;

    auto *row = new QWidget(rows_widget);
    auto *hl = new QHBoxLayout(row);
    hl->setContentsMargins(0, 0, 0, 0);
    hl->setSpacing(4);

    const std::string label_text = name + " (" + std::to_string(gs.count) + ")";
    auto *cb = new QCheckBox(QString::fromStdString(label_text), row);
    cb->setChecked(gs.visible);

    const std::string group_name = name; // capture by value
    connect(cb, &QCheckBox::stateChanged, this, [this, group_name](int state) {
      if (!tractogram)
        return;
      auto it = tractogram->group_states.find(group_name);
      if (it != tractogram->group_states.end())
        it->second.visible = (state == Qt::Checked);
      tractogram->reload_group_colours();
      window().updateGL();
    });
    hl->addWidget(cb, 1);

    auto *swatch = new QPushButton(row);
    swatch->setFixedSize(20, 20);
    const auto &c = gs.color;
    const int r = static_cast<int>(c[0] * 255.0f);
    const int g = static_cast<int>(c[1] * 255.0f);
    const int b = static_cast<int>(c[2] * 255.0f);
    swatch->setStyleSheet(
        QString("QPushButton { background-color: rgb(%1,%2,%3); border: 1px solid #888; }").arg(r).arg(g).arg(b));

    connect(swatch, &QPushButton::clicked, this, [this, group_name, swatch]() {
      if (!tractogram)
        return;
      auto it = tractogram->group_states.find(group_name);
      if (it == tractogram->group_states.end())
        return;
      const auto &old_c = it->second.color;
      const QColor initial(static_cast<int>(old_c[0] * 255.0f),
                           static_cast<int>(old_c[1] * 255.0f),
                           static_cast<int>(old_c[2] * 255.0f));
      const QColor chosen =
          QColorDialog::getColor(initial, this, "Select group colour", QColorDialog::DontUseNativeDialog);
      if (!chosen.isValid())
        return;
      it->second.color = {chosen.redF(), chosen.greenF(), chosen.blueF()};
      swatch->setStyleSheet(QString("QPushButton { background-color: rgb(%1,%2,%3); border: 1px solid #888; }")
                                .arg(chosen.red())
                                .arg(chosen.green())
                                .arg(chosen.blue()));
      tractogram->reload_group_colours();
      window().updateGL();
    });
    hl->addWidget(swatch);

    rows_layout->insertWidget(rows_layout->count() - 1, row); // insert before stretch
    row_widgets.push_back(row);
  }
}

void TrackGroupOptions::multi_policy_changed(int index) {
  if (!tractogram)
    return;
  tractogram->group_multi_policy = (index == 0) ? GroupMultiPolicy::FirstMatch : GroupMultiPolicy::LastMatch;
  tractogram->reload_group_colours();
  window().updateGL();
}

void TrackGroupOptions::show_ungrouped_changed(int state) {
  if (!tractogram)
    return;
  tractogram->show_ungrouped = (state == Qt::Checked);
  tractogram->reload_group_colours();
  window().updateGL();
}

} // namespace MR::GUI::MRView::Tool
