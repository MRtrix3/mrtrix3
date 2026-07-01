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

#include "mrview/tool/tractography/track_scalar_file.h"

#include "dialog/file.h"
#include "mrview/tool/tractography/tractogram.h"

namespace MR::GUI::MRView::Tool {

TrackScalarFileOptions::TrackScalarFileOptions(Tractography *parent)
    : QObject(parent), tool(parent), tractogram(nullptr) {

  // The colour-map menu is placed beside the colour combo-box by the toolbar (and
  //   reparented there); it is created here because this object observes it.
  colourmap_button = new ColourMapButton(nullptr, *this, false, false, true);

  // Scaling spin-boxes (min / max); the toolbar embeds this widget in its "colour"
  //   group and shows it only while colouring by scalar data.
  colour_scaling_widget = new QWidget;
  Tool::Base::HBoxLayout *hlayout = new Tool::Base::HBoxLayout(colour_scaling_widget);
  hlayout->setContentsMargins(0, 0, 0, 0);
  hlayout->setSpacing(0);

  min_entry = new AdjustButton(colour_scaling_widget);
  connect(min_entry, SIGNAL(valueChanged()), this, SLOT(on_set_scaling_slot()));
  hlayout->addWidget(min_entry);

  max_entry = new AdjustButton(colour_scaling_widget);
  connect(max_entry, SIGNAL(valueChanged()), this, SLOT(on_set_scaling_slot()));
  hlayout->addWidget(max_entry);

  // Threshold controls; the toolbar embeds this widget in its "thresholding"
  //   group. The combo-box sits alongside its label (as colour and thickness do).
  threshold_widget = new QWidget;
  Tool::Base::VBoxLayout *threshold_layout = new Tool::Base::VBoxLayout(threshold_widget);
  threshold_layout->setContentsMargins(0, 0, 0, 0);
  threshold_layout->setSpacing(0);

  hlayout = new Tool::Base::HBoxLayout;
  hlayout->setContentsMargins(0, 0, 0, 0);
  hlayout->setSpacing(0);
  hlayout->addWidget(new QLabel("threshold", threshold_widget));

  threshold_file_combobox = new QComboBox(threshold_widget);
  threshold_file_combobox->addItem("None");
  threshold_file_combobox->addItem("Sidecar file");
  connect(threshold_file_combobox, SIGNAL(activated(int)), this, SLOT(threshold_scalar_file_slot(int)));
  hlayout->addWidget(threshold_file_combobox);

  threshold_layout->addLayout(hlayout);

  hlayout = new Tool::Base::HBoxLayout;
  hlayout->setContentsMargins(0, 0, 0, 0);
  hlayout->setSpacing(0);

  threshold_lower_box = new QCheckBox(threshold_widget);
  connect(threshold_lower_box, SIGNAL(stateChanged(int)), this, SLOT(threshold_lower_changed(int)));
  hlayout->addWidget(threshold_lower_box);
  threshold_lower = new AdjustButton(threshold_widget, 0.1);
  connect(threshold_lower, SIGNAL(valueChanged()), this, SLOT(threshold_lower_value_changed()));
  hlayout->addWidget(threshold_lower);

  threshold_upper_box = new QCheckBox(threshold_widget);
  hlayout->addWidget(threshold_upper_box);
  threshold_upper = new AdjustButton(threshold_widget, 0.1);
  connect(threshold_upper_box, SIGNAL(stateChanged(int)), this, SLOT(threshold_upper_changed(int)));
  connect(threshold_upper, SIGNAL(valueChanged()), this, SLOT(threshold_upper_value_changed()));
  hlayout->addWidget(threshold_upper);

  threshold_layout->addLayout(hlayout);

  update_UI();
}

void TrackScalarFileOptions::set_tractogram(Tractogram *selected_tractogram) { tractogram = selected_tractogram; }

void TrackScalarFileOptions::render_tractogram_colourbar(const Tractogram &tractogram) {
  // Colouring and thresholding are independent roles; the colour bar reflects the
  //   full colour scaling range irrespective of any (separate) threshold.
  window().colourbar_renderer.render(
      tractogram.colourmap,
      tractogram.scale_inverted(),
      tractogram.scaling_min(),
      tractogram.scaling_max(),
      tractogram.scaling_min(),
      tractogram.display_range,
      {tractogram.colour[0] / 255.0f, tractogram.colour[1] / 255.0f, tractogram.colour[2] / 255.0f});
}

void TrackScalarFileOptions::update_UI() {

  if (!tractogram) {
    colour_scaling_widget->setVisible(false);
    return;
  }

  if (tractogram->get_color_type() == TrackColourType::ScalarFile) {

    colour_scaling_widget->setVisible(true);
    min_entry->setRate(tractogram->scaling_rate());
    max_entry->setRate(tractogram->scaling_rate());
    min_entry->setValue(tractogram->scaling_min());
    max_entry->setValue(tractogram->scaling_max());

    colourmap_button->set_colourmap_index(tractogram->colourmap);
    colourmap_button->set_scale_inverted(tractogram->scale_inverted());
    colourmap_button->set_show_colourbar(tractogram->show_colour_bar);

  } else {
    colour_scaling_widget->setVisible(false);
  }

  // Rebuild the threshold combo around the three fixed modes: append one entry
  //   per embedded dpv/dps field column, then (when a separate file is loaded)
  //   the loaded-file label as the final entry.
  threshold_file_combobox->blockSignals(true);
  threshold_file_combobox->setToolTip(QString());
  while (threshold_file_combobox->count() > num_fixed_threshold_modes)
    threshold_file_combobox->removeItem(threshold_file_combobox->count() - 1);

  const std::vector<EmbeddedScalarField> &fields = tractogram->embedded_scalar_fields();
  for (const auto &field : fields)
    threshold_file_combobox->addItem(qstr(field.label()));

  switch (tractogram->get_threshold_type()) {
  case TrackThresholdType::None:
    threshold_file_combobox->setCurrentIndex(0);
    break;
  case TrackThresholdType::SeparateFile:
    if (tractogram->threshold_embedded_field.has_value()) {
      const size_t entry = *tractogram->threshold_embedded_field;
      const auto &field = fields[entry];
      threshold_file_combobox->setToolTip(qstr("Embedded field: " + field.label()));
      threshold_file_combobox->setCurrentIndex(num_fixed_threshold_modes + static_cast<int>(entry));
    } else {
      assert(!tractogram->threshold_scalar_path.empty());
      const int file_index = num_fixed_threshold_modes + static_cast<int>(fields.size());
      threshold_file_combobox->addItem(qstr(shorten(tractogram->threshold_scalar_path.filename().string(), 35, 0)));
      threshold_file_combobox->setToolTip(qstr(tractogram->threshold_scalar_path.string()));
      threshold_file_combobox->setCurrentIndex(file_index);
    }
    break;
  }
  threshold_file_combobox->blockSignals(false);

  const bool show_threshold_controls = (tractogram->get_threshold_type() != TrackThresholdType::None);
  threshold_lower_box->setVisible(show_threshold_controls);
  threshold_lower->setVisible(show_threshold_controls);
  threshold_upper_box->setVisible(show_threshold_controls);
  threshold_upper->setVisible(show_threshold_controls);

  if (show_threshold_controls) {
    threshold_lower_box->setChecked(tractogram->use_discard_lower());
    threshold_lower->setEnabled(tractogram->use_discard_lower());
    threshold_upper_box->setChecked(tractogram->use_discard_upper());
    threshold_upper->setEnabled(tractogram->use_discard_upper());
    threshold_lower->setRate(tractogram->get_threshold_rate());
    threshold_lower->setValue(tractogram->lessthan);
    threshold_upper->setRate(tractogram->get_threshold_rate());
    threshold_upper->setValue(tractogram->greaterthan);
  }
}

bool TrackScalarFileOptions::open_intensity_track_scalar_file_slot() {
  auto load_paths = Dialog::File::input_filepath(
      colour_scaling_widget, "Select scalar text file or Track Scalar file (.tsf) to open", "", tool->current_folder);
  if (!load_paths.empty())
    tool->current_folder = load_paths.last_directory;
  return open_intensity_track_scalar_file_slot(load_paths.single_selection);
}

bool TrackScalarFileOptions::open_intensity_track_scalar_file_slot(const std::filesystem::path &scalar_file) {
  bool returnvalue = false;
  if (!scalar_file.empty()) {
    try {
      tractogram->load_intensity_track_scalars(scalar_file);
      tractogram->set_color_type(TrackColourType::ScalarFile);
      returnvalue = true;
    } catch (Exception &E) {
      E.display();
    }
  }
  update_UI();
  window().updateGL();
  return returnvalue;
}

void TrackScalarFileOptions::toggle_show_colour_bar(bool show_colour_bar, const ColourMapButton &) {
  if (tractogram) {
    tractogram->show_colour_bar = show_colour_bar;
    window().updateGL();
  }
}

void TrackScalarFileOptions::selected_colourmap(size_t cmap, const ColourMapButton &) {
  if (tractogram) {
    tractogram->colourmap = cmap;
    window().updateGL();
  }
}

void TrackScalarFileOptions::selected_custom_colour(const QColor &c, const ColourMapButton &) {
  if (tractogram) {
    tractogram->set_colour(c);
    window().updateGL();
  }
}

void TrackScalarFileOptions::set_threshold(default_type min, default_type max) {
  if (tractogram) {
    // Threshold using the scalar data already loaded for colouring; the shared
    //   array means no second copy is uploaded to the GPU.
    tractogram->use_intensity_data_for_threshold();
    tractogram->set_threshold_type(TrackThresholdType::SeparateFile);
    tractogram->lessthan = min;
    tractogram->greaterthan = max;
    threshold_lower_box->setChecked(true);
    threshold_upper_box->setChecked(true);

    update_UI();
    window().updateGL();
  }
}

void TrackScalarFileOptions::set_scaling(default_type min, default_type max) {
  if (tractogram) {
    tractogram->set_windowing(min, max);
    update_UI();
    window().updateGL();
  }
}

void TrackScalarFileOptions::on_set_scaling_slot() {
  if (tractogram) {
    tractogram->set_windowing(min_entry->value(), max_entry->value());
    window().updateGL();
  }
}

bool TrackScalarFileOptions::threshold_scalar_file_slot(int /*unused*/) {

  const int index = threshold_file_combobox->currentIndex();
  const std::vector<EmbeddedScalarField> &fields = tractogram->embedded_scalar_fields();
  const int embedded_count = static_cast<int>(fields.size());
  // Combo layout: [None, Sidecar file, embedded..., (loaded file)]
  const int embedded_begin = num_fixed_threshold_modes;
  const int embedded_end = embedded_begin + embedded_count;

  switch (index) {
  case 0:
    tractogram->set_threshold_type(TrackThresholdType::None);
    tractogram->erase_threshold_scalar_data();
    tractogram->set_use_discard_lower(false);
    tractogram->set_use_discard_upper(false);
    break;
  case 1: {
    std::filesystem::path file_path;
    auto load_paths = Dialog::File::input_filepath(
        threshold_widget, "Select scalar text file or Track Scalar file (.tsf) to open", "", tool->current_folder);
    if (!load_paths.empty()) {
      tool->current_folder = load_paths.last_directory;
      try {
        file_path = load_paths.single_selection;
        tractogram->load_threshold_track_scalars(load_paths.single_selection);
        tractogram->set_threshold_type(TrackThresholdType::SeparateFile);
      } catch (Exception &E) {
        E.display();
        file_path.clear();
      }
    }
    if (file_path.empty()) {
      // Cancelled or failed: restore the combo to the unchanged state.
      update_UI();
      return false;
    }
  } break;
  default:
    if (index >= embedded_begin && index < embedded_end) {
      // An embedded dpv/dps field column was selected for thresholding.
      try {
        tractogram->load_threshold_embedded_scalars(static_cast<size_t>(index - embedded_begin));
        tractogram->set_threshold_type(TrackThresholdType::SeparateFile);
      } catch (Exception &E) {
        E.display();
        update_UI();
        return false;
      }
    } else {
      // The trailing loaded-file label: re-selected the active separate file.
      assert(tractogram->get_threshold_type() == TrackThresholdType::SeparateFile);
    }
    break;
  }
  update_UI();
  window().updateGL();
  return true;
}

void TrackScalarFileOptions::threshold_lower_changed(int) {
  if (tractogram) {
    threshold_lower->setEnabled(threshold_lower_box->isChecked());
    tractogram->set_use_discard_lower(threshold_lower_box->isChecked());
    window().updateGL();
  }
}

void TrackScalarFileOptions::threshold_upper_changed(int) {
  if (tractogram) {
    threshold_upper->setEnabled(threshold_upper_box->isChecked());
    tractogram->set_use_discard_upper(threshold_upper_box->isChecked());
    window().updateGL();
  }
}

void TrackScalarFileOptions::threshold_lower_value_changed() {
  if (tractogram && threshold_lower_box->isChecked()) {
    tractogram->lessthan = threshold_lower->value();
    window().updateGL();
  }
}

void TrackScalarFileOptions::threshold_upper_value_changed() {
  if (tractogram && threshold_upper_box->isChecked()) {
    tractogram->greaterthan = threshold_upper->value();
    window().updateGL();
  }
}

void TrackScalarFileOptions::reset_colourmap(const ColourMapButton &) {
  if (tractogram) {
    tractogram->reset_windowing();
    update_UI();
    window().updateGL();
  }
}

void TrackScalarFileOptions::toggle_invert_colourmap(bool invert, const ColourMapButton &) {
  if (tractogram) {
    tractogram->set_invert_scale(invert);
    window().updateGL();
  }
}

} // namespace MR::GUI::MRView::Tool
