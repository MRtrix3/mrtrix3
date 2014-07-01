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
            line_length_by_value = new QGroupBox (tr("line length by value"));
            line_length_by_value->setCheckable (true);
            line_length_by_value->setChecked (false);
            hlayout->addWidget (line_length_by_value);
            connect (line_length_by_value, SIGNAL (clicked (bool)), this, SLOT (on_line_length_by_value_slot (bool)));

            hlayout = new HBoxLayout;
            main_box->addLayout (hlayout);
            hlayout->addWidget (new QLabel ("line length"));
            line_length = new AdjustButton (this, 0.01);
            line_length->setMin (0.1);
            line_length->setValue (1.0);
            connect (line_length, SIGNAL (valueChanged()), this, SLOT (line_length_slot()));
            hlayout->addWidget (line_length);

            GridLayout* default_opt_grid = new GridLayout;
            line_thickness_slider = new QSlider (Qt::Horizontal);
            line_thickness_slider->setRange (100,1500);
            line_thickness_slider->setSliderPosition (float (100.0));
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
              dynamic_cast<Fixel*>(fixel_list_model->items[i])->renderColourBar (window, transform);
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
          //TODO fix multi selection seg fault, partially checked line length by value
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          colour_combobox->setEnabled (indices.size());
          colourmap_menu->setEnabled (indices.size());
          max_value->setEnabled (indices.size());
          min_value->setEnabled (indices.size());
          threshold_lower_box->setEnabled (indices.size());
          threshold_upper_box->setEnabled (indices.size());
          threshold_lower->setEnabled (indices.size());
          threshold_upper->setEnabled (indices.size());
          line_length->setEnabled (indices.size());
          line_length_by_value->setEnabled (indices.size());

          if (!indices.size()) {
            max_value->setValue (NAN);
            min_value->setValue (NAN);
            threshold_lower->setValue (NAN);
            threshold_upper->setValue (NAN);
            line_length->setValue (NAN);
            line_length_by_value->setChecked (false);
            return;
          }

          float rate = 0.0f, min_val = 0.0f, max_val = 0.0f;
          float lower_threshold_val = 0.0f, upper_threshold_val = 0.0f;
          float line_length_multiplier = 0.0f;
          int num_lower_threshold = 0, num_upper_threshold = 0;
          int num_line_length_by_value = 0;
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
            num_lower_threshold += fixel->use_discard_lower();
            num_upper_threshold += fixel->use_discard_upper();
            if (!std::isfinite (fixel->lessthan))
              fixel->lessthan = fixel->intensity_min();
            if (!std::isfinite (fixel->greaterthan))
              fixel->greaterthan = fixel->intensity_max();
            lower_threshold_val += fixel->lessthan;
            upper_threshold_val += fixel->greaterthan;
            line_length_multiplier += fixel->get_line_length_multiplier();
            num_line_length_by_value += fixel->get_line_length_by_value();
          }

          rate /= indices.size();
          min_val /= indices.size();
          max_val /= indices.size();
          lower_threshold_val /= indices.size();
          upper_threshold_val /= indices.size();
          line_length_multiplier /= indices.size();

          if (colourmap_index < 0)
            for (size_t i = 0; MR::GUI::MRView::ColourMap::maps[i].name; ++i )
              colourmap_actions[i]->setChecked (false);
          else
            colourmap_actions[colourmap_index]->setChecked (true);

          min_value->setRate (rate);
          max_value->setRate (rate);
          min_value->setValue (min_val);
          max_value->setValue (max_val);
          line_length->setValue (line_length_multiplier);

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

          line_length_by_value->setChecked (num_line_length_by_value ?
              ( num_line_length_by_value == indices.size() ?
                Qt::Checked :
                Qt::PartiallyChecked ) :
                Qt::Unchecked);
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


        void Vector::selection_changed_slot (const QItemSelection &, const QItemSelection &)
        {
          update_selection ();
        }


        void Vector::on_crop_to_slice_slot (bool is_checked)
        {
          do_crop_to_slice = is_checked;
          window.updateGL();
        }


        void Vector::line_length_slot ()
        {
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            fixel_list_model->get_fixel_image (indices[i])->set_line_length_multiplier (line_length->value());
          }
          window.updateGL();
        }

        void Vector::on_line_length_by_value_slot (bool length_by_value) {
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            fixel_list_model->get_fixel_image (indices[i])->set_line_length_by_value (length_by_value);
          }
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
              colourmap_menu->setEnabled (true);
              for (int i = 0; i < indices.size(); ++i)
                fixel_list_model->get_fixel_image (indices[i])->set_colour_type (Value);
              break;
            }
            case 1: {
              colourmap_menu->setEnabled (false);
              for (int i = 0; i < indices.size(); ++i)
                fixel_list_model->get_fixel_image (indices[i])->set_colour_type (Direction);
              break;
            }
            case 2: {
              colourmap_menu->setEnabled (false);
              QColor color;
              color = QColorDialog::getColor(Qt::red, this, "Select Color", QColorDialog::DontUseNativeDialog);
              float colour[] = {float(color.redF()), float(color.greenF()), float(color.blueF())};
              if (color.isValid()) {
                for (int i = 0; i < indices.size(); ++i) {
                  fixel_list_model->get_fixel_image (indices[i])->set_colour_type (Colour);
                  fixel_list_model->get_fixel_image (indices[i])->set_colour (colour);
                }
              }
              break;
            }
            case 3: {
              colourmap_menu->setEnabled (false);
              for (int i = 0; i < indices.size(); ++i) {
                float colour[3];
                Math::RNG rng;
                do {
                  colour[0] = rng.uniform();
                  colour[1] = rng.uniform();
                  colour[2] = rng.uniform();
                } while (colour[0] < 0.5 && colour[1] < 0.5 && colour[2] < 0.5);
                dynamic_cast<Fixel*> (fixel_list_model->items[indices[i].row()])->set_colour_type (Colour);
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


      }
    }
  }
}





