/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier and David Raffelt, 13/11/09.

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

#include <QLabel>
#include <QListView>
#include <QStringListModel>
#include <QColorDialog>

#include "mrtrix.h"
#include "gui/mrview/window.h"
#include "gui/mrview/tool/tractography/tractography.h"
#include "gui/dialog/file.h"
#include "gui/mrview/tool/list_model_base.h"
#include "gui/mrview/tool/tractography/track_scalar_file.h"
#include "gui/mrview/tool/tractography/tractogram.h"
#include "gui/opengl/lighting.h"
#include "gui/dialog/lighting.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        class Tractography::Model : public ListModelBase
        {

          public:
            Model (QObject* parent) :
              ListModelBase (parent) { }

            void add_items (std::vector<std::string>& filenames,
                            Window& main_window,
                            Tractography& tractography_tool) {

              for (size_t i = 0; i < filenames.size(); ++i) {
                Tractogram* tractogram = new Tractogram (main_window, tractography_tool, filenames[i]);
                try {
                  tractogram->load_tracks();
                  beginInsertRows (QModelIndex(), items.size(), items.size() + 1);
                  items.push_back (tractogram);
                  endInsertRows();
                } catch (Exception& e) {
                  delete tractogram;
                  e.display();
                }
              }
            }

            Tractogram* get_tractogram (QModelIndex& index) {
              return dynamic_cast<Tractogram*>(items[index.row()]);
            }
        };


        Tractography::Tractography (Window& main_window, Dock* parent) :
          Base (main_window, parent),
          line_thickness (1.0),
          do_crop_to_slab (true),
          use_lighting (false),
          line_opacity (1.0),
          scalar_file_options (NULL),
          lighting_dialog (NULL) {

            float voxel_size;
            if (main_window.image()) {
              voxel_size = (main_window.image()->voxel().vox(0) +
                            main_window.image()->voxel().vox(1) +
                            main_window.image()->voxel().vox(2)) / 3;
            } else {
              voxel_size = 2.5;
            }

            slab_thickness  = 2 * voxel_size;

            QVBoxLayout* main_box = new QVBoxLayout (this);
            QHBoxLayout* layout = new QHBoxLayout;
            layout->setContentsMargins (0, 0, 0, 0);
            layout->setSpacing (0);

            QPushButton* button = new QPushButton (this);
            button->setToolTip (tr ("Open Tracks"));
            button->setIcon (QIcon (":/open.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (tractogram_open_slot ()));
            layout->addWidget (button, 1);

            button = new QPushButton (this);
            button->setToolTip (tr ("Close Tracks"));
            button->setIcon (QIcon (":/close.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (tractogram_close_slot ()));
            layout->addWidget (button, 1);

            hide_all_button = new QPushButton (this);
            hide_all_button->setToolTip (tr ("Hide Tracks"));
            hide_all_button->setIcon (QIcon (":/hide.svg"));
            hide_all_button->setCheckable (true);
            connect (hide_all_button, SIGNAL (clicked()), this, SLOT (hide_all_slot ()));
            layout->addWidget (hide_all_button, 1);

            main_box->addLayout (layout, 0);

            tractogram_list_view = new QListView (this);
            tractogram_list_view->setSelectionMode (QAbstractItemView::ExtendedSelection);
            tractogram_list_view->setDragEnabled (true);
            tractogram_list_view->viewport()->setAcceptDrops (true);
            tractogram_list_view->setDropIndicatorShown (true);

            tractogram_list_model = new Model (this);
            tractogram_list_view->setModel (tractogram_list_model);

            connect (tractogram_list_model, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
                     this, SLOT (toggle_shown_slot (const QModelIndex&, const QModelIndex&)));

            connect (tractogram_list_view->selectionModel(),
                     SIGNAL (selectionChanged (const QItemSelection &, const QItemSelection &)),
                     SLOT (selection_changed_slot (const QItemSelection &, const QItemSelection &)));

            tractogram_list_view->setContextMenuPolicy (Qt::CustomContextMenu);
            connect (tractogram_list_view, SIGNAL (customContextMenuRequested (const QPoint&)),
                     this, SLOT (right_click_menu_slot (const QPoint&)));

            main_box->addWidget (tractogram_list_view, 1);

            QGridLayout* default_opt_grid = new QGridLayout;

            QSlider* slider;
            slider = new QSlider (Qt::Horizontal);
            slider->setRange (1,1000);
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

            QGroupBox* slab_group_box = new QGroupBox (tr("crop to slab"));
            slab_group_box->setCheckable (true);
            slab_group_box->setChecked (true);
            default_opt_grid->addWidget (slab_group_box, 2, 0, 1, 2);

            connect (slab_group_box, SIGNAL (clicked (bool)), this, SLOT (on_crop_to_slab_slot (bool)));

            QGridLayout* slab_layout = new QGridLayout;
            slab_group_box->setLayout(slab_layout);
            slab_layout->addWidget (new QLabel ("thickness (mm)"), 0, 0);
            slab_entry = new AdjustButton (this, 0.1);
            slab_entry->setValue (slab_thickness);
            slab_entry->setMin (0.0);
            connect (slab_entry, SIGNAL (valueChanged()), this, SLOT (on_slab_thickness_slot()));
            slab_layout->addWidget (slab_entry, 0, 1);

            QGroupBox* lighting_group_box = new QGroupBox (tr("lighting"));
            lighting_group_box->setCheckable (true);
            lighting_group_box->setChecked (false);
            default_opt_grid->addWidget (lighting_group_box, 3, 0, 1, 2);

            connect (lighting_group_box, SIGNAL (clicked (bool)), this, SLOT (on_use_lighting_slot (bool)));

            QVBoxLayout* lighting_layout = new QVBoxLayout (lighting_group_box);
            QPushButton* lighting_button = new QPushButton ("settings...");
            connect (lighting_button, SIGNAL (clicked()), this, SLOT (on_lighting_settings()));
            lighting_layout->addWidget (lighting_button);

            main_box->addLayout (default_opt_grid, 0);

            lighting = new GL::Lighting (parent); 
            connect (lighting, SIGNAL (changed()), SLOT (hide_all_slot()));


            QAction* action;
            track_option_menu = new QMenu ();
            action = new QAction("&Colour by direction", this);
            connect (action, SIGNAL(triggered()), this, SLOT (colour_track_by_direction_slot()));
            track_option_menu->addAction (action);
            action = new QAction("&Colour by track ends", this);
            connect (action, SIGNAL(triggered()), this, SLOT (colour_track_by_ends_slot()));
            track_option_menu->addAction (action);
            action = new QAction("&Randomise colour", this);
            connect (action, SIGNAL(triggered()), this, SLOT (randomise_track_colour_slot()));
            track_option_menu->addAction (action);
            action = new QAction("&Set colour", this);
            connect (action, SIGNAL(triggered()), this, SLOT (set_track_colour_slot()));
            track_option_menu->addAction (action);
            action = new QAction("&Colour by (track) scalar file", this);
            connect (action, SIGNAL(triggered()), this, SLOT (colour_by_scalar_file_slot()));
            track_option_menu->addAction (action);
        }


        Tractography::~Tractography () {}


        void Tractography::draw2D (const Projection& transform) {
          for (int i = 0; i < tractogram_list_model->rowCount(); ++i) {
            if (tractogram_list_model->items[i]->show && !hide_all_button->isChecked())
              dynamic_cast<Tractogram*>(tractogram_list_model->items[i])->render2D (transform);
          }
        }


        void Tractography::drawOverlays (const Projection& transform) {
          for (int i = 0; i < tractogram_list_model->rowCount(); ++i) {
            if (tractogram_list_model->items[i]->show)
              dynamic_cast<Tractogram*>(tractogram_list_model->items[i])->renderColourBar (transform);
          }
        }


        void Tractography::draw3D (const Projection& transform) {
        }


        void Tractography::tractogram_open_slot ()
        {
          std::vector<std::string> list = Dialog::File::get_files (this, "Select tractograms to open", "Tractograms (*.tck)");
          if (list.empty())
            return;
          try {
            tractogram_list_model->add_items (list, window, *this);
          }
          catch (Exception& E) {
            E.display();
          }
        }


        void Tractography::tractogram_close_slot ()
        {
          QModelIndexList indexes = tractogram_list_view->selectionModel()->selectedIndexes();
          while (indexes.size()) {
            tractogram_list_model->remove_item (indexes.first());
            indexes = tractogram_list_view->selectionModel()->selectedIndexes();
          }
          window.updateGL();
        }


        void Tractography::toggle_shown_slot (const QModelIndex& index, const QModelIndex& index2) {
          if (index.row() == index2.row()) {
            tractogram_list_view->setCurrentIndex(index);
          } else {
            for (size_t i = 0; i < tractogram_list_model->items.size(); ++i) {
              if (tractogram_list_model->items[i]->show) {
                tractogram_list_view->setCurrentIndex (tractogram_list_model->index (i, 0));
                break;
              }
            }
          }
          window.updateGL();
        }


        void Tractography::hide_all_slot () {
          window.updateGL();
        }


        void Tractography::on_crop_to_slab_slot (bool is_checked) {
          do_crop_to_slab = is_checked;
          window.updateGL();
        }

        void Tractography::on_use_lighting_slot (bool is_checked) {
          use_lighting = is_checked;
          window.updateGL();
        }

        void Tractography::on_lighting_settings () {
          if (!lighting_dialog)
            lighting_dialog = new Dialog::Lighting (&window, "Tractogram lighting", *lighting);
          lighting_dialog->show();
        }


        void Tractography::on_slab_thickness_slot() {
          slab_thickness = slab_entry->value();
          window.updateGL();
        }


        void Tractography::opacity_slot (int opacity) {
          line_opacity = Math::pow2(static_cast<float>(opacity)) / 1.0e6f;
          window.updateGL();
        }


        void Tractography::line_thickness_slot (int thickness) {
          line_thickness = static_cast<float>(thickness) / 200.0f;
          window.updateGL();
        }


        void Tractography::right_click_menu_slot (const QPoint& pos)
        {
          QModelIndex index = tractogram_list_view->indexAt (pos);
          if (index.isValid()) {
            QPoint globalPos = tractogram_list_view->mapToGlobal( pos);
            tractogram_list_view->selectionModel()->select(index, QItemSelectionModel::Select);
            track_option_menu->exec(globalPos);
          }
        }


        void Tractography::colour_track_by_direction_slot()
        {
          QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i)  {
            tractogram_list_model->get_tractogram (indices[i])->erase_nontrack_data();
            tractogram_list_model->get_tractogram (indices[i])->color_type = Direction;
          }
          window.updateGL();
        }
        
        
        void Tractography::colour_track_by_ends_slot()
        {
          QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            tractogram_list_model->get_tractogram (indices[i])->erase_nontrack_data();
            tractogram_list_model->get_tractogram (indices[i])->color_type = Ends;
            tractogram_list_model->get_tractogram (indices[i])->load_end_colours();
          }
          window.updateGL();
        }


        void Tractography::set_track_colour_slot()
        {
          QColor color;
          color = QColorDialog::getColor(Qt::red, this, "Select Color", QColorDialog::DontUseNativeDialog);
          float colour[] = {float(color.redF()), float(color.greenF()), float(color.blueF())};
          if (color.isValid()) {
            QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
            for (int i = 0; i < indices.size(); ++i) {
              tractogram_list_model->get_tractogram (indices[i])->color_type = Colour;
              tractogram_list_model->get_tractogram (indices[i])->set_colour (colour);
            }
          }
          window.updateGL();
        }


        void Tractography::randomise_track_colour_slot()
        {
          QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            float colour[3];
            Math::RNG rng;
            do {
              colour[0] = rng.uniform();
              colour[1] = rng.uniform();
              colour[2] = rng.uniform();
            } while (colour[0] < 0.5 && colour[1] < 0.5 && colour[2] < 0.5);
            dynamic_cast<Tractogram*> (tractogram_list_model->items[indices[i].row()])->erase_nontrack_data();
            dynamic_cast<Tractogram*> (tractogram_list_model->items[indices[i].row()])->color_type = Colour;
            dynamic_cast<Tractogram*> (tractogram_list_model->items[indices[i].row()])->set_colour (colour);
          }
          window.updateGL();
        }


        void Tractography::colour_by_scalar_file_slot()
        {
          QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
          if (indices.size() != 1) {
            QMessageBox msgBox;
            msgBox.setText("Please select only one tractogram when colouring by scalar file.    ");
            msgBox.exec();
          } else {
            if (!scalar_file_options) {
              scalar_file_options = Tool::create<TrackScalarFile> ("Scalar File Options", window);
            }
            dynamic_cast<TrackScalarFile*> (scalar_file_options->tool)->set_tractogram (tractogram_list_model->get_tractogram (indices[0]));
            if (dynamic_cast<Tractogram*> (tractogram_list_model->items[indices[0].row()])->scalar_filename.length() == 0) {
              if (!dynamic_cast<TrackScalarFile*> (scalar_file_options->tool)->open_track_scalar_file_slot())
                return;
            } else {
              dynamic_cast<Tractogram*> (tractogram_list_model->items[indices[0].row()])->erase_nontrack_data();
              dynamic_cast<Tractogram*> (tractogram_list_model->items[indices[0].row()])->color_type = ScalarFile;
            }
            scalar_file_options->show();
            window.updateGL();
          }
        }


        void Tractography::selection_changed_slot (const QItemSelection &, const QItemSelection &)
        {
          if (scalar_file_options) {
            QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
            if (indices.size() == 1) {
              dynamic_cast<TrackScalarFile*> (scalar_file_options->tool)->set_tractogram (tractogram_list_model->get_tractogram (indices[0]));
            } else {
              dynamic_cast<TrackScalarFile*> (scalar_file_options->tool)->set_tractogram (NULL);
            }
          }
        }

      }
    }
  }
}





