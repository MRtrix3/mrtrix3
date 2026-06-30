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

#include <filesystem>

#include "mrview/adjust_button.h"
#include "mrview/colourmap_button.h"
#include "mrview/displayable.h"
#include "mrview/tool/base.h"
#include "mrview/tool/tractography/tractogram_enums.h"

namespace MR::GUI::MRView::Tool {
class Tractogram;
class Tractography;

//! \brief Controller for a tractogram's colour-map/scaling and threshold controls.
/*! No longer a group box of its own: it builds two embeddable container widgets —
 *  \c colour_scaling_widget (placed in the toolbar's "colour" group) and
 *  \c threshold_widget (placed in the "thresholding" group) — and owns their logic
 *  while the toolbar owns the per-mechanism group boxes. */
class TrackScalarFileOptions : public QObject, public ColourMapButtonObserver, public DisplayableVisitor {
  Q_OBJECT

public:
  TrackScalarFileOptions(Tractography *);
  virtual ~TrackScalarFileOptions() {}

  void set_tractogram(Tractogram *selected_tractogram);

  //! Scaling spin-boxes (embedded in the "colour" group; min/max for scalar colour).
  QWidget *colour_scaling_widget;
  //! Colour-map menu button; the toolbar places it beside the colour combo-box and
  //!   shows it only while colouring by scalar data.
  ColourMapButton *colourmap_button;
  //! Threshold source combo-box and lower/upper controls (in the "thresholding" group).
  QWidget *threshold_widget;

  void render_tractogram_colourbar(const Tool::Tractogram &) override;

  void update_UI();
  void set_scaling(default_type min, default_type max);
  //! Threshold using the scalar data already loaded for colouring (shared array).
  void set_threshold(default_type min, default_type max);
  void set_colourmap(int colourmap_index) { colourmap_button->set_colourmap_index(colourmap_index); }

  void selected_colourmap(size_t, const ColourMapButton &) override;
  void selected_custom_colour(const QColor &, const ColourMapButton &) override;
  void toggle_show_colour_bar(bool, const ColourMapButton &) override;
  void toggle_invert_colourmap(bool, const ColourMapButton &) override;
  void reset_colourmap(const ColourMapButton &) override;

public slots:
  bool open_intensity_track_scalar_file_slot();
  bool open_intensity_track_scalar_file_slot(const std::filesystem::path &);

private slots:
  void on_set_scaling_slot();
  bool threshold_scalar_file_slot(int);
  void threshold_lower_changed(int unused);
  void threshold_upper_changed(int unused);
  void threshold_lower_value_changed();
  void threshold_upper_value_changed();

protected:
  //! Number of fixed threshold-combo entries preceding any embedded-field
  //!   entries (None, Sidecar file).
  static constexpr int num_fixed_threshold_modes = 2;

  Tractography *tool;
  Tractogram *tractogram;
  AdjustButton *max_entry, *min_entry;
  QComboBox *threshold_file_combobox;
  AdjustButton *threshold_lower, *threshold_upper;
  QCheckBox *threshold_upper_box, *threshold_lower_box;

private:
  // Required since this no longer derives from Tool::Base
  Window &window() const { return *Window::main; }
};

} // namespace MR::GUI::MRView::Tool
