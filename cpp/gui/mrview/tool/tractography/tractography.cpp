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

#include "mrview/tool/tractography/tractography.h"

#include "magic_enum/magic_enum.hpp"
#include <array>

#include "dialog/file.h"
#include "enum.h"
#include "file/config.h"
#include "gui.h"
#include "lighting_dock.h"
#include "mrtrix.h"
#include "mrview/qthelpers.h"
#include "mrview/tool/list_model_base.h"
#include "mrview/tool/tractography/track_scalar_file.h"
#include "mrview/tool/tractography/tractogram.h"
#include "mrview/tool/tractography/tractogram_enums.h"
#include "mrview/window.h"
#include "opengl/lighting.h"

namespace MR::GUI::MRView::Tool {

class Tractography::Model : public ListModelBase {

public:
  Model(QObject *parent) : ListModelBase(parent) {}

  void add_items(const std::vector<std::filesystem::path> &filepaths, Tractography &tractography_tool) {

    for (size_t i = 0; i < filepaths.size(); ++i) {
      Tractogram *tractogram = new Tractogram(tractography_tool, filepaths[i]);
      try {
        tractogram->load_tracks();
        beginInsertRows(QModelIndex(), items.size(), items.size() + 1);
        items.push_back(std::unique_ptr<Displayable>(tractogram));
        endInsertRows();
      } catch (Exception &e) {
        delete tractogram;
        e.display();
      }
    }
  }

  Tractogram *get_tractogram(QModelIndex &index) { return dynamic_cast<Tractogram *>(items[index.row()].get()); }
};

Tractography::Tractography(Dock *parent)
    : Base(parent),
      do_crop_to_slab(true),
      use_lighting(false),
      not_3D(true),
      line_opacity(1.0),
      scalar_file_options(nullptr),
      lighting_dock(nullptr) {

  float voxel_size;
  if (window().image()) {
    voxel_size = (window().image()->header().spacing(0) + window().image()->header().spacing(1) +
                  window().image()->header().spacing(2)) /
                 3.0f;
  } else {
    voxel_size = 2.5;
  }

  slab_thickness = 2 * voxel_size;

  VBoxLayout *main_box = new VBoxLayout(this);
  HBoxLayout *hlayout = new HBoxLayout;
  hlayout->setContentsMargins(0, 0, 0, 0);
  hlayout->setSpacing(0);

  QPushButton *button = new QPushButton(this);
  button->setToolTip(tr("Open tractogram"));
  button->setIcon(QIcon(":/open.svg"));
  connect(button, SIGNAL(clicked()), this, SLOT(tractogram_open_slot()));
  hlayout->addWidget(button, 1);

  button = new QPushButton(this);
  button->setToolTip(tr("Close tractogram"));
  button->setIcon(QIcon(":/close.svg"));
  connect(button, SIGNAL(clicked()), this, SLOT(tractogram_close_slot()));
  hlayout->addWidget(button, 1);

  hide_all_button = new QPushButton(this);
  hide_all_button->setToolTip(tr("Hide all tractograms"));
  hide_all_button->setIcon(QIcon(":/hide.svg"));
  hide_all_button->setCheckable(true);
  connect(hide_all_button, SIGNAL(clicked()), this, SLOT(hide_all_slot()));
  hlayout->addWidget(hide_all_button, 1);

  main_box->addLayout(hlayout, 0);

  tractogram_list_view = new QListView(this);
  tractogram_list_view->setSelectionMode(QAbstractItemView::ExtendedSelection);
  tractogram_list_view->setDragEnabled(true);
  tractogram_list_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  tractogram_list_view->setTextElideMode(Qt::ElideLeft);
  tractogram_list_view->viewport()->setAcceptDrops(true);
  tractogram_list_view->setDropIndicatorShown(true);

  tractogram_list_model = new Model(this);
  tractogram_list_view->setModel(tractogram_list_model);

  connect(tractogram_list_model,
          SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
          this,
          SLOT(toggle_shown_slot(const QModelIndex &, const QModelIndex &)));

  connect(tractogram_list_view->selectionModel(),
          SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
          SLOT(selection_changed_slot(const QItemSelection &, const QItemSelection &)));

  tractogram_list_view->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(tractogram_list_view,
          SIGNAL(customContextMenuRequested(const QPoint &)),
          this,
          SLOT(right_click_menu_slot(const QPoint &)));

  main_box->addWidget(tractogram_list_view, 1);

  // Builds the colour-map/scaling and threshold widgets embedded into the colour
  //   and thresholding groups below.
  scalar_file_options = new TrackScalarFileOptions(this);

  // Outer group collecting the per-mechanism tractogram groups.
  tractogram_options_groupbox = new QGroupBox("Tractogram-specific options");
  VBoxLayout *tractogram_options_layout = new VBoxLayout;
  tractogram_options_layout->setContentsMargins(0, 0, 0, 0);
  tractogram_options_layout->setSpacing(0);
  tractogram_options_groupbox->setLayout(tractogram_options_layout);

  // --- GEOMETRY group ---
  geometry_groupbox = new QGroupBox("geometry");
  HBoxLayout *geometry_layout = new HBoxLayout;
  geometry_layout->setContentsMargins(0, 0, 0, 0);
  geometry_layout->setSpacing(0);
  geometry_groupbox->setLayout(geometry_layout);

  geom_type_combobox = new ComboBoxWithErrorMsg(this, "(variable)");
  geom_type_combobox->setToolTip(tr("Set the tractogram geometry type"));
  // Insert combobox entries in the same order as the enumerators are declared,
  //   so that each entry's index matches its TrackGeometryType underlying value.
  for (const auto &geom_type_name : magic_enum::enum_names<TrackGeometryType>())
    geom_type_combobox->addItem(qstr(geom_type_name));
  connect(geom_type_combobox, SIGNAL(activated(int)), this, SLOT(geom_type_selection_slot(int)));
  geometry_layout->addWidget(geom_type_combobox);

  tractogram_options_layout->addWidget(geometry_groupbox);

  // --- COLOUR group (mode selection + colour-map/scaling for scalar colouring) ---
  colour_groupbox = new QGroupBox("colour");
  VBoxLayout *colour_box = new VBoxLayout;
  colour_box->setContentsMargins(0, 0, 0, 0);
  colour_box->setSpacing(0);
  colour_groupbox->setLayout(colour_box);

  hlayout = new HBoxLayout;
  hlayout->setContentsMargins(0, 0, 0, 0);
  hlayout->setSpacing(0);
  hlayout->addWidget(new QLabel("color"));

  colour_combobox = new ComboBoxWithErrorMsg(this, "(variable)");
  colour_combobox->setToolTip(tr("Set how this tractogram will be colored"));
  colour_combobox->addItem("Direction");
  colour_combobox->addItem("Endpoints");
  colour_combobox->addItem("Random");
  colour_combobox->addItem("Manual");
  colour_combobox->addItem("Sidecar file");
  colour_combobox->setEnabled(false);
  connect(colour_combobox, SIGNAL(activated(int)), this, SLOT(colour_mode_selection_slot(int)));
  hlayout->addWidget(colour_combobox);

  // The fixed-colour chooser and the colour-map menu are mutually exclusive (Manual
  //   vs scalar colouring); each is shown only when its mode is active.
  colour_button = new QColorButton;
  colour_button->setToolTip(tr("Set the fixed colour to use for all tracks"));
  colour_button->setVisible(false);
  connect(colour_button, SIGNAL(clicked()), this, SLOT(colour_button_slot()));
  hlayout->addWidget(colour_button);

  scalar_file_options->colourmap_button->setVisible(false);
  hlayout->addWidget(scalar_file_options->colourmap_button);

  colour_box->addLayout(hlayout);
  colour_box->addWidget(scalar_file_options->colour_scaling_widget);

  tractogram_options_layout->addWidget(colour_groupbox);

  // --- THRESHOLDING group ---
  threshold_groupbox = new QGroupBox("thresholding");
  VBoxLayout *threshold_box = new VBoxLayout;
  threshold_box->setContentsMargins(0, 0, 0, 0);
  threshold_box->setSpacing(0);
  threshold_groupbox->setLayout(threshold_box);
  threshold_box->addWidget(scalar_file_options->threshold_widget);

  tractogram_options_layout->addWidget(threshold_groupbox);

  // --- THICKNESS group (global slider + modulation source + offset/scale) ---
  thickness_groupbox = new QGroupBox("thickness");
  VBoxLayout *thickness_box = new VBoxLayout;
  thickness_box->setContentsMargins(0, 0, 0, 0);
  thickness_box->setSpacing(0);
  thickness_groupbox->setLayout(thickness_box);

  hlayout = new HBoxLayout;
  hlayout->setContentsMargins(0, 0, 0, 0);
  hlayout->setSpacing(0);
  thickness_label = new QLabel("thickness");
  hlayout->addWidget(thickness_label);
  thickness_slider = new QSlider(Qt::Horizontal);
  thickness_slider->setRange(-1000, 1000);
  thickness_slider->setSliderPosition(0);
  connect(thickness_slider, SIGNAL(valueChanged(int)), this, SLOT(line_thickness_slot(int)));
  hlayout->addWidget(thickness_slider);
  thickness_box->addLayout(hlayout);

  hlayout = new HBoxLayout;
  hlayout->setContentsMargins(0, 0, 0, 0);
  hlayout->setSpacing(0);
  thickness_modulation_label = new QLabel("modulate");
  hlayout->addWidget(thickness_modulation_label);
  thickness_modulation_combobox = new QComboBox(this);
  thickness_modulation_combobox->setToolTip(
      tr("Modulate streamline thickness by tractogram sidecar data"
         " (pseudo-tube width scales as its square root, point radius as its cube root)"));
  thickness_modulation_combobox->addItem("None");
  thickness_modulation_combobox->addItem("Sidecar file");
  connect(thickness_modulation_combobox, SIGNAL(activated(int)), this, SLOT(thickness_modulation_selection_slot(int)));
  hlayout->addWidget(thickness_modulation_combobox);
  thickness_box->addLayout(hlayout);

  hlayout = new HBoxLayout;
  hlayout->setContentsMargins(0, 0, 0, 0);
  hlayout->setSpacing(0);
  thickness_offset_label = new QLabel("offset");
  hlayout->addWidget(thickness_offset_label);
  thickness_offset_button = new AdjustButton(this);
  thickness_offset_button->setToolTip(
      tr("Sidecar value mapped to zero thickness; values below it render at zero thickness"));
  thickness_offset_button->setValue(0.0);
  connect(thickness_offset_button, SIGNAL(valueChanged()), this, SLOT(thickness_offset_slot()));
  hlayout->addWidget(thickness_offset_button);
  // Separate the offset control from the scale control so they don't run together.
  hlayout->addSpacing(8);
  thickness_scale_label = new QLabel("scale");
  hlayout->addWidget(thickness_scale_label);
  thickness_scale_button = new AdjustButton(this);
  thickness_scale_button->setToolTip(
      tr("Sidecar value span mapped to the unmodulated thickness"
         " (auto-populated with the data mean so typical thickness is stable across data)"));
  thickness_scale_button->setValue(1.0);
  connect(thickness_scale_button, SIGNAL(valueChanged()), this, SLOT(thickness_scale_slot()));
  hlayout->addWidget(thickness_scale_button);
  thickness_box->addLayout(hlayout);

  hlayout = new HBoxLayout;
  hlayout->setContentsMargins(0, 0, 0, 0);
  hlayout->setSpacing(0);
  // Label text is set per-geometry in update_geometry_type_gui().
  thickness_power_checkbox = new QCheckBox("value is cylinder area");
  thickness_power_checkbox->setToolTip(
      tr("When checked, the sidecar value is treated as cross-sectional area (pseudo-tubes) or"
         " volume (points), so thickness scales as its square root or cube root respectively;"
         " when unchecked, thickness is proportional to the value directly"));
  thickness_power_checkbox->setChecked(true);
  connect(thickness_power_checkbox, SIGNAL(stateChanged(int)), this, SLOT(thickness_power_slot(int)));
  hlayout->addWidget(thickness_power_checkbox);
  thickness_box->addLayout(hlayout);

  tractogram_options_layout->addWidget(thickness_groupbox);

  main_box->addWidget(tractogram_options_groupbox);

  QGroupBox *general_groupbox = new QGroupBox("General options");
  GridLayout *general_opt_grid = new GridLayout;
  general_opt_grid->setContentsMargins(0, 0, 0, 0);
  general_opt_grid->setSpacing(0);

  general_groupbox->setLayout(general_opt_grid);

  opacity_slider = new QSlider(Qt::Horizontal);
  opacity_slider->setRange(1, 1000);
  opacity_slider->setSliderPosition(1000);
  connect(opacity_slider, SIGNAL(valueChanged(int)), this, SLOT(opacity_slot(int)));
  general_opt_grid->addWidget(new QLabel("opacity"), 0, 0);
  general_opt_grid->addWidget(opacity_slider, 0, 1);

  slab_group_box = new QGroupBox(tr("crop to slab"));
  slab_group_box->setCheckable(true);
  slab_group_box->setChecked(true);
  general_opt_grid->addWidget(slab_group_box, 4, 0, 1, 2);

  connect(slab_group_box, SIGNAL(clicked(bool)), this, SLOT(on_crop_to_slab_slot(bool)));

  GridLayout *slab_layout = new GridLayout;
  slab_group_box->setLayout(slab_layout);
  slab_layout->addWidget(new QLabel("thickness (mm)"), 0, 0);
  slab_entry = new AdjustButton(this, 0.1);
  slab_entry->setValue(slab_thickness);
  slab_entry->setMin(0.0);
  connect(slab_entry, SIGNAL(valueChanged()), this, SLOT(on_slab_thickness_slot()));
  slab_layout->addWidget(slab_entry, 0, 1);

  lighting_group_box = new QGroupBox(tr("use lighting"));
  lighting_group_box->setCheckable(true);
  lighting_group_box->setChecked(false);
  general_opt_grid->addWidget(lighting_group_box, 5, 0, 1, 2);

  connect(lighting_group_box, SIGNAL(clicked(bool)), this, SLOT(on_use_lighting_slot(bool)));

  VBoxLayout *lighting_layout = new VBoxLayout(lighting_group_box);
  lighting_button = new QPushButton("Track lighting...");
  lighting_button->setIcon(QIcon(":/light.svg"));
  connect(lighting_button, SIGNAL(clicked()), this, SLOT(on_lighting_settings()));
  lighting_layout->addWidget(lighting_button);

  main_box->addWidget(general_groupbox, 0);

  lighting = new GL::Lighting(parent);
  lighting->diffuse = 0.8;
  lighting->shine = 5.0;
  connect(lighting, SIGNAL(changed()), SLOT(hide_all_slot()));

  QAction *action;
  track_option_menu = new QMenu();
  action = new QAction("&Colour by direction", this);
  connect(action, SIGNAL(triggered()), this, SLOT(colour_track_by_direction_slot()));
  track_option_menu->addAction(action);
  action = new QAction("&Colour by track ends", this);
  connect(action, SIGNAL(triggered()), this, SLOT(colour_track_by_ends_slot()));
  track_option_menu->addAction(action);
  action = new QAction("&Randomise colour", this);
  connect(action, SIGNAL(triggered()), this, SLOT(randomise_track_colour_slot()));
  track_option_menu->addAction(action);
  action = new QAction("&Set colour", this);
  connect(action, SIGNAL(triggered()), this, SLOT(set_track_colour_slot()));
  track_option_menu->addAction(action);
  action = new QAction("&Colour by (track) scalar file", this);
  connect(action, SIGNAL(triggered()), this, SLOT(colour_by_scalar_file_slot()));
  track_option_menu->addAction(action);

  Tractogram::default_tract_geom = TrackGeometryType::Pseudotubes;
  // CONF option: MRViewDefaultTractGeomType
  // CONF default: Pseudotubes
  // CONF The default geometry type used to render tractograms.
  // CONF Options are Pseudotubes, Lines or Points
  auto default_geom_type_config = File::Config::get("MRViewDefaultTractGeomType");
  if (default_geom_type_config.has_value()) {
    try {
      Tractogram::default_tract_geom = MR::Enum::from_name<TrackGeometryType>(default_geom_type_config.value());
    } catch (Exception &e) {
      e.display();
    }
  }
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  geom_type_combobox->setCurrentIndex(*magic_enum::enum_index(Tractogram::default_tract_geom));

  // CONF option: MRViewTractogramHalfPrecisionGPU
  // CONF default: false (0)
  // CONF Force mrview to upload tractogram vertex data to the GPU in IEEE
  // CONF half-precision (16-bit float) regardless of the input file's on-disk
  // CONF vertex datatype, halving the vertex buffer footprint at the cost of
  // CONF positional precision (~0.06 mm near +/-100 mm). This is a rendering
  // CONF representation only and never feeds quantitative paths. When unset,
  // CONF mrview honours the on-disk datatype (Float16 -> half; Float32 -> float;
  // CONF Float64 -> float).
  Tractogram::force_half_precision_gpu = File::Config::get_bool("MRViewTractogramHalfPrecisionGPU", false);

  // In the instance where pseudotubes are _not_ the default, enable lighting by default
  if (Tractogram::default_tract_geom != TrackGeometryType::Pseudotubes) {
    use_lighting = true;
    lighting_group_box->setChecked(true);
  }

  update_geometry_type_gui();
}

Tractography::~Tractography() {}

void Tractography::draw(const Projection &transform, bool is_3D, int, int) {
  GL::assert_context_is_current();
  not_3D = !is_3D;
  for (int i = 0; i < tractogram_list_model->rowCount(); ++i) {
    Tractogram *tractogram = dynamic_cast<Tractogram *>(tractogram_list_model->items[i].get());
    if (tractogram->show && !hide_all_button->isChecked())
      tractogram->render(transform);
  }
  GL::assert_context_is_current();
}

void Tractography::draw_colourbars() {
  if (hide_all_button->isChecked())
    return;

  for (int i = 0; i < tractogram_list_model->rowCount(); ++i) {
    Tractogram *tractogram = dynamic_cast<Tractogram *>(tractogram_list_model->items[i].get());
    if (tractogram->show && tractogram->get_color_type() == TrackColourType::ScalarFile &&
        (!tractogram->intensity_scalar_path.empty() || tractogram->intensity_embedded_field.has_value()))
      tractogram->request_render_colourbar(*scalar_file_options);
  }
}

size_t Tractography::visible_number_colourbars() {
  size_t total_visible(0);

  if (!hide_all_button->isChecked()) {
    for (size_t i = 0, N = tractogram_list_model->rowCount(); i < N; ++i) {
      Tractogram *tractogram = dynamic_cast<Tractogram *>(tractogram_list_model->items[i].get());
      if (tractogram->show && tractogram->get_color_type() == TrackColourType::ScalarFile &&
          (!tractogram->intensity_scalar_path.empty() || tractogram->intensity_embedded_field.has_value()))
        total_visible += 1;
    }
  }

  return total_visible;
}

void Tractography::tractogram_open_slot() {
  auto load_paths =
      Dialog::File::input_filepaths(this, "Select tractograms to open", "Tractograms (*.tck)", current_folder);
  if (!load_paths.empty())
    current_folder = load_paths.last_directory;
  add_tractogram(load_paths.multi_selection);
}

void Tractography::add_tractogram(const std::vector<std::filesystem::path> &list) {
  if (list.empty())
    return;
  try {
    tractogram_list_model->add_items(list, *this);
    select_last_added_tractogram();
  } catch (Exception &E) {
    E.display();
  }
}

void Tractography::dropEvent(QDropEvent *event) {
  static constexpr int max_files = 32;

  const QMimeData *mimeData = event->mimeData();
  if (mimeData->hasUrls()) {
    std::vector<std::filesystem::path> list;
    QList<QUrl> urlList = mimeData->urls();
    for (int i = 0; i < urlList.size() && i < max_files; ++i) {
      list.push_back(QtHelpers::url_to_fspath(urlList.at(i)));
    }
    try {
      tractogram_list_model->add_items(list, *this);
      window().updateGL();
    } catch (Exception &e) {
      e.display();
    }
    event->acceptProposedAction();
  }
}

void Tractography::tractogram_close_slot() {
  GL::Context::Grab context;
  QModelIndexList indexes = tractogram_list_view->selectionModel()->selectedIndexes();
  while (!indexes.empty()) {
    tractogram_list_model->remove_item(indexes.first());
    indexes = tractogram_list_view->selectionModel()->selectedIndexes();
  }
  scalar_file_options->set_tractogram(nullptr);
  scalar_file_options->update_UI();
  window().updateGL();
}

void Tractography::toggle_shown_slot(const QModelIndex &index, const QModelIndex &index2) {
  if (index.row() == index2.row()) {
    tractogram_list_view->setCurrentIndex(index);
  } else {
    for (size_t i = 0; i < tractogram_list_model->items.size(); ++i) {
      if (tractogram_list_model->items[i]->show) {
        tractogram_list_view->setCurrentIndex(tractogram_list_model->index(i, 0));
        break;
      }
    }
  }
  window().updateGL();
}

void Tractography::hide_all_slot() { window().updateGL(); }

void Tractography::on_crop_to_slab_slot(bool is_checked) {
  do_crop_to_slab = is_checked;

  for (size_t i = 0, N = tractogram_list_model->rowCount(); i < N; ++i) {
    Tractogram *tractogram = dynamic_cast<Tractogram *>(tractogram_list_model->items[i].get());
    tractogram->should_update_lod = true;
  }

  window().updateGL();
}

void Tractography::on_use_lighting_slot(bool is_checked) {
  use_lighting = is_checked;
  window().updateGL();
}

void Tractography::on_lighting_settings() {
  if (!lighting_dock) {
    lighting_dock = new LightingDock("Tractogram lighting", *lighting);
    window().addDockWidget(Qt::RightDockWidgetArea, lighting_dock);
  }
  lighting_dock->show();
}

void Tractography::on_slab_thickness_slot() {
  slab_thickness = slab_entry->value();
  window().updateGL();
}

void Tractography::opacity_slot(int opacity) {
  line_opacity = Math::pow2(static_cast<float>(opacity)) / 1.0e6f;
  window().updateGL();
}

void Tractography::line_thickness_slot(int thickness) {
  QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
  for (int i = 0; i < indices.size(); ++i) {
    Tractogram *tractogram = tractogram_list_model->get_tractogram(indices[i]);
    tractogram->line_thickness = thickness;
    tractogram->should_update_lod = true;
  }

  window().updateGL();
}

void Tractography::right_click_menu_slot(const QPoint &pos) {
  QModelIndex index = tractogram_list_view->indexAt(pos);
  if (index.isValid()) {
    QPoint globalPos = tractogram_list_view->mapToGlobal(pos);
    tractogram_list_view->selectionModel()->select(index, QItemSelectionModel::Select);
    track_option_menu->exec(globalPos);
  }
}

void Tractography::colour_track_by_direction_slot() {
  QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
  for (int i = 0; i < indices.size(); ++i)
    tractogram_list_model->get_tractogram(indices[i])->set_color_type(TrackColourType::Direction);
  update_scalar_options();
  update_colour_gui();
  window().updateGL();
}

void Tractography::colour_track_by_ends_slot() {
  QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
  for (int i = 0; i < indices.size(); ++i) {
    Tractogram *tractogram = tractogram_list_model->get_tractogram(indices[i]);
    tractogram->set_color_type(TrackColourType::Ends);
    tractogram->load_end_colours();
  }
  update_scalar_options();
  update_colour_gui();
  window().updateGL();
}

void Tractography::randomise_track_colour_slot() {
  QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
  for (int i = 0; i < indices.size(); ++i) {
    Tractogram *tractogram = tractogram_list_model->get_tractogram(indices[i]);
    std::array<float, 3> colour;
    Math::RNG::Uniform<float> rng;
    do {
      colour[0] = rng();
      colour[1] = rng();
      colour[2] = rng();
    } while (colour[0] < 0.5F && colour[1] < 0.5F && colour[2] < 0.5F);
    tractogram->set_color_type(TrackColourType::Manual);
    QColor c(colour[0] * 255.0F, colour[1] * 255.0F, colour[2] * 255.0F);
    tractogram->set_colour(c);
  }
  update_scalar_options();
  update_colour_gui();
  window().updateGL();
}

void Tractography::set_track_colour_slot() {
  QColor color;
  color = QColorDialog::getColor(Qt::red, this, "Select Color", QColorDialog::DontUseNativeDialog);
  if (color.isValid()) {
    QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
    for (int i = 0; i < indices.size(); ++i) {
      Tractogram *tractogram = tractogram_list_model->get_tractogram(indices[i]);
      tractogram->set_color_type(TrackColourType::Manual);
      tractogram->set_colour(color);
    }
    update_scalar_options();
    update_colour_gui();
  }
  window().updateGL();
}

void Tractography::colour_by_scalar_file_slot() {
  QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
  if (indices.size() != 1) {
    // User may have accessed this from the context menu
    QMessageBox::warning(QApplication::activeWindow(),
                         tr("Tractogram colour error"),
                         tr("Cannot set multiple tractograms to use the same file for streamline colouring"),
                         QMessageBox::Ok,
                         QMessageBox::Ok);
    update_colour_gui();
    return;
  }

  Tractogram *tractogram = tractogram_list_model->get_tractogram(indices[0]);
  scalar_file_options->set_tractogram(tractogram);
  // Always prompt for a (possibly different) sidecar file; on success the helper
  //   loads it and sets the colour type to ScalarFile. On cancel the colour state
  //   is unchanged and update_colour_gui() restores the combo.
  scalar_file_options->open_intensity_track_scalar_file_slot();
  update_scalar_options();
  update_colour_gui();
  window().updateGL();
}

void Tractography::colour_mode_selection_slot(int) {
  const int index = colour_combobox->currentIndex();
  switch (index) {
  case 0:
    colour_track_by_direction_slot();
    return;
  case 1:
    colour_track_by_ends_slot();
    return;
  case 2:
    randomise_track_colour_slot();
    return;
  case 3:
    set_track_colour_slot();
    return;
  case 4:
    colour_by_scalar_file_slot();
    return;
  default:
    break;
  }
  // Indices beyond the fixed modes are an embedded-field entry, the trailing
  //   loaded-file label (that source is already active, so a no-op), or the
  //   trailing "(variable)" error entry (also a no-op).
  QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
  if (indices.size() == 1) {
    const Tractogram *tractogram = tractogram_list_model->get_tractogram(indices[0]);
    const size_t entry = static_cast<size_t>(index - num_fixed_colour_modes);
    if (entry < tractogram->embedded_scalar_fields().size())
      colour_by_embedded_field_slot(entry);
  }
}

void Tractography::colour_by_embedded_field_slot(const size_t entry) {
  QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
  if (indices.size() != 1)
    return;
  Tractogram *tractogram = tractogram_list_model->get_tractogram(indices[0]);
  try {
    tractogram->load_intensity_embedded_scalars(entry);
    tractogram->set_color_type(TrackColourType::ScalarFile);
  } catch (Exception &e) {
    e.display();
    update_colour_gui();
    return;
  }
  scalar_file_options->set_tractogram(tractogram);
  update_scalar_options();
  update_colour_gui();
  window().updateGL();
}

void Tractography::update_colour_gui() {
  QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
  const bool any = !indices.empty();
  const bool single = indices.size() == 1;

  colour_combobox->blockSignals(true);
  colour_combobox->clearError();
  colour_combobox->setToolTip(tr("Set how this tractogram will be colored"));
  // Strip previous embedded-field and trailing loaded-file entries.
  while (colour_combobox->count() > num_fixed_colour_modes)
    colour_combobox->removeItem(colour_combobox->count() - 1);

  colour_combobox->setEnabled(any);
  if (!any) {
    colour_button->setVisible(false);
    scalar_file_options->colourmap_button->setVisible(false);
    colour_combobox->blockSignals(false);
    return;
  }

  const Tractogram *first = tractogram_list_model->get_tractogram(indices[0]);
  const TrackColourType color_type = first->get_color_type();
  bool consistent = true;
  for (int i = 1; i != indices.size(); ++i)
    if (tractogram_list_model->get_tractogram(indices[i])->get_color_type() != color_type)
      consistent = false;

  // The embedded-field entries (and the trailing loaded external file) are
  //   tractogram-specific, so only listed for a single selection.
  if (single)
    for (const auto &field : first->embedded_scalar_fields())
      colour_combobox->addItem(qstr(field.label()));

  // The fixed-colour chooser is shown only for Manual; the colour-map menu only
  //   for scalar colouring; the two modes are mutually exclusive.
  colour_button->setVisible(consistent && color_type == TrackColourType::Manual);
  scalar_file_options->colourmap_button->setVisible(single && color_type == TrackColourType::ScalarFile);

  if (!consistent) {
    colour_combobox->setError();
    colour_combobox->blockSignals(false);
    return;
  }

  switch (color_type) {
  case TrackColourType::Direction:
    colour_combobox->setCurrentIndex(0);
    break;
  case TrackColourType::Ends:
    colour_combobox->setCurrentIndex(1);
    break;
  case TrackColourType::Manual:
    colour_combobox->setCurrentIndex(3);
    colour_button->setColor(QColor(first->colour[0], first->colour[1], first->colour[2]));
    break;
  case TrackColourType::ScalarFile:
    if (single && first->intensity_embedded_field.has_value()) {
      colour_combobox->setCurrentIndex(num_fixed_colour_modes + static_cast<int>(*first->intensity_embedded_field));
    } else if (single && !first->intensity_scalar_path.empty()) {
      // Show the loaded external file as the final combo entry and select it; the
      //   "Sidecar file" entry above stays available to choose a different file.
      colour_combobox->addItem(qstr(shorten(first->intensity_scalar_path.filename().string(), 35, 0)));
      colour_combobox->setToolTip(qstr(first->intensity_scalar_path.string()));
      colour_combobox->setCurrentIndex(colour_combobox->count() - 1);
    } else {
      colour_combobox->setCurrentIndex(4);
    }
    break;
  }
  colour_combobox->blockSignals(false);
}

void Tractography::colour_button_slot() {
  // Button brings up its own colour prompt; if set_track_colour_slot()
  //   were to be called, this would present its own selection prompt
  // Need to instead set the colours here explicitly
  const QColor color = colour_button->color();
  if (color.isValid()) {
    QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
    for (int i = 0; i < indices.size(); ++i)
      tractogram_list_model->get_tractogram(indices[i])->set_colour(color);
    update_colour_gui();
    window().updateGL();
  }
}

void Tractography::geom_type_selection_slot(int selected_index) {
  // Combo box shows the "(variable)" message, and the user has
  //   re-clicked on it -> nothing to do
  if (selected_index == 3)
    return;

  const TrackGeometryType geom_type = magic_enum::enum_value<TrackGeometryType>(static_cast<size_t>(selected_index));

  QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
  for (int i = 0; i < indices.size(); ++i)
    tractogram_list_model->get_tractogram(indices[i])->set_geometry_type(geom_type);

  update_geometry_type_gui();

  window().updateGL();
}

void Tractography::thickness_modulation_selection_slot(int) {
  QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
  // The control is only enabled for a single selection (an embedded field is
  //   tractogram-specific); restore the displayed state and bail otherwise.
  if (indices.size() != 1) {
    rebuild_thickness_modulation_combobox();
    return;
  }

  Tractogram *tractogram = tractogram_list_model->get_tractogram(indices[0]);
  const int index = thickness_modulation_combobox->currentIndex();

  switch (index) {
  case 0: // None: leave every streamline at the global thickness.
    tractogram->set_thickness_type(TrackThicknessType::None);
    tractogram->erase_thickness_scalar_data();
    break;
  case 1: { // External scalar file (.tsf per-vertex, or a per-streamline text vector).
    std::filesystem::path file_path;
    auto load_paths = Dialog::File::input_filepath(
        this, "Select scalar text file or Track Scalar file (.tsf) for thickness modulation", "", current_folder);
    if (!load_paths.empty()) {
      current_folder = load_paths.last_directory;
      try {
        file_path = load_paths.single_selection;
        tractogram->load_thickness_track_scalars(load_paths.single_selection);
        tractogram->set_thickness_type(TrackThicknessType::SidecarData);
      } catch (Exception &E) {
        E.display();
        file_path.clear();
      }
    }
    if (file_path.empty()) {
      // Cancelled or failed: restore the combo to the unchanged state.
      rebuild_thickness_modulation_combobox();
      return;
    }
  } break;
  default: {
    // Combo layout: [None, File, embedded fields..., (loaded file)]. Embedded
    //   entries carry their embedded_scalar_fields() index as item user data; the
    //   trailing loaded-file label carries none and is a no-op when re-selected.
    const QVariant data = thickness_modulation_combobox->itemData(index);
    if (data.isValid()) {
      try {
        tractogram->load_thickness_embedded_scalars(static_cast<size_t>(data.toInt()));
        tractogram->set_thickness_type(TrackThicknessType::SidecarData);
      } catch (Exception &E) {
        E.display();
        rebuild_thickness_modulation_combobox();
        return;
      }
    }
  } break;
  }

  rebuild_thickness_modulation_combobox();
  update_geometry_type_gui();
  window().updateGL();
}

void Tractography::thickness_offset_slot() {
  QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
  if (indices.size() != 1)
    return;
  tractogram_list_model->get_tractogram(indices[0])->thickness_offset = thickness_offset_button->value();
  window().updateGL();
}

void Tractography::thickness_scale_slot() {
  QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
  if (indices.size() != 1)
    return;
  tractogram_list_model->get_tractogram(indices[0])->thickness_scale = thickness_scale_button->value();
  window().updateGL();
}

void Tractography::thickness_power_slot(int) {
  QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
  if (indices.size() != 1)
    return;
  tractogram_list_model->get_tractogram(indices[0])->thickness_power_transform = thickness_power_checkbox->isChecked();
  window().updateGL();
}

void Tractography::rebuild_thickness_modulation_combobox() {
  thickness_modulation_combobox->blockSignals(true);
  thickness_modulation_combobox->clear();
  thickness_modulation_combobox->setToolTip(QString());
  thickness_modulation_combobox->addItem("None");
  thickness_modulation_combobox->addItem("Sidecar file");

  QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
  if (indices.size() == 1) {
    const Tractogram *tractogram = tractogram_list_model->get_tractogram(indices[0]);
    // Append one entry per embedded field column (any dpv or dps), recording the
    //   field's index into embedded_scalar_fields() as the item's user data.
    const std::vector<EmbeddedScalarField> &fields = tractogram->embedded_scalar_fields();
    for (size_t i = 0; i != fields.size(); ++i)
      thickness_modulation_combobox->addItem(qstr(fields[i].label()), QVariant(static_cast<int>(i)));

    switch (tractogram->get_thickness_type()) {
    case TrackThicknessType::None:
      thickness_modulation_combobox->setCurrentIndex(0);
      break;
    case TrackThicknessType::SidecarData:
      if (tractogram->thickness_embedded_field.has_value()) {
        const int target = static_cast<int>(*tractogram->thickness_embedded_field);
        int found = 0;
        for (int j = 0; j != thickness_modulation_combobox->count(); ++j) {
          const QVariant data = thickness_modulation_combobox->itemData(j);
          if (data.isValid() && data.toInt() == target) {
            found = j;
            break;
          }
        }
        thickness_modulation_combobox->setToolTip(
            qstr("Embedded field: " + fields[static_cast<size_t>(target)].label()));
        thickness_modulation_combobox->setCurrentIndex(found);
      } else {
        assert(!tractogram->thickness_scalar_path.empty());
        thickness_modulation_combobox->addItem(
            qstr(shorten(tractogram->thickness_scalar_path.filename().string(), 35, 0)));
        thickness_modulation_combobox->setToolTip(qstr(tractogram->thickness_scalar_path.string()));
        thickness_modulation_combobox->setCurrentIndex(thickness_modulation_combobox->count() - 1);
      }
      break;
    }
  }
  thickness_modulation_combobox->blockSignals(false);
}

void Tractography::selection_changed_slot(const QItemSelection &, const QItemSelection &) {
  update_scalar_options();
  update_geometry_type_gui();
  update_colour_gui();
  rebuild_thickness_modulation_combobox();

  QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
  if (indices.empty())
    return;

  const Tractogram *first_tractogram = tractogram_list_model->get_tractogram(indices[0]);

  TrackGeometryType geom_type = first_tractogram->get_geometry_type();
  bool geometry_type_consistent = true;
  float mean_thickness = first_tractogram->line_thickness;
  for (int i = 1; i != indices.size(); ++i) {
    const Tractogram *tractogram = tractogram_list_model->get_tractogram(indices[i]);
    if (tractogram->get_geometry_type() != geom_type)
      geometry_type_consistent = false;
    mean_thickness += tractogram->line_thickness;
  }

  if (geometry_type_consistent) {
    geom_type_combobox->blockSignals(true);
    switch (geom_type) {
    case TrackGeometryType::Pseudotubes:
      geom_type_combobox->setCurrentIndex(0);
      break;
    case TrackGeometryType::Lines:
      geom_type_combobox->setCurrentIndex(1);
      break;
    case TrackGeometryType::Points:
      geom_type_combobox->setCurrentIndex(2);
      break;
    }
    geom_type_combobox->clearError();
    geom_type_combobox->blockSignals(false);
  } else {
    geom_type_combobox->setError();
  }

  thickness_slider->blockSignals(true);
  thickness_slider->setSliderPosition(mean_thickness / static_cast<float>(indices.size()));
  thickness_slider->blockSignals(false);
}

void Tractography::update_scalar_options() {
  QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
  if (indices.size() == 1)
    scalar_file_options->set_tractogram(tractogram_list_model->get_tractogram(indices[0]));
  else
    scalar_file_options->set_tractogram(nullptr);
  scalar_file_options->update_UI();
}

void Tractography::update_geometry_type_gui() {
  QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
  const bool any = !indices.empty();
  const bool single = indices.size() == 1;
  const Tractogram *first_tractogram = any ? tractogram_list_model->get_tractogram(indices[0]) : nullptr;

  // The outer "Tractogram-specific options" group appears with any selection.
  tractogram_options_groupbox->setVisible(any);

  // Per-mechanism groups: geometry and colour apply to any selection; thresholding
  //   operates on a single tractogram; thickness needs a width-bearing geometry.
  geometry_groupbox->setVisible(any);
  colour_groupbox->setVisible(any);
  threshold_groupbox->setVisible(single);
  thickness_groupbox->setVisible(false);

  geom_type_combobox->setEnabled(any);
  lighting_button->setEnabled(false);
  lighting_group_box->setEnabled(false);

  if (!first_tractogram)
    return;

  const TrackGeometryType geom_type = first_tractogram->get_geometry_type();
  if (geom_type != TrackGeometryType::Pseudotubes && geom_type != TrackGeometryType::Points)
    return;

  // The whole thickness group (slider, modulation combo, offset/scale) is shown
  //   only for a width-bearing geometry, so the slider and modulation control share
  //   visibility exactly.
  thickness_groupbox->setVisible(true);
  lighting_button->setEnabled(true);
  lighting_group_box->setEnabled(true);

  // Modulation needs a single selection to choose a field; offset and scale are
  //   only meaningful while modulation is active.
  thickness_modulation_combobox->setEnabled(single);
  thickness_modulation_label->setEnabled(single);
  const bool modulating = single && first_tractogram->get_thickness_type() == TrackThicknessType::SidecarData;
  thickness_offset_label->setVisible(modulating);
  thickness_offset_button->setVisible(modulating);
  thickness_scale_label->setVisible(modulating);
  thickness_scale_button->setVisible(modulating);
  thickness_power_checkbox->setVisible(modulating);
  if (modulating) {
    thickness_offset_button->blockSignals(true);
    thickness_offset_button->setRate(first_tractogram->get_thickness_offset_rate());
    thickness_offset_button->setValue(first_tractogram->thickness_offset);
    thickness_offset_button->blockSignals(false);
    thickness_scale_button->blockSignals(true);
    thickness_scale_button->setRate(first_tractogram->get_thickness_offset_rate());
    thickness_scale_button->setValue(first_tractogram->thickness_scale);
    thickness_scale_button->blockSignals(false);
    thickness_power_checkbox->blockSignals(true);
    thickness_power_checkbox->setChecked(first_tractogram->thickness_power_transform);
    // Only the detail relevant to the current geometry is shown.
    thickness_power_checkbox->setText(geom_type == TrackGeometryType::Pseudotubes ? "value is cylinder area"
                                                                                  : "value is sphere volume");
    thickness_power_checkbox->blockSignals(false);
  }
}

void Tractography::add_commandline_options(MR::App::OptionList &options) {
  using namespace MR::App;
  // clang-format off
  options + OptionGroup("Tractography tool options")

      + Option("tractography.load",
               "Load the specified tracks file into the tractography tool.").allow_multiple()
        + Argument("tracks").type_file_in()

      + Option("tractography.thickness",
               "Line thickness of tractography display, [-1.0, 1.0];"
               " default is 0.0.").allow_multiple()
        + Argument("value").type_float(-1.0, 1.0)

      + Option("tractography.geometry",
               "The geometry type to use when rendering tractograms"
               " (options are: " + MR::Enum::join<TrackGeometryType>() + ")").allow_multiple()
        + Argument("value").type_choice<TrackGeometryType>()

      + Option("tractography.opacity",
               "Opacity of tractography display, [0.0, 1.0];"
               " default is 1.0.").allow_multiple()
        + Argument("value").type_float(0.0, 1.0)

      + Option("tractography.slab",
               "Slab thickness of tractography display, in mm."
               " -1 to turn off crop to slab.").allow_multiple()
        + Argument("value").type_float(-1, 1e6)

      + Option("tractography.lighting",
               "Toggle the use of lighting of tractogram geometry").allow_multiple()
        + Argument("value").type_bool()

      + Option("tractography.colour",
               "Specify a manual colour for the tractogram,"
               " as three comma-separated values").allow_multiple()
        + Argument("R,G,B").type_sequence_float()

      + Option("tractography.tsf_load",
               "Load the specified tractography scalar file.").allow_multiple()
        + Argument("tsf").type_file_in()

      + Option("tractography.tsf_range",
               "Set range for the tractography scalar file."
               " Requires -tractography.tsf_load already provided.").allow_multiple()
        + Argument("RangeMin,RangeMax").type_sequence_float()

      + Option("tractography.tsf_thresh",
               "Set thresholds for the tractography scalar file."
               " Requires -tractography.tsf_load already provided.").allow_multiple()
        + Argument("ThresholdMin,ThresholdMax").type_sequence_float()

      + Option("tractography.tsf_colourmap",
               "Sets the colourmap of the .tsf file as indexed in the tsf colourmap dropdown menu."
               " Requires -tractography.tsf_load already.").allow_multiple()
        + Argument("index").type_integer();
  // clang-format on
}

/*
  Selects the last tractogram in the tractogram_list_view and updates the window. If no tractograms are in the list
  view, no action is taken.
*/
void Tractography::select_last_added_tractogram() {
  int count = tractogram_list_model->rowCount();
  if (count != 0) {
    QModelIndex index = tractogram_list_view->model()->index(count - 1, 0);
    tractogram_list_view->setCurrentIndex(index);
    window().updateGL();
  }
}

bool Tractography::process_commandline_option(const MR::App::ParsedOption &opt) {

  if (opt.opt->is("tractography.load")) {
    std::vector<std::filesystem::path> list(1, opt[0]);
    add_tractogram(list);
    return true;
  }

  if (opt.opt->is("tractography.tsf_load")) {
    try {

      if (process_commandline_option_tsf_check_tracto_loaded()) {
        QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();

        if (indices.size() == 1) { // just in case future edits break this assumption
          Tractogram *tractogram = tractogram_list_model->get_tractogram(indices[0]);

          // set its tsf filename and load the tsf file
          scalar_file_options->set_tractogram(tractogram);
          scalar_file_options->open_intensity_track_scalar_file_slot(std::string(opt[0]));

          // Reflect the loaded file as the trailing colour-combo entry.
          update_colour_gui();
        }
      }
    } catch (Exception &E) {
      E.display();
    }

    return true;
  }

  if (opt.opt->is("tractography.tsf_range")) {
    try {
      // Set the tsf visualisation range
      std::vector<default_type> range;
      if (process_commandline_option_tsf_option(opt, 2, range))
        scalar_file_options->set_scaling(range[0], range[1]);
    } catch (Exception &E) {
      E.display();
    }
    return true;
  }

  if (opt.opt->is("tractography.tsf_thresh")) {
    try {
      // Set the tsf visualisation threshold
      std::vector<default_type> range;
      if (process_commandline_option_tsf_option(opt, 2, range))
        scalar_file_options->set_threshold(range[0], range[1]);
    } catch (Exception &E) {
      E.display();
    }
    return true;
  }

  if (opt.opt->is("tractography.thickness")) {
    // Thickness runs from -1000 to 1000,
    float thickness = static_cast<float>(opt[0]) * 1000.0F;
    try {
      thickness_slider->setValue(thickness);
    } catch (Exception &E) {
      E.display();
    }
    return true;
  }

  if (opt.opt->is("tractography.tsf_colourmap")) {
    try {
      int n = opt[0];
      if (n < 0 || ColourMap::maps[n].name.empty())
        throw Exception("invalid tsf colourmap index \"" + std::string(opt[0]) +
                        "\" for -tractography.tsf_colourmap option");
      if (process_commandline_option_tsf_check_tracto_loaded()) {
        // get list of selected tractograms:
        QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
        if (indices.size() != 1)
          throw Exception("-tractography.tsf_colourmap option requires one tractogram to be selected");

        // get pointer to tractogram:
        Tractogram *tractogram = tractogram_list_model->get_tractogram(indices[0]);

        // check tractogram has a scalar file attached and prepare the scalar_file_options object:
        if (tractogram->get_color_type() == TrackColourType::ScalarFile) {
          scalar_file_options->set_tractogram(tractogram);
          scalar_file_options->set_colourmap(opt[0]);
        }
      }
    } catch (Exception &e) {
      e.display();
    }
    return true;
  }

  if (opt.opt->is("tractography.colour")) {
    try {

      auto values = parse_floats(opt[0]);
      if (values.size() != 3)
        throw Exception("must provide exactly three comma-separated values to the -tractography.colour option");
      const float max_value = std::max({values[0], values[1], values[2]});
      if (std::min({values[0], values[1], values[2]}) < 0.0 || max_value > 255)
        throw Exception(
            "values provided to -tractogram.colour must be either between 0.0 and 1.0, or between 0 and 255");
      const float multiplier = max_value <= 1.0 ? 255.0 : 1.0;

      // input need to be a float *
      QColor colour(multiplier * values[0], multiplier * values[1], multiplier * values[2]);

      QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();

      if (indices.size() != 1)
        throw Exception("-tractography.colour option requires one tractogram to be selected");
      // get pointer to tractogram:
      Tractogram *tractogram = tractogram_list_model->get_tractogram(indices[0]);

      // set the color
      tractogram->set_color_type(TrackColourType::Manual);
      tractogram->set_colour(colour);

      update_colour_gui();

    } catch (Exception &e) {
      e.display();
    }
    return true;
  }

  if (opt.opt->is("tractography.geometry")) {
    try {
      const TrackGeometryType geom_type = MR::Enum::from_name<TrackGeometryType>(std::string(opt[0]));
      QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
      if (!indices.empty()) {
        for (int i = 0; i < indices.size(); ++i)
          tractogram_list_model->get_tractogram(indices[i])->set_geometry_type(geom_type);
      } else {
        Tractogram::default_tract_geom = geom_type;
      }
      update_geometry_type_gui();
    } catch (Exception &E) {
      E.display();
    }
    return true;
  }

  if (opt.opt->is("tractography.opacity")) {
    // Opacity runs from 0 to 1000, so multiply by 1000
    float opacity = static_cast<float>(opt[0]) * 1000.0F;
    try {
      opacity_slider->setValue(opacity);
    } catch (Exception &E) {
      E.display();
    }
    return true;
  }

  if (opt.opt->is("tractography.slab")) {
    float thickness = opt[0];
    try {
      bool crop = thickness > 0;
      slab_group_box->setChecked(crop);
      on_crop_to_slab_slot(crop); // Needs to be manually bumped
      if (crop) {
        slab_entry->setValue(thickness);
        on_slab_thickness_slot(); // Needs to be manually bumped
      }
    } catch (Exception &E) {
      E.display();
    }
    return true;
  }

  if (opt.opt->is("tractography.lighting")) {
    const bool value = bool(opt[0]);
    lighting_group_box->setChecked(value);
    use_lighting = bool(value);
    return true;
  }

  return false;
}

/*Checks whether any tractography has been loaded and warns the user if it has not*/
bool Tractography::process_commandline_option_tsf_check_tracto_loaded() {
  int count = tractogram_list_model->rowCount();
  if (count == 0) {
    // Error to std error to save many dialogs appearing for a single missed argument
    std::cerr << "TSF argument specified but no tractography loaded. Ensure TSF arguments follow the tractography.load "
                 "argument.\n";
  }
  return count != 0;
}

/*Checks whether legal to apply tsf options and prepares the scalar_file_options to do so. Returns the vector of floats
 * parsed from the options, or null on fail*/
bool Tractography::process_commandline_option_tsf_option(const MR::App::ParsedOption &opt,
                                                         uint reqArgSize,
                                                         std::vector<default_type> &range) {
  if (process_commandline_option_tsf_check_tracto_loaded()) {
    QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
    range = opt[0].as_sequence_float();
    if (indices.size() == 1 && range.size() == reqArgSize) {
      // values supplied
      Tractogram *tractogram = tractogram_list_model->get_tractogram(indices[0]);
      if (tractogram->get_color_type() == TrackColourType::ScalarFile) {
        // prereq options supplied/executed
        scalar_file_options->set_tractogram(tractogram);
        return true;
      } else {
        std::cerr << "Could not apply TSF argument - tractography.load_tsf not supplied.\n";
      }
    } else {
      std::cerr << "Could not apply TSF argument - insufficient number of arguments provided.\n";
    }
  }
  return false;
}
} // namespace MR::GUI::MRView::Tool
