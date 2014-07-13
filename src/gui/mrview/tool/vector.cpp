/*
   Copyright 2014 Brain Research Institute, Melbourne, Australia

   Written by David Raffelt, 2014

   This file is part of MRtrix.

   MRtrix is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MRtrix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "mrtrix.h"
#include "gui/mrview/window.h"
#include "gui/mrview/tool/vector.h"
#include "gui/mrview/tool/fixel.h"
#include "gui/dialog/file.h"
#include "gui/mrview/tool/list_model_base.h"
#include "math/rng.h"
#include "gui/mrview/colourmap.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        class Vector::Model : public ListModelBase
        {

          public:
            Model (QObject* parent) :
              ListModelBase (parent) { }

            void add_items (std::vector<std::string>& filenames, Vector& fixel_tool) {
              beginInsertRows (QModelIndex(), items.size(), items.size() + filenames.size());
              for (size_t i = 0; i < filenames.size(); ++i) {
                Fixel* fixel_image = new Fixel (filenames[i], fixel_tool);
                items.push_back (fixel_image);
              }
              endInsertRows();
            }

            Fixel* get_fixel_image (QModelIndex& index) {
              return dynamic_cast<Fixel*>(items[index.row()]);
            }
        };



        Vector::Vector (Window& main_window, Dock* parent) :
          Base (main_window, parent),
          line_thickness (2.0),
          do_crop_to_slice (true),
          not_3D (true),
          line_opacity (1.0) {

            VBoxLayout* main_box = new VBoxLayout (this);
            HBoxLayout* layout = new HBoxLayout;
            layout->setContentsMargins (0, 0, 0, 0);
            layout->setSpacing (0);

            QPushButton* button = new QPushButton (this);
            button->setToolTip (tr ("Open Fixel Image"));
            button->setIcon (QIcon (":/open.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (fixel_open_slot ()));
            layout->addWidget (button, 1);

            button = new QPushButton (this);
            button->setToolTip (tr ("Close Fixel Image"));
            button->setIcon (QIcon (":/close.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (fixel_close_slot ()));
            layout->addWidget (button, 1);

            hide_all_button = new QPushButton (this);
            hide_all_button->setToolTip (tr ("Hide Fixel Images"));
            hide_all_button->setIcon (QIcon (":/hide.svg"));
            hide_all_button->setCheckable (true);
            connect (hide_all_button, SIGNAL (clicked()), this, SLOT (hide_all_slot ()));
            layout->addWidget (hide_all_button, 1);

            main_box->addLayout (layout, 0);

            fixel_list_view = new QListView (this);
            fixel_list_view->setSelectionMode (QAbstractItemView::ExtendedSelection);
            fixel_list_view->setDragEnabled (true);
            fixel_list_view->viewport()->setAcceptDrops (true);
            fixel_list_view->setDropIndicatorShown (true);

            fixel_list_model = new Model (this);
            fixel_list_view->setModel (fixel_list_model);

            connect (fixel_list_model, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
                     this, SLOT (toggle_shown_slot (const QModelIndex&, const QModelIndex&)));

            connect (fixel_list_view->selectionModel(),
                     SIGNAL (selectionChanged (const QItemSelection &, const QItemSelection &)),
                     SLOT (selection_changed_slot (const QItemSelection &, const QItemSelection &)));

            main_box->addWidget (fixel_list_view, 1);


            HBoxLayout* hlayout = new HBoxLayout;
            hlayout->setContentsMargins (0, 0, 0, 0);
            hlayout->setSpacing (0);

            colour_combobox = new QComboBox;
            colour_combobox->addItem ("Colour By Value");
            colour_combobox->addItem ("Colour By Direction");
            colour_combobox->addItem ("Set Colour");
            colour_combobox->addItem ("Randomise Colour");
            hlayout->addWidget (colour_combobox, 0);
            connect (colour_combobox, SIGNAL (activated(int)), this, SLOT (colour_changed_slot(int)));

            // Colourmap menu:
            colourmap_menu = new QMenu (tr ("Colourmap menu"), this);

            ColourMap::create_menu (this, colourmap_group, colourmap_menu, colourmap_actions, false, false);
            connect (colourmap_group, SIGNAL (triggered (QAction*)), this, SLOT (select_colourmap_slot()));
            colourmap_actions[1]->setChecked (true);

            colourmap_menu->addSeparator();

            show_colour_bar = colourmap_menu->addAction (tr ("Show colour bar"), this, SLOT (show_colour_bar_slot()));
            show_colour_bar->setCheckable (true);
            show_colour_bar->setChecked (true);
            addAction (show_colour_bar);

            invert_scale = colourmap_menu->addAction (tr ("Invert"), this, SLOT (invert_colourmap_slot()));
            invert_scale->setCheckable (true);
            addAction (invert_scale);

            QAction* reset_intensity = colourmap_menu->addAction (tr ("Reset intensity"), this, SLOT (reset_intensity_slot()));
            addAction (reset_intensity);

            // TODO It may make more sense for the colourmap button to appear alongside / with the intensity scaling,
            //   as these GUI elements are only applicable if the 'colour by' combo box is set to 'value'
            colourmap_button = new QToolButton (this);
            colourmap_button->setToolTip (tr ("Colourmap menu"));
            colourmap_button->setIcon (QIcon (":/colourmap.svg"));
            colourmap_button->setPopupMode (QToolButton::InstantPopup);
            colourmap_button->setMenu (colourmap_menu);
            hlayout->addWidget (colourmap_button);
            main_box->addLayout (hlayout);

            QGroupBox* group_box = new QGroupBox ("Intensity scaling");
            main_box->addWidget (group_box);
            hlayout = new HBoxLayout;
            group_box->setLayout (hlayout);

            min_value = new AdjustButton (this);
            connect (min_value, SIGNAL (valueChanged()), this, SLOT (on_set_scaling_slot()));
            hlayout->addWidget (min_value);

            max_value = new AdjustButton (this);
            connect (max_value, SIGNAL (valueChanged()), this, SLOT (on_set_scaling_slot()));
            hlayout->addWidget (max_value);

            QGroupBox* threshold_box = new QGroupBox ("Thresholds");
            main_box->addWidget (threshold_box);
            hlayout = new HBoxLayout;
            threshold_box->setLayout (hlayout);

            threshold_lower_box = new QCheckBox (this);
            connect (threshold_lower_box, SIGNAL (stateChanged(int)), this, SLOT (threshold_lower_changed(int)));
            hlayout->addWidget (threshold_lower_box);
            threshold_lower = new AdjustButton (this, 0.1);
            connect (threshold_lower, SIGNAL (valueChanged()), this, SLOT (threshold_lower_value_changed()));
            hlayout->addWidget (threshold_lower);

            threshold_upper_box = new QCheckBox (this);
            hlayout->addWidget (threshold_upper_box);
            threshold_upper = new AdjustButton (this, 0.1);
            connect (threshold_upper_box, SIGNAL (stateChanged(int)), this, SLOT (threshold_upper_changed(int)));
            connect (threshold_upper, SIGNAL (valueChanged()), this, SLOT (threshold_upper_value_changed()));
            hlayout->addWidget (threshold_upper);

            hlayout = new HBoxLayout;
            main_box->addLayout (hlayout);
            hlayout->addWidget (new QLabel ("scale by"));
            length_combobox = new QComboBox;
            length_combobox->addItem ("Unity");
            length_combobox->addItem ("Fixel amplitude");
            length_combobox->addItem ("Associated value");
            hlayout->addWidget (length_combobox, 0);
            connect (length_combobox, SIGNAL (activated(int)), this, SLOT (length_type_slot(int)));

            hlayout = new HBoxLayout;
            main_box->addLayout (hlayout);
            hlayout->addWidget (new QLabel ("length multiplier"));
            line_length = new AdjustButton (this, 0.01);
            line_length->setMin (0.1);
            line_length->setValue (1.0);
            connect (line_length, SIGNAL (valueChanged()), this, SLOT (length_multiplier_slot()));
            hlayout->addWidget (line_length);

            GridLayout* default_opt_grid = new GridLayout;
            line_thickness_slider = new QSlider (Qt::Horizontal);
            line_thickness_slider->setRange (100,1500);
            line_thickness_slider->setSliderPosition (float (200.0));
            connect (line_thickness_slider, SIGNAL (valueChanged (int)), this, SLOT (line_thickness_slot (int)));
            default_opt_grid->addWidget (new QLabel ("line thickness"), 0, 0);
            default_opt_grid->addWidget (line_thickness_slider, 0, 1);

            opacity_slider = new QSlider (Qt::Horizontal);
            opacity_slider->setRange (1, 1000);
            opacity_slider->setSliderPosition (int (1000));
            connect (opacity_slider, SIGNAL (valueChanged (int)), this, SLOT (opacity_slot (int)));
            default_opt_grid->addWidget (new QLabel ("opacity"), 1, 0);
            default_opt_grid->addWidget (opacity_slider, 1, 1);

            crop_to_slice = new QGroupBox (tr("crop to slice"));
            crop_to_slice->setCheckable (true);
            crop_to_slice->setChecked (true);
            connect (crop_to_slice, SIGNAL (clicked (bool)), this, SLOT (on_crop_to_slice_slot (bool)));
            default_opt_grid->addWidget (crop_to_slice, 2, 0, 1, 2);

            main_box->addLayout (default_opt_grid, 0);

            main_box->addStretch ();
            setMinimumSize (main_box->minimumSize());
            update_selection();
        }


        Vector::~Vector () {}


        void Vector::draw (const Projection& transform, bool is_3D, int axis, int slice)
        {
          not_3D = !is_3D;
          if (!window.snap_to_image() && do_crop_to_slice)
            return;
          for (int i = 0; i < fixel_list_model->rowCount(); ++i) {
            if (fixel_list_model->items[i]->show && !hide_all_button->isChecked())
              dynamic_cast<Fixel*>(fixel_list_model->items[i])->render (transform, axis, slice);
          }
        }


        void Vector::drawOverlays (const Projection& transform)
        {
          for (int i = 0; i < fixel_list_model->rowCount(); ++i) {
            if (fixel_list_model->items[i]->show)
              dynamic_cast<Fixel*>(fixel_list_model->items[i])->renderColourBar (transform);
          }
        }


        void Vector::fixel_open_slot ()
        {
          std::vector<std::string> list = Dialog::File::get_files (this, "Select fixel images to open", "MRtrix sparse format (*.msf *.msh)");
          if (list.empty())
            return;
          size_t previous_size = fixel_list_model->rowCount();
          fixel_list_model->add_items (list, *this);
          QModelIndex first = fixel_list_model->index (previous_size, 0, QModelIndex());
          QModelIndex last = fixel_list_model->index (fixel_list_model->rowCount()-1, 0, QModelIndex());
          fixel_list_view->selectionModel()->select (QItemSelection (first, last), QItemSelectionModel::Select);
          update_selection();
        }


        void Vector::fixel_close_slot ()
        {
          QModelIndexList indexes = fixel_list_view->selectionModel()->selectedIndexes();
          while (indexes.size()) {
            fixel_list_model->remove_item (indexes.first());
            indexes = fixel_list_view->selectionModel()->selectedIndexes();
          }
          window.updateGL();
        }


        void Vector::toggle_shown_slot (const QModelIndex& index, const QModelIndex& index2)
        {
          if (index.row() == index2.row()) {
            fixel_list_view->setCurrentIndex(index);
          } else {
            for (size_t i = 0; i < fixel_list_model->items.size(); ++i) {
              if (fixel_list_model->items[i]->show) {
                fixel_list_view->setCurrentIndex (fixel_list_model->index (i, 0));
                break;
              }
            }
          }
          window.updateGL();
        }


        void Vector::hide_all_slot ()
        {
          window.updateGL();
        }


        void Vector::update_selection ()
        {
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();

          colour_combobox->setEnabled (indices.size());
          colourmap_button->setEnabled (indices.size());
          max_value->setEnabled (indices.size());
          min_value->setEnabled (indices.size());
          threshold_lower_box->setEnabled (indices.size());
          threshold_upper_box->setEnabled (indices.size());
          threshold_lower->setEnabled (indices.size());
          threshold_upper->setEnabled (indices.size());
          line_length->setEnabled (indices.size());
          length_combobox->setEnabled (indices.size());

          if (!indices.size()) {
            max_value->setValue (NAN);
            min_value->setValue (NAN);
            threshold_lower->setValue (NAN);
            threshold_upper->setValue (NAN);
            line_length->setValue (NAN);
            return;
          }

          float rate = 0.0f, min_val = 0.0f, max_val = 0.0f;
          float lower_threshold_val = 0.0f, upper_threshold_val = 0.0f;
          float line_length_multiplier = 0.0f;
          int num_lower_threshold = 0, num_upper_threshold = 0;
          int colourmap_index = -2;
          for (int i = 0; i < indices.size(); ++i) {
            Fixel* fixel = dynamic_cast<Fixel*> (fixel_list_model->get_fixel_image (indices[i]));
            if (colourmap_index != int (fixel->colourmap)) {
              if (colourmap_index == -2)
                colourmap_index = fixel->colourmap;
              else
                colourmap_index = -1;
            }
            rate += fixel->scaling_rate();
            min_val += fixel->scaling_min();
            max_val += fixel->scaling_max();
            num_lower_threshold += (fixel->use_discard_lower() ? 1 : 0);
            num_upper_threshold += (fixel->use_discard_upper() ? 1 : 0);
            if (!std::isfinite (fixel->lessthan))
              fixel->lessthan = fixel->intensity_min();
            if (!std::isfinite (fixel->greaterthan))
              fixel->greaterthan = fixel->intensity_max();
            lower_threshold_val += fixel->lessthan;
            upper_threshold_val += fixel->greaterthan;
            line_length_multiplier += fixel->get_line_length_multiplier();
          }

          rate /= indices.size();
          min_val /= indices.size();
          max_val /= indices.size();
          lower_threshold_val /= indices.size();
          upper_threshold_val /= indices.size();
          line_length_multiplier /= indices.size();

          // Not all colourmaps are added to this list; therefore need to find out
          //   how many menu elements were actually created by ColourMap::create_menu()
          static size_t colourmap_count = 0;
          if (!colourmap_count) {
            for (size_t i = 0; MR::GUI::MRView::ColourMap::maps[i].name; ++i) {
              if (!MR::GUI::MRView::ColourMap::maps[i].special)
                ++colourmap_count;
            }
          }

          if (colourmap_index < 0) {
            for (size_t i = 0; i != colourmap_count; ++i )
              colourmap_actions[i]->setChecked (false);
          } else {
            colourmap_actions[colourmap_index]->setChecked (true);
          }

          // FIXME Intensity windowing display values are not correctly updated
          min_value->setRate (rate);
          max_value->setRate (rate);
          min_value->setValue (min_val);
          max_value->setValue (max_val);
          line_length->setValue (line_length_multiplier);

          // Do a better job of setting colour / length with multiple inputs
          Fixel* first_fixel = dynamic_cast<Fixel*> (fixel_list_model->get_fixel_image (indices[0]));
          const FixelLengthType length_type = first_fixel->get_length_type();
          const FixelColourType colour_type = first_fixel->get_colour_type();
          bool consistent_length = true, consistent_colour = true;
          size_t colour_by_value_count = (first_fixel->get_colour_type() == CValue);
          for (int i = 1; i < indices.size(); ++i) {
            Fixel* fixel = dynamic_cast<Fixel*> (fixel_list_model->get_fixel_image (indices[i]));
            if (fixel->get_length_type() != length_type)
              consistent_length = false;
            if (fixel->get_colour_type() != colour_type)
              consistent_colour = false;
            if (fixel->get_colour_type() == CValue)
              ++colour_by_value_count;
          }

          // TODO For both length and colour comboboxes, would like to indicate when
          //   the current set of selected files have different values for that
          //   particular parameter.
          // Only way I see of doing this is to add an extra element to the combobox,
          //   e.g. 'mixed', and change the combobox index to this item. The item
          //   is then erased from the combobox once the selection
          //   becomes consistent once again, and has no effect on the selected files
          //   if it is re-selected.

          if (consistent_length) {
            length_combobox->setCurrentIndex (length_type);
          } else {
            //length_combobox->setEnabled (false);
          }

          if (consistent_colour) {
            colour_combobox->setCurrentIndex (colour_type);
            enable_colour_by_value_features (colour_type == CValue);
          } else {
            //colour_combobox->setEnabled (false);
            // Enable as long as there is at least one colour-by-value
            enable_colour_by_value_features (colour_by_value_count);
          }

          threshold_lower->setValue (lower_threshold_val);
          threshold_lower_box->setCheckState (num_lower_threshold ?
              ( num_lower_threshold == indices.size() ?
                Qt::Checked :
                Qt::PartiallyChecked ) :
                Qt::Unchecked);
          threshold_lower->setRate (rate);

          threshold_upper->setValue (upper_threshold_val);
          threshold_upper_box->setCheckState (num_upper_threshold ?
              ( num_upper_threshold == indices.size() ?
                Qt::Checked :
                Qt::PartiallyChecked ) :
                Qt::Unchecked);
          threshold_upper->setRate (rate);
        }


        void Vector::opacity_slot (int opacity)
        {
          line_opacity = Math::pow2 (static_cast<float>(opacity)) / 1.0e6f;
          window.updateGL();
        }


        void Vector::line_thickness_slot (int thickness)
        {
          line_thickness = static_cast<float>(thickness) / 200.0f;
          window.updateGL();
        }


        void Vector::length_multiplier_slot ()
        {
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i)
            fixel_list_model->get_fixel_image (indices[i])->set_line_length_multiplier (line_length->value());
          window.updateGL();
        }


        void Vector::length_type_slot (int selection)
        {
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          switch (selection) {
            case 0: {
              for (int i = 0; i < indices.size(); ++i)
                fixel_list_model->get_fixel_image (indices[i])->set_length_type (Unity);
              break;
            }
            case 1: {
              for (int i = 0; i < indices.size(); ++i)
                fixel_list_model->get_fixel_image (indices[i])->set_length_type (Amplitude);
              break;
            }
            case 2: {
              for (int i = 0; i < indices.size(); ++i)
                fixel_list_model->get_fixel_image (indices[i])->set_length_type (LValue);
              break;
            }
          }
          window.updateGL();
        }


        void Vector::selection_changed_slot (const QItemSelection &, const QItemSelection &)
        {
          update_selection ();
        }


        void Vector::on_crop_to_slice_slot (bool is_checked)
        {
          do_crop_to_slice = is_checked;
          window.updateGL();
        }


        void Vector::show_colour_bar_slot ()
        {
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i)
            fixel_list_model->get_fixel_image (indices[i])->set_show_colour_bar (show_colour_bar->isChecked());
          window.updateGL();
        }


        void Vector::select_colourmap_slot ()
        {
          QAction* action = colourmap_group->checkedAction();
          size_t n = 0;
          while (action != colourmap_actions[n])
            ++n;
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i)
            fixel_list_model->get_fixel_image (indices[i])->colourmap = n;
          window.updateGL();
        }



        void Vector::colour_changed_slot (int selection)
        {
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();

          switch (selection) {
            case 0: {
              enable_colour_by_value_features (true);
              for (int i = 0; i < indices.size(); ++i)
                fixel_list_model->get_fixel_image (indices[i])->set_colour_type (CValue);
              break;
            }
            case 1: {
              enable_colour_by_value_features (false);
              for (int i = 0; i < indices.size(); ++i)
                fixel_list_model->get_fixel_image (indices[i])->set_colour_type (Direction);
              break;
            }
            case 2: {
              enable_colour_by_value_features (false);
              QColor color;
              color = QColorDialog::getColor(Qt::red, this, "Select Color", QColorDialog::DontUseNativeDialog);
              float colour[] = {float(color.redF()), float(color.greenF()), float(color.blueF())};
              if (color.isValid()) {
                for (int i = 0; i < indices.size(); ++i) {
                  fixel_list_model->get_fixel_image (indices[i])->set_colour_type (Manual);
                  fixel_list_model->get_fixel_image (indices[i])->set_colour (colour);
                }
              }
              break;
            }
            case 3: {
              enable_colour_by_value_features (false);
              for (int i = 0; i < indices.size(); ++i) {
                float colour[3];
                Math::RNG rng;
                do {
                  colour[0] = rng.uniform();
                  colour[1] = rng.uniform();
                  colour[2] = rng.uniform();
                } while (colour[0] < 0.5 && colour[1] < 0.5 && colour[2] < 0.5);
                dynamic_cast<Fixel*> (fixel_list_model->items[indices[i].row()])->set_colour_type (Manual);
                dynamic_cast<Fixel*> (fixel_list_model->items[indices[i].row()])->set_colour (colour);
              }
              break;
            }
            default:
              break;
          }
          window.updateGL();

        }


        void Vector::reset_intensity_slot ()
        {
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i)
            fixel_list_model->get_fixel_image (indices[i])->reset_windowing ();
          update_selection ();
          window.updateGL();
        }


        void Vector::invert_colourmap_slot ()
        {
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i)
            fixel_list_model->get_fixel_image (indices[i])->set_invert_scale (invert_scale->isChecked());
          window.updateGL();
        }


        void Vector::on_set_scaling_slot ()
        {
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i)
            fixel_list_model->get_fixel_image (indices[i])->set_windowing (min_value->value(), max_value->value());
          window.updateGL();
        }


        void Vector::threshold_lower_changed (int unused)
        {
          threshold_lower->setEnabled (threshold_lower_box->isChecked());
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i)
            fixel_list_model->get_fixel_image (indices[i])->set_use_discard_lower (threshold_lower_box->isChecked());
          window.updateGL();
        }


        void Vector::threshold_upper_changed (int unused)
        {
          threshold_upper->setEnabled (threshold_upper_box->isChecked());
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i)
            fixel_list_model->get_fixel_image (indices[i])->set_use_discard_upper (threshold_upper_box->isChecked());
          window.updateGL();
        }


        void Vector::threshold_lower_value_changed ()
        {
          if (threshold_lower_box->isChecked()) {
            QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
            for (int i = 0; i < indices.size(); ++i)
              fixel_list_model->get_fixel_image (indices[i])->lessthan = threshold_lower->value();
            window.updateGL();
          }
        }


        void Vector::threshold_upper_value_changed ()
        {
          if (threshold_upper_box->isChecked()) {
            QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
            for (int i = 0; i < indices.size(); ++i)
              fixel_list_model->get_fixel_image (indices[i])->greaterthan = threshold_upper->value();
            window.updateGL();
          }
        }


        bool Vector::process_batch_command (const std::string& cmd, const std::string& args)
        {
          // BATCH_COMMAND fixel.load path # Load the specified MRtrix sparse image file (.msf) into the fixel tool
          if (cmd == "fixel.load") {
            std::vector<std::string> list (1, args);
            try { fixel_list_model->add_items (list , *this); }
            catch (Exception& E) { E.display(); }
            return true;
          }
          return false;
        }



        void Vector::enable_colour_by_value_features (const bool by_value)
        {
          if (by_value) {
            colourmap_button->setEnabled (true);
            min_value->setEnabled (true);
            max_value->setEnabled (true);
            // Determine appropriate values to display for min and max scaling values
            QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
            if (indices.size()) {
              float min = 0.0f, max = 0.0f;
              for (int i = 0; i < indices.size(); ++i) {
                Fixel* fixel = dynamic_cast<Fixel*> (fixel_list_model->get_fixel_image (indices[i]));
                min += fixel->intensity_min();
                max += fixel->intensity_max();
              }
              min_value->setValue (min / indices.size());
              max_value->setValue (max / indices.size());
            }
          } else {
            colourmap_button->setEnabled (false);
            min_value->setValue (NAN);
            min_value->setEnabled (false);
            max_value->setValue (NAN);
            max_value->setEnabled (false);
          }
        }



      }
    }
  }
}





