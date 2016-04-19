/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#include "mrtrix.h"
#include "gui/mrview/window.h"
#include "gui/mrview/tool/tractography/tractography.h"
#include "gui/dialog/file.h"
#include "gui/mrview/tool/list_model_base.h"
#include "gui/mrview/tool/tractography/track_scalar_file.h"
#include "gui/mrview/tool/tractography/tractogram.h"
#include "gui/opengl/lighting.h"
#include "gui/lighting_dock.h"

 // as a fraction of the image FOV:
#define MRTRIX_DEFAULT_LINE_THICKNESS 0.002f

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
                            Tractography& tractography_tool) {

              for (size_t i = 0; i < filenames.size(); ++i) {
                Tractogram* tractogram = new Tractogram (tractography_tool, filenames[i]);
                try {
                  tractogram->load_tracks();
                  beginInsertRows (QModelIndex(), items.size(), items.size() + 1);
                  items.push_back (std::unique_ptr<Displayable> (tractogram));
                  endInsertRows();
                } catch (Exception& e) {
                  delete tractogram;
                  e.display();
                }
              }
            }

            Tractogram* get_tractogram (QModelIndex& index) {
              return dynamic_cast<Tractogram*>(items[index.row()].get());
            }
        };


        Tractography::Tractography (Dock* parent) :
          Base (parent),
          line_thickness (MRTRIX_DEFAULT_LINE_THICKNESS),
          do_crop_to_slab (true),
          use_lighting (false),
          not_3D (true),
          line_opacity (1.0),
          scalar_file_options (nullptr),
          lighting_dock (nullptr) {

            float voxel_size;
            if (window().image()) {
              voxel_size = (window().image()->header().spacing(0) +
                            window().image()->header().spacing(1) +
                            window().image()->header().spacing(2)) / 3.0f;
            } else {
              voxel_size = 2.5;
            }

            slab_thickness  = 2 * voxel_size;

            VBoxLayout* main_box = new VBoxLayout (this);
            HBoxLayout* layout = new HBoxLayout;
            layout->setContentsMargins (0, 0, 0, 0);
            layout->setSpacing (0);

            QPushButton* button = new QPushButton (this);
            button->setToolTip (tr ("Open tractogram"));
            button->setIcon (QIcon (":/open.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (tractogram_open_slot ()));
            layout->addWidget (button, 1);

            button = new QPushButton (this);
            button->setToolTip (tr ("Close tractogram"));
            button->setIcon (QIcon (":/close.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (tractogram_close_slot ()));
            layout->addWidget (button, 1);

            hide_all_button = new QPushButton (this);
            hide_all_button->setToolTip (tr ("Hide all tractograms"));
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

            GridLayout* default_opt_grid = new GridLayout;

            QSlider* slider;
            slider = new QSlider (Qt::Horizontal);
            slider->setRange (1,1000);
            slider->setSliderPosition (1000);
            connect (slider, SIGNAL (valueChanged (int)), this, SLOT (opacity_slot (int)));
            default_opt_grid->addWidget (new QLabel ("opacity"), 0, 0);
            default_opt_grid->addWidget (slider, 0, 1);

            slider = new QSlider (Qt::Horizontal);
            slider->setRange (-1000,1000);
            slider->setSliderPosition (0);
            connect (slider, SIGNAL (valueChanged (int)), this, SLOT (line_thickness_slot (int)));
            default_opt_grid->addWidget (new QLabel ("line thickness"), 1, 0);
            default_opt_grid->addWidget (slider, 1, 1);

            default_opt_grid->addWidget (new QLabel ("color"), 2, 0);

            layout = new HBoxLayout;
            layout->setContentsMargins (0, 0, 0, 0);
            layout->setSpacing (0);

            colour_combobox = new ComboBoxWithErrorMsg (this, "(variable)");
            colour_combobox->setToolTip (tr ("Set how this tractogram will be colored"));
            colour_combobox->addItem ("Direction");
            colour_combobox->addItem ("Endpoints");
            colour_combobox->addItem ("Random");
            colour_combobox->addItem ("Manual");
            colour_combobox->addItem ("File");
            connect (colour_combobox, SIGNAL (activated(int)), this, SLOT (colour_mode_selection_slot (int)));
            layout->addWidget (colour_combobox);

            colour_button = new QColorButton;
            colour_button->setToolTip (tr ("Set the fixed colour to use for all tracks"));
            colour_button->setEnabled (false);
            connect (colour_button, SIGNAL (clicked()), this, SLOT (colour_button_slot()));
            layout->addWidget (colour_button);

            default_opt_grid->addLayout (layout, 2, 1, 1, 1);

            scalar_file_options = new TrackScalarFileOptions (std::string("Scalar file options"), this);
            scalar_file_options->setVisible (false);
            default_opt_grid->addWidget (scalar_file_options, 3, 0, 1, 2);

            QGroupBox* slab_group_box = new QGroupBox (tr("crop to slab"));
            slab_group_box->setCheckable (true);
            slab_group_box->setChecked (true);
            default_opt_grid->addWidget (slab_group_box, 4, 0, 1, 2);

            connect (slab_group_box, SIGNAL (clicked (bool)), this, SLOT (on_crop_to_slab_slot (bool)));

            GridLayout* slab_layout = new GridLayout;
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
            default_opt_grid->addWidget (lighting_group_box, 5, 0, 1, 2);

            connect (lighting_group_box, SIGNAL (clicked (bool)), this, SLOT (on_use_lighting_slot (bool)));

            VBoxLayout* lighting_layout = new VBoxLayout (lighting_group_box);
            QPushButton* lighting_button = new QPushButton ("settings...");
            connect (lighting_button, SIGNAL (clicked()), this, SLOT (on_lighting_settings()));
            lighting_layout->addWidget (lighting_button);

            main_box->addLayout (default_opt_grid, 0);

            lighting = new GL::Lighting (parent); 
            lighting->diffuse = 0.8;
            lighting->shine = 5.0;
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


        void Tractography::draw (const Projection& transform, bool is_3D, int, int)
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          not_3D = !is_3D;
          for (int i = 0; i < tractogram_list_model->rowCount(); ++i) {
            if (tractogram_list_model->items[i]->show && !hide_all_button->isChecked())
              dynamic_cast<Tractogram*>(tractogram_list_model->items[i].get())->render (transform);
          }
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }


        void Tractography::draw_colourbars ()
        {
          if(!scalar_file_options || hide_all_button->isChecked())
            return;

          for (int i = 0; i < tractogram_list_model->rowCount(); ++i) {
            if (tractogram_list_model->items[i]->show)
              dynamic_cast<Tractogram*>(tractogram_list_model->items[i].get())->request_render_colourbar (*scalar_file_options);
          }
        }



        size_t Tractography::visible_number_colourbars () {
           size_t total_visible(0);

           if(scalar_file_options && !hide_all_button->isChecked()) {
             for (size_t i = 0, N = tractogram_list_model->rowCount(); i < N; ++i) {
               Tractogram* tractogram = dynamic_cast<Tractogram*>(tractogram_list_model->items[i].get());
               if (tractogram && tractogram->show && tractogram->intensity_scalar_filename.length())
                 total_visible += 1;
             }
           }

           return total_visible;
        }



        void Tractography::tractogram_open_slot ()
        {
          std::vector<std::string> list = Dialog::File::get_files (this, "Select tractograms to open", "Tractograms (*.tck)");
          if (list.empty())
            return;
          try {
            tractogram_list_model->add_items (list, *this);
            window().updateGL();
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
          window().updateGL();
        }


        void Tractography::toggle_shown_slot (const QModelIndex& index, const QModelIndex& index2) 
        {
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
          window().updateGL();
        }


        void Tractography::hide_all_slot ()
        {
          window().updateGL();
        }


        void Tractography::on_crop_to_slab_slot (bool is_checked)
        {
          do_crop_to_slab = is_checked;

          for (size_t i = 0, N = tractogram_list_model->rowCount(); i < N; ++i) {
            Tractogram* tractogram = dynamic_cast<Tractogram*>(tractogram_list_model->items[i].get());
            tractogram->should_update_stride = true;
          }

          window().updateGL();
        }


        void Tractography::on_use_lighting_slot (bool is_checked)
        {
          use_lighting = is_checked;
          window().updateGL();
        }


        void Tractography::on_lighting_settings ()
        {
          if (!lighting_dock) {
            lighting_dock = new LightingDock("Tractogram lighting", *lighting);
            window().addDockWidget (Qt::RightDockWidgetArea, lighting_dock);
          }
          lighting_dock->show();
        }


        void Tractography::on_slab_thickness_slot()
        {
          slab_thickness = slab_entry->value();
          window().updateGL();
        }


        void Tractography::opacity_slot (int opacity)
        {
          line_opacity = Math::pow2(static_cast<float>(opacity)) / 1.0e6f;
          window().updateGL();
        }


        void Tractography::line_thickness_slot (int thickness)
        {
          line_thickness = MRTRIX_DEFAULT_LINE_THICKNESS * std::exp (2.0e-3f * thickness);

          for (size_t i = 0, N = tractogram_list_model->rowCount(); i < N; ++i) {
            Tractogram* tractogram = dynamic_cast<Tractogram*>(tractogram_list_model->items[i].get());
            tractogram->should_update_stride = true;
          }

          window().updateGL();
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
          colour_combobox->blockSignals (true);
          colour_combobox->setCurrentIndex (0);
          colour_combobox->blockSignals (false);
          colour_button->setEnabled (false);
          scalar_file_options->setVisible (false);
          window().updateGL();
        }
        
        
        void Tractography::colour_track_by_ends_slot()
        {
          QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            tractogram_list_model->get_tractogram (indices[i])->erase_nontrack_data();
            tractogram_list_model->get_tractogram (indices[i])->color_type = Ends;
            tractogram_list_model->get_tractogram (indices[i])->load_end_colours();
          }
          colour_combobox->blockSignals (true);
          colour_combobox->setCurrentIndex (1);
          colour_combobox->blockSignals (false);
          colour_button->setEnabled (false);
          scalar_file_options->setVisible (false);
          window().updateGL();
        }


        void Tractography::randomise_track_colour_slot()
        {
          QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            float colour[3];
            Math::RNG::Uniform<float> rng;
            do {
              colour[0] = rng();
              colour[1] = rng();
              colour[2] = rng();
            } while (colour[0] < 0.5 && colour[1] < 0.5 && colour[2] < 0.5);
            dynamic_cast<Tractogram*> (tractogram_list_model->items[indices[i].row()].get())->erase_nontrack_data();
            dynamic_cast<Tractogram*> (tractogram_list_model->items[indices[i].row()].get())->color_type = Manual;
            dynamic_cast<Tractogram*> (tractogram_list_model->items[indices[i].row()].get())->set_colour (colour);
            if (!i)
              colour_button->setColor (QColor (colour[0]*255.0f, colour[1]*255.0f, colour[2]*255.0f));
          }
          colour_combobox->blockSignals (true);
          colour_combobox->setCurrentIndex (2);
          colour_combobox->blockSignals (false);
          colour_button->setEnabled (true);
          scalar_file_options->setVisible (false);
          window().updateGL();
        }


        void Tractography::set_track_colour_slot()
        {
          QColor color;
          color = QColorDialog::getColor(Qt::red, this, "Select Color", QColorDialog::DontUseNativeDialog);
          float colour[] = {float(color.redF()), float(color.greenF()), float(color.blueF())};
          if (color.isValid()) {
            QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
            for (int i = 0; i < indices.size(); ++i) {
              tractogram_list_model->get_tractogram (indices[i])->erase_nontrack_data();
              tractogram_list_model->get_tractogram (indices[i])->color_type = Manual;
              tractogram_list_model->get_tractogram (indices[i])->set_colour (colour);
            }
            colour_combobox->blockSignals (true);
            colour_combobox->setCurrentIndex (3);
            colour_combobox->blockSignals (false);
            colour_button->setEnabled (true);
            colour_button->setColor (QColor (colour[0]*255.0f, colour[1]*255.0f, colour[2]*255.0f));
            scalar_file_options->setVisible (false);
          }
          window().updateGL();
        }


        void Tractography::colour_by_scalar_file_slot()
        {
          QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
          colour_button->setEnabled (false);
          if (indices.size() != 1) {
            QMessageBox msgBox;
            msgBox.setText("Please select only one tractogram when colouring by scalar file.    ");
            msgBox.exec();
            scalar_file_options->setVisible (false);
          } else {
            scalar_file_options->set_tractogram (tractogram_list_model->get_tractogram (indices[0]));
            if (dynamic_cast<Tractogram*> (tractogram_list_model->items[indices[0].row()].get())->intensity_scalar_filename.length() == 0) {
              if (!scalar_file_options->open_intensity_track_scalar_file_slot())
                return;
            } else {
              dynamic_cast<Tractogram*> (tractogram_list_model->items[indices[0].row()].get())->erase_nontrack_data();
              dynamic_cast<Tractogram*> (tractogram_list_model->items[indices[0].row()].get())->color_type = ScalarFile;
            }
            colour_combobox->blockSignals (true);
            colour_combobox->setCurrentIndex (4);
            colour_combobox->blockSignals (false);
            colour_button->setEnabled (false);
            scalar_file_options->setVisible (true);
            window().updateGL();
          }
        }


        void Tractography::colour_mode_selection_slot (int /*unused*/)
        {
          switch (colour_combobox->currentIndex()) {
            case 0: colour_track_by_direction_slot(); break;
            case 1: colour_track_by_ends_slot(); break;
            case 2: randomise_track_colour_slot(); break;
            case 3: set_track_colour_slot(); break;
            case 4: colour_by_scalar_file_slot(); break;
            case 5: break;
            default: assert (0);
          }
        }


        void Tractography::colour_button_slot()
        {
          // Button brings up its own colour prompt; if set_track_colour_slot()
          //   were to be called, this would present its own selection prompt
          // Need to instead set the colours here explicitly
          const QColor color = colour_button->color();
          if (color.isValid()) {
            QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
            float c[3] = { color.red()/255.0f, color.green()/255.0f, color.blue()/255.0f };
            for (int i = 0; i < indices.size(); ++i)
              tractogram_list_model->get_tractogram (indices[i])->set_colour (c);
            colour_combobox->blockSignals (true);
            colour_combobox->setCurrentIndex (3); // In case it was on random
            colour_combobox->blockSignals (false);
            window().updateGL();
          }
        }


        void Tractography::selection_changed_slot (const QItemSelection &, const QItemSelection &)
        {
          QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
          if (scalar_file_options) {
            if (indices.size() == 1) {
              scalar_file_options->set_tractogram (tractogram_list_model->get_tractogram (indices[0]));
            } else {
              scalar_file_options->set_tractogram (nullptr);
            }
          }
          if (!indices.size()) {
            colour_combobox->setEnabled (false);
            colour_button->setEnabled (false);
            return;
          }
          colour_combobox->setEnabled (true);
          TrackColourType color_type = tractogram_list_model->get_tractogram (indices[0])->color_type;
          Eigen::Array3f color = tractogram_list_model->get_tractogram (indices[0])->colour;
          bool color_type_consistent = true;
          for (int i = 1; i != indices.size(); ++i) {
            const Tractogram* tractogram = tractogram_list_model->get_tractogram (indices[i]);
            if (tractogram->color_type != color_type) {
              color_type_consistent = false;
              break;
            }
          }
          if (color_type_consistent) {
            colour_combobox->blockSignals (true);
            switch (color_type) {
              case Direction:
                colour_combobox->setCurrentIndex (0);
                colour_button->setEnabled (false);
                break;
              case Ends:
                colour_combobox->setCurrentIndex (1);
                colour_button->setEnabled (false);
                break;
              case Manual:
                colour_combobox->setCurrentIndex (3);
                colour_button->setEnabled (true);
                colour_button->setColor (QColor (color[0]*255.0f, color[1]*255.0f, color[2]*255.0f));
                break;
              case ScalarFile:
                colour_combobox->setCurrentIndex (4);
                colour_button->setEnabled (false);
                break;
            }
            colour_combobox->blockSignals (false);
          } else {
            colour_combobox->setError();
          }
        }




        void Tractography::add_commandline_options (MR::App::OptionList& options) 
        { 
          using namespace MR::App;
          options
            + OptionGroup ("Tractography tool options")

            + Option ("tractography.load", "Load the specified tracks file into the tractography tool.").allow_multiple()
            +   Argument ("tracks").type_file_in();
        }

        bool Tractography::process_commandline_option (const MR::App::ParsedOption& opt) 
        {
          if (opt.opt->is ("tractography.load")) {
            std::vector<std::string> list (1, std::string(opt[0]));
            try { 
              tractogram_list_model->add_items (list, *this); 
              window().updateGL();
            }
            catch (Exception& E) { E.display(); }
            return true;
          }

          return false;
        }



      }
    }
  }
}





