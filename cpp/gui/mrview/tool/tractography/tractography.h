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

#include "color_button.h"
#include "mrview/adjust_button.h"
#include "mrview/combo_box_error.h"
#include "mrview/tool/base.h"
#include "mrview/tool/tractography/track_scalar_file.h"
#include "projection.h"

namespace MR::GUI::GL {
class Lighting;
} // namespace MR::GUI::GL

namespace MR::GUI {
class LightingDock;
} // namespace MR::GUI

namespace MR::GUI::MRView::Tool {

class Tractography : public Base {
  Q_OBJECT

public:
  class Model;

  Tractography(Dock *parent);

  virtual ~Tractography();

  void draw(const Projection &transform, bool is_3D, int axis, int slice) override;
  void draw_colourbars() override;
  size_t visible_number_colourbars() override;
  bool crop_to_slab() const { return (do_crop_to_slab && not_3D); }

  static void add_commandline_options(MR::App::OptionList &options);
  virtual bool process_commandline_option(const MR::App::ParsedOption &opt) override;

  QPushButton *hide_all_button;
  bool do_crop_to_slab;
  bool use_lighting;
  bool use_threshold_scalarfile;
  bool not_3D;
  float slab_thickness;
  float line_opacity;
  Model *tractogram_list_model;
  QListView *tractogram_list_view;

  GL::Lighting *lighting;

private slots:
  void tractogram_open_slot();
  void tractogram_close_slot();
  void toggle_shown_slot(const QModelIndex &, const QModelIndex &);
  void hide_all_slot();
  void on_slab_thickness_slot();
  void on_crop_to_slab_slot(bool is_checked);
  void on_use_lighting_slot(bool is_checked);
  void on_lighting_settings();
  void opacity_slot(int opacity);
  void line_thickness_slot(int thickness);
  void right_click_menu_slot(const QPoint &pos);
  void colour_track_by_direction_slot();
  void colour_track_by_ends_slot();
  void randomise_track_colour_slot();
  void set_track_colour_slot();
  void colour_by_scalar_file_slot();
  void colour_mode_selection_slot(int);
  void colour_button_slot();
  void geom_type_selection_slot(int);
  void thickness_modulation_selection_slot(int);
  void thickness_offset_slot();
  void thickness_scale_slot();
  void thickness_power_slot(int);
  void selection_changed_slot(const QItemSelection &, const QItemSelection &);

protected:
  AdjustButton *slab_entry;
  QMenu *track_option_menu;

  // Outer group wrapping the per-mechanism groups; shown while any tractogram is
  //   selected.
  QGroupBox *tractogram_options_groupbox;
  // Per-mechanism group boxes (geometry, colour, thresholding, thickness), shown
  //   only while a relevant tractogram selection makes each mechanism applicable.
  QGroupBox *geometry_groupbox;
  QGroupBox *colour_groupbox;
  QGroupBox *threshold_groupbox;
  QGroupBox *thickness_groupbox;

  ComboBoxWithErrorMsg *colour_combobox;
  QColorButton *colour_button;

  ComboBoxWithErrorMsg *geom_type_combobox;

  QLabel *thickness_label;
  QSlider *thickness_slider;

  //! Selects a sidecar source modulating streamline thickness.
  /*! Hidden (along with the thickness slider) for the "line" geometry; disabled
   *  whenever a single tractogram is not selected. */
  QLabel *thickness_modulation_label;
  QComboBox *thickness_modulation_combobox;
  //! Sidecar value mapped to zero thickness; shown only while modulation is active.
  QLabel *thickness_offset_label;
  AdjustButton *thickness_offset_button;
  //! Data value span mapping to the unmodulated thickness; shown while modulating.
  QLabel *thickness_scale_label;
  AdjustButton *thickness_scale_button;
  //! Whether the sidecar value is a cross-sectional area / volume (so thickness
  //!   scales as its square / cube root) rather than a thickness directly.
  QCheckBox *thickness_power_checkbox;

  TrackScalarFileOptions *scalar_file_options;
  LightingDock *lighting_dock;

  QGroupBox *slab_group_box;
  QGroupBox *lighting_group_box;
  QPushButton *lighting_button;

  QSlider *opacity_slider;

  //! Number of fixed colour-combo entries preceding any embedded-field entries
  //!   (Direction, Endpoints, Random, Manual, Sidecar file).
  static constexpr int num_fixed_colour_modes = 5;
  //! Colour the single selected tractogram by an embedded scalar field column.
  /*! \a entry indexes the selected tractogram's embedded_scalar_fields(). */
  void colour_by_embedded_field_slot(size_t entry);
  //! Rebuild the colour combo (embedded fields + trailing loaded external file)
  //!   and reflect the selection's colour mode, including the fixed-colour chooser
  //!   and colour-map button visibility.
  void update_colour_gui();
  //! Rebuild the thickness-modulation combo (None, Sidecar file, then each embedded
  //!   field) and reflect the selected tractogram's current source.
  void rebuild_thickness_modulation_combobox();

  void dropEvent(QDropEvent *event) override;
  void update_scalar_options();
  void add_tractogram(const std::vector<std::filesystem::path> &list);
  void select_last_added_tractogram();
  bool process_commandline_option_tsf_check_tracto_loaded();
  bool process_commandline_option_tsf_option(const MR::App::ParsedOption &, uint, std::vector<default_type> &range);
  void update_geometry_type_gui();
};

} // namespace MR::GUI::MRView::Tool
