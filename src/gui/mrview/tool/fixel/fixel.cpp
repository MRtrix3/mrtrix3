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
#include "gui/mrview/tool/fixel/fixel.h"
#include "gui/mrview/tool/fixel/image.h"
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


        class Fixel::Model : public ListModelBase
        {

          public:
            Model (QObject* parent) :
              ListModelBase (parent) { }

            void add_items (std::vector<std::string>& filenames, Fixel& fixel_tool) {
              beginInsertRows (QModelIndex(), items.size(), items.size() + filenames.size());
              for (size_t i = 0; i < filenames.size(); ++i) {
                FixelImage* fixel_image = new FixelImage (filenames[i], fixel_tool);
                items.push_back (fixel_image);
              }
              endInsertRows();
            }

            FixelImage* get_fixel_image (QModelIndex& index) {
              return dynamic_cast<FixelImage*>(items[index.row()]);
            }
        };



        Fixel::Fixel (Window& main_window, Dock* parent) :
          Base (main_window, parent),
          line_thickness (1.0),
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

            fixel_list_view->setContextMenuPolicy (Qt::CustomContextMenu);
            connect (fixel_list_view, SIGNAL (customContextMenuRequested (const QPoint&)),
                     this, SLOT (right_click_menu_slot (const QPoint&)));

            main_box->addWidget (fixel_list_view, 1);


            HBoxLayout* hlayout = new HBoxLayout;
            hlayout->setContentsMargins (0, 0, 0, 0);
            hlayout->setSpacing (0);

            colour_combobox = new QComboBox;
            colour_combobox->addItem ("Colour Bar");
            colour_combobox->addItem ("Colour By Direction");
            colour_combobox->addItem ("Set Colour");
            colour_combobox->addItem ("Randomise Colour");
            hlayout->addWidget (colour_combobox, 0);
            connect (colour_combobox, SIGNAL (activated(int)), this, SLOT (colour_changed(int)));

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

            min_entry = new AdjustButton (this);
            connect (min_entry, SIGNAL (valueChanged()), this, SLOT (on_set_scaling_slot()));
            hlayout->addWidget (min_entry);

            max_entry = new AdjustButton (this);
            connect (max_entry, SIGNAL (valueChanged()), this, SLOT (on_set_scaling_slot()));
            hlayout->addWidget (max_entry);


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

            GridLayout* default_opt_grid = new GridLayout;

            QSlider* slider;
            slider = new QSlider (Qt::Horizontal);
            slider->setRange (1, 1000);
            slider->setSliderPosition (int (1000));
            connect (slider, SIGNAL (valueChanged (int)), this, SLOT (opacity_slot (int)));
            default_opt_grid->addWidget (new QLabel ("opacity"), 0, 0);
            default_opt_grid->addWidget (slider, 0, 1);

            slider = new QSlider (Qt::Horizontal);
            slider->setRange (100,1000);
            slider->setSliderPosition (float (100.0));
            connect (slider, SIGNAL (valueChanged (int)), this, SLOT (line_thickness_slot (int)));
            default_opt_grid->addWidget (new QLabel ("line thickness"), 1, 0);
            default_opt_grid->addWidget (slider, 1, 1);

            QGroupBox* slab_group_box = new QGroupBox (tr("crop to slice"));
            slab_group_box->setCheckable (true);
            slab_group_box->setChecked (true);
            default_opt_grid->addWidget (slab_group_box, 2, 0, 1, 2);

            main_box->addLayout (default_opt_grid, 0);

            main_box->addStretch ();
            setMinimumSize (main_box->minimumSize());
        }


        Fixel::~Fixel () {}





        void Fixel::draw (const Projection& transform, bool is_3D)
        {
          if (!window.snap_to_image() && !is_3D)
            return;
          for (int i = 0; i < fixel_list_model->rowCount(); ++i) {
            if (fixel_list_model->items[i]->show && !hide_all_button->isChecked())
              dynamic_cast<FixelImage*>(fixel_list_model->items[i])->render (transform, is_3D, window.plane(), window.slice());
          }
        }





        void Fixel::drawOverlays (const Projection& transform)
        {
          for (int i = 0; i < fixel_list_model->rowCount(); ++i) {
            if (fixel_list_model->items[i]->show)
              dynamic_cast<FixelImage*>(fixel_list_model->items[i])->renderColourBar (transform);
          }
        }





        void Fixel::fixel_open_slot ()
        {
          std::vector<std::string> list = Dialog::File::get_files (this, "Select fixel images to open", "MRtrix sparse format (*.msf)");
          if (list.empty())
            return;
          else
            fixel_list_model->add_items (list, *this);
        }


        void Fixel::fixel_close_slot ()
        {
          QModelIndexList indexes = fixel_list_view->selectionModel()->selectedIndexes();
          while (indexes.size()) {
            fixel_list_model->remove_item (indexes.first());
            indexes = fixel_list_view->selectionModel()->selectedIndexes();
          }
          window.updateGL();
        }


        void Fixel::toggle_shown_slot (const QModelIndex& index, const QModelIndex& index2) {
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


        void Fixel::hide_all_slot () {
          window.updateGL();
        }


        void Fixel::opacity_slot (int opacity) {
          line_opacity = Math::pow2(static_cast<float>(opacity)) / 1.0e6f;
          window.updateGL();
        }


        void Fixel::line_thickness_slot (int thickness) {
          line_thickness = static_cast<float>(thickness) / 200.0f;
          window.updateGL();
        }


        void Fixel::colour_by_direction_slot()
        {
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i)  {
            fixel_list_model->get_fixel_image (indices[i])->color_type = Direction;
          }
          window.updateGL();
        }
        
        
        void Fixel::set_colour_slot()
        {
          QColor color;
          color = QColorDialog::getColor(Qt::red, this, "Select Color", QColorDialog::DontUseNativeDialog);
          float colour[] = {float(color.redF()), float(color.greenF()), float(color.blueF())};
          if (color.isValid()) {
            QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
            for (int i = 0; i < indices.size(); ++i) {
              fixel_list_model->get_fixel_image (indices[i])->color_type = Colour;
              fixel_list_model->get_fixel_image (indices[i])->set_colour (colour);
            }
          }
          window.updateGL();
        }


        void Fixel::randomise_colour_slot()
        {
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            float colour[3];
            Math::RNG rng;
            do {
              colour[0] = rng.uniform();
              colour[1] = rng.uniform();
              colour[2] = rng.uniform();
            } while (colour[0] < 0.5 && colour[1] < 0.5 && colour[2] < 0.5);
            dynamic_cast<FixelImage*> (fixel_list_model->items[indices[i].row()])->color_type = Colour;
            dynamic_cast<FixelImage*> (fixel_list_model->items[indices[i].row()])->set_colour (colour);
          }
          window.updateGL();
        }



        void Fixel::selection_changed_slot (const QItemSelection &, const QItemSelection &)
        {
          TRACE;
        }

        void Fixel::on_crop_to_grid_slot (bool is_checked)
        {
          TRACE;
        }

        void Fixel::line_size_slot (int thickness)
        {
          TRACE;
        }

        void Fixel::show_colour_bar_slot ()
        {
//          if (tractogram) {
//            tractogram->show_colour_bar = show_colour_bar->isChecked();
//            window.updateGL();
//          }
        }


        void Fixel::select_colourmap_slot (int selection)
        {
//          if (tractogram) {
//            QAction* action = colourmap_group->checkedAction();
//            size_t n = 0;
//            while (action != colourmap_actions[n])
//              ++n;
//            tractogram->colourmap = n;
//            window.updateGL();
//          }
        }

        void Fixel::colour_changed_slot (int selection)
        {
        }

        void Fixel::reset_intensity_slot ()
        {
//          if (tractogram) {
//            tractogram->set_windowing (min_entry->value(), max_entry->value());
//            window.updateGL();
//          }
        }


        void Fixel::invert_colourmap_slot ()
        {
//          if (tractogram) {
//            tractogram->set_windowing (min_entry->value(), max_entry->value());
//            window.updateGL();
//          }
        }



        void Fixel::on_set_scaling_slot ()
        {
//          if (tractogram) {
//            tractogram->set_windowing (min_entry->value(), max_entry->value());
//            window.updateGL();
//          }
        }


        void Fixel::threshold_lower_changed (int unused)
        {
//          if (tractogram) {
//            threshold_lower->setEnabled (threshold_lower_box->isChecked());
//            tractogram->set_use_discard_lower (threshold_lower_box->isChecked());
//            window.updateGL();
//          }
        }


        void Fixel::threshold_upper_changed (int unused)
        {
//          if (tractogram) {
//            threshold_upper->setEnabled (threshold_upper_box->isChecked());
//            tractogram->set_use_discard_upper (threshold_upper_box->isChecked());
//            window.updateGL();
//          }
        }



        void Fixel::threshold_lower_value_changed ()
        {
//          if (tractogram && threshold_lower_box->isChecked()) {
//            tractogram->lessthan = threshold_lower->value();
//            window.updateGL();
//          }
        }



        void Fixel::threshold_upper_value_changed ()
        {
//          if (tractogram && threshold_upper_box->isChecked()) {
//            tractogram->greaterthan = threshold_upper->value();
//            window.updateGL();
//          }
        }


//        bool Fixel::process_batch_command (const std::string& cmd, const std::string& args)
//        {
//          // BATCH_COMMAND fixel.load path # Load the specified tracks file into the fixel tool
//          if (cmd == "tractography.load") {
//            std::vector<std::string> list (1, args);
//            try { fixel_list_model->add_items (list, window, *this); }
//            catch (Exception& E) { E.display(); }
//            return true;
//          }
//
//          return false;
//        }


      }
    }
  }
}





