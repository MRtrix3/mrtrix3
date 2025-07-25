/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include "mrtrix.h"
#include "gui/mrview/window.h"
#include "gui/mrview/tool/tractography/tractography.h"
#include "gui/dialog/file.h"
#include "gui/mrview/tool/list_model_base.h"
#include "gui/mrview/tool/tractography/track_scalar_file.h"
#include "gui/mrview/tool/tractography/tractogram.h"
#include "gui/opengl/lighting.h"
#include "gui/lighting_dock.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {
        const char* tractogram_geometry_types[] = { "pseudotubes", "lines", "points", nullptr };

        TrackGeometryType geometry_index2type (const int idx)
        {
          switch (idx) {
            case 0: return TrackGeometryType::Pseudotubes;
            case 1: return TrackGeometryType::Lines;
            case 2: return TrackGeometryType::Points;
            default: assert (0); return TrackGeometryType::Pseudotubes;
          }
        }

        size_t geometry_string2index (std::string type_str)
        {
          type_str = lowercase (type_str);
          size_t index = 0;
          for (const char* const* p = tractogram_geometry_types; *p; ++p, ++index) {
            if (type_str == *p)
              return index;
          }
          throw Exception ("Unrecognised value for tractogram geometry \"" + type_str + "\" (options are: " + join(tractogram_geometry_types, ", ") + "); ignoring");
          return 0;
        }



        class Tractography::Model : public ListModelBase
        { MEMALIGN(Tractography::Model)

          public:
            Model (QObject* parent) :
              ListModelBase (parent) { }

            void add_items (vector<std::string>& filenames,
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
            HBoxLayout* hlayout = new HBoxLayout;
            hlayout->setContentsMargins (0, 0, 0, 0);
            hlayout->setSpacing (0);

            QPushButton* button = new QPushButton (this);
            button->setToolTip (tr ("Open tractogram"));
            button->setIcon (QIcon (":/open.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (tractogram_open_slot ()));
            hlayout->addWidget (button, 1);

            button = new QPushButton (this);
            button->setToolTip (tr ("Close tractogram"));
            button->setIcon (QIcon (":/close.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (tractogram_close_slot ()));
            hlayout->addWidget (button, 1);

            hide_all_button = new QPushButton (this);
            hide_all_button->setToolTip (tr ("Hide all tractograms"));
            hide_all_button->setIcon (QIcon (":/hide.svg"));
            hide_all_button->setCheckable (true);
            connect (hide_all_button, SIGNAL (clicked()), this, SLOT (hide_all_slot ()));
            hlayout->addWidget (hide_all_button, 1);

            main_box->addLayout (hlayout, 0);

            tractogram_list_view = new QListView (this);
            tractogram_list_view->setSelectionMode (QAbstractItemView::ExtendedSelection);
            tractogram_list_view->setDragEnabled (true);
            tractogram_list_view->setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
            tractogram_list_view->setTextElideMode (Qt::ElideLeft);
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

            hlayout = new HBoxLayout;
            hlayout->setContentsMargins (0, 0, 0, 0);
            hlayout->setSpacing (0);

            hlayout->addWidget (new QLabel ("color"));

            colour_combobox = new ComboBoxWithErrorMsg (this, "(variable)");
            colour_combobox->setToolTip (tr ("Set how this tractogram will be colored"));
            colour_combobox->addItem ("Direction");
            colour_combobox->addItem ("Endpoints");
            colour_combobox->addItem ("Random");
            colour_combobox->addItem ("Manual");
            colour_combobox->addItem ("File");
            colour_combobox->setEnabled (false);
            connect (colour_combobox, SIGNAL (activated(int)), this, SLOT (colour_mode_selection_slot (int)));
            hlayout->addWidget (colour_combobox);

            colour_button = new QColorButton;
            colour_button->setToolTip (tr ("Set the fixed colour to use for all tracks"));
            colour_button->setEnabled (false);
            connect (colour_button, SIGNAL (clicked()), this, SLOT (colour_button_slot()));
            hlayout->addWidget (colour_button);

            main_box->addLayout (hlayout);

            hlayout = new HBoxLayout;
            hlayout->setContentsMargins (0, 0, 0, 0);
            hlayout->setSpacing (0);

            hlayout->addWidget (new QLabel ("geometry"));

            geom_type_combobox = new ComboBoxWithErrorMsg (this, "(variable)");
            geom_type_combobox->setToolTip (tr ("Set the tractogram geometry type"));
            geom_type_combobox->addItem ("Pseudotubes");
            geom_type_combobox->addItem ("Lines");
            geom_type_combobox->addItem ("Points");
            connect (geom_type_combobox, SIGNAL (activated(int)), this, SLOT (geom_type_selection_slot (int)));
            hlayout->addWidget (geom_type_combobox);

            main_box->addLayout (hlayout);

            hlayout = new HBoxLayout;
            hlayout->setContentsMargins (0, 0, 0, 0);
            hlayout->setSpacing (0);

            thickness_label = new QLabel ("thickness");
            hlayout->addWidget (thickness_label);

            thickness_slider = new QSlider (Qt::Horizontal);
            thickness_slider->setRange (-1000,1000);
            thickness_slider->setSliderPosition (0);
            connect (thickness_slider, SIGNAL (valueChanged (int)), this, SLOT (line_thickness_slot (int)));
            hlayout->addWidget (thickness_slider);

            main_box->addLayout (hlayout);

            scalar_file_options = new TrackScalarFileOptions (this);
            main_box->addWidget (scalar_file_options);

            QGroupBox* general_groupbox = new QGroupBox ("General options");
            GridLayout* general_opt_grid = new GridLayout;
            general_opt_grid->setContentsMargins (0, 0, 0, 0);
            general_opt_grid->setSpacing (0);

            general_groupbox->setLayout (general_opt_grid);

            opacity_slider = new QSlider (Qt::Horizontal);
            opacity_slider->setRange (1,1000);
            opacity_slider->setSliderPosition (1000);
            connect (opacity_slider, SIGNAL (valueChanged (int)), this, SLOT (opacity_slot (int)));
            general_opt_grid->addWidget (new QLabel ("opacity"), 0, 0);
            general_opt_grid->addWidget (opacity_slider, 0, 1);

            slab_group_box = new QGroupBox (tr("crop to slab"));
            slab_group_box->setCheckable (true);
            slab_group_box->setChecked (true);
            general_opt_grid->addWidget (slab_group_box, 4, 0, 1, 2);

            connect (slab_group_box, SIGNAL (clicked (bool)), this, SLOT (on_crop_to_slab_slot (bool)));

            GridLayout* slab_layout = new GridLayout;
            slab_group_box->setLayout(slab_layout);
            slab_layout->addWidget (new QLabel ("thickness (mm)"), 0, 0);
            slab_entry = new AdjustButton (this, 0.1);
            slab_entry->setValue (slab_thickness);
            slab_entry->setMin (0.0);
            connect (slab_entry, SIGNAL (valueChanged()), this, SLOT (on_slab_thickness_slot()));
            slab_layout->addWidget (slab_entry, 0, 1);

            lighting_group_box = new QGroupBox (tr("use lighting"));
            lighting_group_box->setCheckable (true);
            lighting_group_box->setChecked (false);
            general_opt_grid->addWidget (lighting_group_box, 5, 0, 1, 2);

            connect (lighting_group_box, SIGNAL (clicked (bool)), this, SLOT (on_use_lighting_slot (bool)));

            VBoxLayout* lighting_layout = new VBoxLayout (lighting_group_box);
            lighting_button = new QPushButton ("Track lighting...");
            lighting_button->setIcon (QIcon (":/light.svg"));
            connect (lighting_button, SIGNAL (clicked()), this, SLOT (on_lighting_settings()));
            lighting_layout->addWidget (lighting_button);

            main_box->addWidget (general_groupbox, 0);

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

            //CONF option: MRViewDefaultTractGeomType
            //CONF default: Pseudotubes
            //CONF The default geometry type used to render tractograms.
            //CONF Options are Pseudotubes, Lines or Points
            const std::string default_geom_type = File::Config::get ("MRViewDefaultTractGeomType", tractogram_geometry_types[0]);
            try {
              const size_t default_geom_index = geometry_string2index (default_geom_type);
              Tractogram::default_tract_geom = geometry_index2type (default_geom_index);
              geom_type_combobox->setCurrentIndex (default_geom_index);
            } catch (Exception& e) {
              e.display();
            }

            // In the instance where pseudotubes are _not_ the default, enable lighting by default
            if (Tractogram::default_tract_geom != TrackGeometryType::Pseudotubes) {
              use_lighting = true;
              lighting_group_box->setChecked (true);
            }

            update_geometry_type_gui();
        }


        Tractography::~Tractography () {}


        void Tractography::draw (const Projection& transform, bool is_3D, int, int)
        {
          GL::assert_context_is_current();
          not_3D = !is_3D;
          for (int i = 0; i < tractogram_list_model->rowCount(); ++i) {
            Tractogram* tractogram = dynamic_cast<Tractogram*>(tractogram_list_model->items[i].get());
            if (tractogram->show && !hide_all_button->isChecked())
              tractogram->render (transform);
          }
          GL::assert_context_is_current();
        }


        void Tractography::draw_colourbars ()
        {
          if (hide_all_button->isChecked())
            return;

          for (int i = 0; i < tractogram_list_model->rowCount(); ++i) {
            Tractogram* tractogram = dynamic_cast<Tractogram*>(tractogram_list_model->items[i].get());
            if (tractogram->show && tractogram->get_color_type() == TrackColourType::ScalarFile && tractogram->intensity_scalar_filename.length())
              tractogram->request_render_colourbar (*scalar_file_options);
          }
        }



        size_t Tractography::visible_number_colourbars () {
           size_t total_visible(0);

           if (!hide_all_button->isChecked()) {
             for (size_t i = 0, N = tractogram_list_model->rowCount(); i < N; ++i) {
               Tractogram* tractogram = dynamic_cast<Tractogram*>(tractogram_list_model->items[i].get());
               if (tractogram->show && tractogram->get_color_type() == TrackColourType::ScalarFile && tractogram->intensity_scalar_filename.length())
                 total_visible += 1;
             }
           }

           return total_visible;
        }



        void Tractography::tractogram_open_slot ()
        {

          vector<std::string> list = Dialog::File::get_files (this, "Select tractograms to open", "Tractograms (*.tck)", &current_folder);
          add_tractogram(list);
        }





        void Tractography::add_tractogram (vector<std::string>& list)
        {
          if (list.empty())
          { return; }
          try {
            tractogram_list_model->add_items (list, *this);
            select_last_added_tractogram();
          }
          catch (Exception& E) {
            E.display();
          }

        }





        void Tractography::dropEvent (QDropEvent* event)
        {
          static constexpr int max_files = 32;

          const QMimeData* mimeData = event->mimeData();
          if (mimeData->hasUrls()) {
            vector<std::string> list;
            QList<QUrl> urlList = mimeData->urls();
            for (int i = 0; i < urlList.size() && i < max_files; ++i) {
                list.push_back (urlList.at (i).path().toUtf8().constData());
            }
            try {
              tractogram_list_model->add_items (list, *this);
              window().updateGL();
            }
            catch (Exception& e) {
              e.display();
            }
            event->acceptProposedAction();
          }
        }


        void Tractography::tractogram_close_slot ()
        {
          GL::Context::Grab context;
          QModelIndexList indexes = tractogram_list_view->selectionModel()->selectedIndexes();
          while (indexes.size()) {
            tractogram_list_model->remove_item (indexes.first());
            indexes = tractogram_list_view->selectionModel()->selectedIndexes();
          }
          scalar_file_options->set_tractogram (nullptr);
          scalar_file_options->update_UI();
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
          QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i)  {
            Tractogram* tractogram = tractogram_list_model->get_tractogram (indices[i]);
            tractogram->line_thickness = thickness;
            tractogram->should_update_stride = true;
          }

          window().updateGL();
        }


        void Tractography::right_click_menu_slot (const QPoint& pos)
        {
          QModelIndex index = tractogram_list_view->indexAt (pos);
          if (index.isValid()) {
            QPoint globalPos = tractogram_list_view->mapToGlobal (pos);
            tractogram_list_view->selectionModel()->select (index, QItemSelectionModel::Select);
            track_option_menu->exec (globalPos);
          }
        }


        void Tractography::colour_track_by_direction_slot()
        {
          QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i)  {
            Tractogram* tractogram = tractogram_list_model->get_tractogram (indices[i]);
            tractogram->set_color_type (TrackColourType::Direction);
            if (tractogram->get_threshold_type() == TrackThresholdType::UseColourFile)
              tractogram->set_threshold_type (TrackThresholdType::None);
          }
          colour_combobox->blockSignals (true);
          colour_combobox->setCurrentIndex (0);
          colour_combobox->clearError();
          colour_combobox->blockSignals (false);
          colour_button->setEnabled (false);
          update_scalar_options();
          window().updateGL();
        }


        void Tractography::colour_track_by_ends_slot()
        {
          QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            Tractogram* tractogram = tractogram_list_model->get_tractogram (indices[i]);
            tractogram->set_color_type (TrackColourType::Ends);
            tractogram->load_end_colours();
            if (tractogram->get_threshold_type() == TrackThresholdType::UseColourFile)
              tractogram->set_threshold_type (TrackThresholdType::None);
          }
          colour_combobox->blockSignals (true);
          colour_combobox->setCurrentIndex (1);
          colour_combobox->clearError();
          colour_combobox->blockSignals (false);
          colour_button->setEnabled (false);
          update_scalar_options();
          window().updateGL();
        }


        void Tractography::randomise_track_colour_slot()
        {
          QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            Tractogram* tractogram = tractogram_list_model->get_tractogram (indices[i]);
            float colour[3];
            Math::RNG::Uniform<float> rng;
            do {
              colour[0] = rng();
              colour[1] = rng();
              colour[2] = rng();
            } while (colour[0] < 0.5 && colour[1] < 0.5 && colour[2] < 0.5);
            tractogram->set_color_type (TrackColourType::Manual);
            QColor c (colour[0]*255.0f, colour[1]*255.0f, colour[2]*255.0f);
            tractogram->set_colour (c);
            if (tractogram->get_threshold_type() == TrackThresholdType::UseColourFile)
              tractogram->set_threshold_type (TrackThresholdType::None);
            if (!i)
              colour_button->setColor (c);
          }
          colour_combobox->blockSignals (true);
          colour_combobox->setCurrentIndex (2);
          colour_combobox->clearError();
          colour_combobox->blockSignals (false);
          colour_button->setEnabled (true);
          update_scalar_options();
          window().updateGL();
        }


        void Tractography::set_track_colour_slot()
        {
          QColor color;
          color = QColorDialog::getColor(Qt::red, this, "Select Color", QColorDialog::DontUseNativeDialog);
          if (color.isValid()) {
            QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
            for (int i = 0; i < indices.size(); ++i) {
              Tractogram* tractogram = tractogram_list_model->get_tractogram (indices[i]);
              tractogram->set_color_type (TrackColourType::Manual);
              tractogram->set_colour (color);
              if (tractogram->get_threshold_type() == TrackThresholdType::UseColourFile)
                tractogram->set_threshold_type (TrackThresholdType::None);
            }
            colour_combobox->blockSignals (true);
            colour_combobox->setCurrentIndex (3);
            colour_combobox->clearError();
            colour_combobox->blockSignals (false);
            colour_button->setEnabled (true);
            colour_button->setColor (color);
            update_scalar_options();
          }
          window().updateGL();
        }


        void Tractography::colour_by_scalar_file_slot()
        {
          QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
          if (indices.size() != 1) {
            // User may have accessed this from the context menu
            QMessageBox::warning (QApplication::activeWindow(),
                                  tr ("Tractogram colour error"),
                                  tr ("Cannot set multiple tractograms to use the same file for streamline colouring"),
                                  QMessageBox::Ok,
                                  QMessageBox::Ok);
            return;
          }

          Tractogram* tractogram = tractogram_list_model->get_tractogram (indices[0]);
          scalar_file_options->set_tractogram (tractogram);
          if (tractogram->intensity_scalar_filename.empty()) {
            if (!scalar_file_options->open_intensity_track_scalar_file_slot()) {
              colour_combobox->blockSignals (true);
              switch (tractogram->get_color_type()) {
                case TrackColourType::Direction:  colour_combobox->setCurrentIndex (0); break;
                case TrackColourType::Ends:       colour_combobox->setCurrentIndex (1); break;
                case TrackColourType::Manual:     colour_combobox->setCurrentIndex (3); break;
                case TrackColourType::ScalarFile: colour_combobox->setCurrentIndex (4); break;
              }
              colour_combobox->clearError();
              colour_combobox->blockSignals (false);
              return;
            }
          }
          tractogram->set_color_type (TrackColourType::ScalarFile);
          colour_combobox->blockSignals (true);
          colour_combobox->setCurrentIndex (4);
          colour_combobox->clearError();
          colour_combobox->blockSignals (false);
          colour_button->setEnabled (false);
          update_scalar_options();
          window().updateGL();
        }


        void Tractography::colour_mode_selection_slot (int)
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
            for (int i = 0; i < indices.size(); ++i)
              tractogram_list_model->get_tractogram (indices[i])->set_colour (color);
            colour_combobox->blockSignals (true);
            colour_combobox->setCurrentIndex (3); // In case it was on random
            colour_combobox->clearError();
            colour_combobox->blockSignals (false);
            window().updateGL();
          }
        }



        void Tractography::geom_type_selection_slot (int selected_index)
        {
          // Combo box shows the "(variable)" message, and the user has
          //   re-clicked on it -> nothing to do
          if (selected_index == 3)
            return;

          TrackGeometryType geom_type = geometry_index2type (selected_index);

          QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i)
            tractogram_list_model->get_tractogram (indices[i])->set_geometry_type (geom_type);

          update_geometry_type_gui();

          window().updateGL();
        }



        void Tractography::selection_changed_slot (const QItemSelection &, const QItemSelection &)
        {
          update_scalar_options();
          update_geometry_type_gui();

          QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
          if (!indices.size()) {
            colour_combobox->setEnabled (false);
            colour_button->setEnabled (false);
            return;
          }
          colour_combobox->setEnabled (true);

          const Tractogram* first_tractogram = tractogram_list_model->get_tractogram (indices[0]);

          TrackColourType color_type = first_tractogram->get_color_type();
          QColor color (first_tractogram->colour[0], first_tractogram->colour[1], first_tractogram->colour[2]);
          TrackGeometryType geom_type = first_tractogram->get_geometry_type();
          bool color_type_consistent = true, geometry_type_consistent = true;
          float mean_thickness = first_tractogram->line_thickness;
          for (int i = 1; i != indices.size(); ++i) {
            const Tractogram* tractogram = tractogram_list_model->get_tractogram (indices[i]);
            if (tractogram->get_color_type() != color_type)
              color_type_consistent = false;
            if (tractogram->get_geometry_type() != geom_type)
              geometry_type_consistent = false;
            mean_thickness += tractogram->line_thickness;
          }
          if (color_type_consistent) {
            colour_combobox->blockSignals (true);
            switch (color_type) {
              case TrackColourType::Direction:
                colour_combobox->setCurrentIndex (0);
                colour_button->setEnabled (false);
                break;
              case TrackColourType::Ends:
                colour_combobox->setCurrentIndex (1);
                colour_button->setEnabled (false);
                break;
              case TrackColourType::Manual:
                colour_combobox->setCurrentIndex (3);
                colour_button->setEnabled (true);
                colour_button->setColor (color);
                break;
              case TrackColourType::ScalarFile:
                colour_combobox->setCurrentIndex (4);
                colour_button->setEnabled (false);
                break;
            }
            colour_combobox->clearError();
            colour_combobox->blockSignals (false);
          } else {
            colour_combobox->setError();
          }

          if (geometry_type_consistent) {
            geom_type_combobox->blockSignals (true);
            switch (geom_type) {
              case TrackGeometryType::Pseudotubes:
                geom_type_combobox->setCurrentIndex (0);
                break;
              case TrackGeometryType::Lines:
                geom_type_combobox->setCurrentIndex (1);
                break;
              case TrackGeometryType::Points:
                geom_type_combobox->setCurrentIndex (2);
                break;
            }
            geom_type_combobox->clearError();
            geom_type_combobox->blockSignals (false);
          } else {
            geom_type_combobox->setError();
          }

          thickness_slider->blockSignals (true);
          thickness_slider->setSliderPosition (mean_thickness / float(indices.size()));
          thickness_slider->blockSignals (false);
        }



        void Tractography::update_scalar_options()
        {
          QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
          if (indices.size() == 1)
            scalar_file_options->set_tractogram (tractogram_list_model->get_tractogram (indices[0]));
          else
            scalar_file_options->set_tractogram (nullptr);
          scalar_file_options->update_UI();
        }



        void Tractography::update_geometry_type_gui()
        {
          thickness_slider->setHidden (true);
          thickness_label->setHidden (true);
          lighting_button->setEnabled (false);
          lighting_group_box->setEnabled (false);
          geom_type_combobox->setEnabled (false);

          QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
          if (!indices.size())
            return;

          geom_type_combobox->setEnabled (true);

          const Tractogram* first_tractogram = tractogram_list_model->get_tractogram (indices[0]);
          const TrackGeometryType geom_type = first_tractogram->get_geometry_type();

          if (geom_type == TrackGeometryType::Pseudotubes || geom_type == TrackGeometryType::Points) {
            thickness_slider->setHidden (false);
            thickness_label->setHidden (false);
            lighting_button->setEnabled (true);
            lighting_group_box->setEnabled (true);
          }

        }







        void Tractography::add_commandline_options (MR::App::OptionList& options)
        {
          using namespace MR::App;
          options
            + OptionGroup ("Tractography tool options")

            + Option ("tractography.load", "Load the specified tracks file into the tractography tool.").allow_multiple()
            +   Argument ("tracks").type_file_in()

            + Option ("tractography.thickness", "Line thickness of tractography display, [-1.0, 1.0], default is 0.0.").allow_multiple()
            +   Argument ("value").type_float ( -1.0, 1.0 )

            + Option ("tractography.geometry", "The geometry type to use when rendering tractograms (options are: " + join(tractogram_geometry_types, ", ") + ")").allow_multiple()
            +   Argument ("value").type_choice (tractogram_geometry_types)

            + Option ("tractography.opacity", "Opacity of tractography display, [0.0, 1.0], default is 1.0.").allow_multiple()
            +   Argument ("value").type_float ( 0.0, 1.0 )

            + Option ("tractography.slab", "Slab thickness of tractography display, in mm. -1 to turn off crop to slab.").allow_multiple()
            +   Argument ("value").type_float(-1, 1e6)

            + Option ("tractography.lighting", "Toggle the use of lighting of tractogram geometry").allow_multiple()
            +  Argument ("value").type_bool()

            + Option ("tractography.colour", "Specify a manual colour for the tractogram, as three comma-separated values").allow_multiple()
            +   Argument ("R,G,B").type_sequence_float()

            + Option ("tractography.tsf_load", "Load the specified tractography scalar file.").allow_multiple()
            +  Argument ("tsf").type_file_in()

            + Option ("tractography.tsf_range", "Set range for the tractography scalar file. Requires -tractography.tsf_load already provided.").allow_multiple()
            +  Argument ("RangeMin,RangeMax").type_sequence_float()

            + Option ("tractography.tsf_thresh", "Set thresholds for the tractography scalar file. Requires -tractography.tsf_load already provided.").allow_multiple()
            +  Argument ("ThresholdMin,ThresholdMax").type_sequence_float()

            + Option ("tractography.tsf_colourmap", "Sets the colourmap of the .tsf file as indexed in the tsf colourmap dropdown menu. Requires -tractography.tsf_load already.").allow_multiple()
            +   Argument ("index").type_integer();

        }

        /*
          Selects the last tractogram in the tractogram_list_view and updates the window. If no tractograms are in the list view, no action is taken.
        */
        void Tractography::select_last_added_tractogram()
        {
          int count = tractogram_list_model->rowCount();
          if(count != 0){
            QModelIndex index = tractogram_list_view->model()->index(count-1, 0);
            tractogram_list_view->setCurrentIndex(index);
            window().updateGL();
          }
        }


        bool Tractography::process_commandline_option (const MR::App::ParsedOption& opt)
        {

          if (opt.opt->is ("tractography.load"))
          {
            vector<std::string> list (1, std::string(opt[0]));
            add_tractogram(list);
            return true;
          }


          if (opt.opt->is ("tractography.tsf_load"))
          {
            try {

              if (process_commandline_option_tsf_check_tracto_loaded()) {
                QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();

                if (indices.size() == 1) {//just in case future edits break this assumption
                  Tractogram* tractogram = tractogram_list_model->get_tractogram (indices[0]);

                  //set its tsf filename and load the tsf file
                  scalar_file_options->set_tractogram (tractogram);
                  scalar_file_options->open_intensity_track_scalar_file_slot (std::string(opt[0]));

                  //Set the GUI to use the file for visualisation
                  colour_combobox->setCurrentIndex(4); // Set combobox to "File"
                }
              }
            }
            catch (Exception& E) { E.display(); }

            return true;
          }

          if (opt.opt->is ("tractography.tsf_range"))
          {
            try {
              //Set the tsf visualisation range
              vector<default_type> range;
              if (process_commandline_option_tsf_option(opt,2, range))
                scalar_file_options->set_scaling (range[0], range[1]);
            }
            catch (Exception& E) { E.display(); }
            return true;
          }


          if (opt.opt->is ("tractography.tsf_thresh"))
          {
            try {
              //Set the tsf visualisation threshold
              vector<default_type> range;
              if (process_commandline_option_tsf_option(opt,2, range))
                scalar_file_options->set_threshold (TrackThresholdType::UseColourFile,range[0], range[1]);
            }
            catch(Exception& E) { E.display(); }
            return true;
          }



          if (opt.opt->is ("tractography.thickness")) {
            // Thickness runs from -1000 to 1000,
            float thickness = float(opt[0]) * 1000.0f;
            try {
              thickness_slider->setValue(thickness);
            }
            catch (Exception& E) { E.display(); }
            return true;
          }


          if (opt.opt->is ("tractography.tsf_colourmap")) {
            try {
              int n = opt[0];
              if (n < 0 || !ColourMap::maps[n].name)
                throw Exception ("invalid tsf colourmap index \"" + std::string (opt[0]) + "\" for -tractography.tsf_colourmap option");
              if (process_commandline_option_tsf_check_tracto_loaded()) {
                // get list of selected tractograms:
                QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
                if (indices.size() != 1)
                  throw Exception ("-tractography.tsf_colourmap option requires one tractogram to be selected");

                // get pointer to tractogram:
                Tractogram* tractogram = tractogram_list_model->get_tractogram (indices[0]);

                // check tractogram has a scalar file attached and prepare the scalar_file_options object:
                if (tractogram->get_color_type() == TrackColourType::ScalarFile) {
                  scalar_file_options->set_tractogram (tractogram);
                  scalar_file_options->set_colourmap (opt[0]);
                }
              }
            } catch (Exception& e) { e.display(); }
            return true;
          }


           if (opt.opt->is ("tractography.colour")) {
            try {

              auto values = parse_floats (opt[0]);
              if (values.size() != 3)
                throw Exception ("must provide exactly three comma-separated values to the -tractography.colour option");
              const float max_value = std::max ({ values[0], values[1], values[2] });
              if (std::min ({ values[0], values[1], values[2] }) < 0.0 || max_value > 255)
                throw Exception ("values provided to -tractogram.colour must be either between 0.0 and 1.0, or between 0 and 255");
              const float multiplier = max_value <= 1.0 ? 255.0 : 1.0;

              //input need to be a float *
              QColor colour (multiplier*values[0], multiplier*values[1], multiplier*values[2]);

              QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();

              if (indices.size() != 1)
                  throw Exception ("-tractography.colour option requires one tractogram to be selected");
              // get pointer to tractogram:
              Tractogram* tractogram = tractogram_list_model->get_tractogram (indices[0]);

              // set the color
              tractogram->set_color_type (TrackColourType::Manual);
              tractogram->set_colour (colour);

              // update_color_type_gui
              colour_combobox->setCurrentIndex (3);
              colour_button->setEnabled (true);
              colour_button->setColor (colour);


            }
            catch (Exception& e) { e.display(); }
            return true;
           }


          if (opt.opt-> is ("tractography.geometry")) {
            try {             const TrackGeometryType geom_type = geometry_index2type (geometry_string2index (opt[0]));
              QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
              if (indices.size()) {
                for (int i = 0; i < indices.size(); ++i)
                  tractogram_list_model->get_tractogram (indices[i])->set_geometry_type (geom_type);
              } else {
                Tractogram::default_tract_geom = geom_type;
              }
              update_geometry_type_gui();
            }
            catch (Exception& E) { E.display(); }
            return true;
          }


          if (opt.opt->is ("tractography.opacity")) {
            // Opacity runs from 0 to 1000, so multiply by 1000
            float opacity = float(opt[0]) * 1000.0f;
            try {
              opacity_slider->setValue(opacity);
            }
            catch (Exception& E) { E.display(); }
            return true;
          }

          if (opt.opt->is ("tractography.slab")) {
            float thickness = opt[0];
            try {
              bool crop = thickness > 0;
              slab_group_box->setChecked(crop);
              on_crop_to_slab_slot(crop);//Needs to be manually bumped
              if(crop)
              {
                slab_entry->setValue(thickness);
                on_slab_thickness_slot();//Needs to be manually bumped
              }
            }
            catch (Exception& E) { E.display(); }
            return true;
          }

          if (opt.opt->is ("tractography.lighting")) {
            const bool value = bool(opt[0]);
            lighting_group_box->setChecked (value);
            use_lighting = bool(value);
            return true;
          }

          return false;
        }






      /*Checks whether any tractography has been loaded and warns the user if it has not*/
        bool Tractography::process_commandline_option_tsf_check_tracto_loaded()
        {
          int count = tractogram_list_model->rowCount();
          if (count == 0){
            //Error to std error to save many dialogs appearing for a single missed argument
            std::cerr << "TSF argument specified but no tractography loaded. Ensure TSF arguments follow the tractography.load argument.\n";
          }
          return count != 0;
        }






      /*Checks whether legal to apply tsf options and prepares the scalar_file_options to do so. Returns the vector of floats parsed from the options, or null on fail*/
        bool Tractography::process_commandline_option_tsf_option(const MR::App::ParsedOption& opt, uint reqArgSize, vector<default_type>& range)
        {
          if(process_commandline_option_tsf_check_tracto_loaded()){
            QModelIndexList indices = tractogram_list_view->selectionModel()->selectedIndexes();
            range = opt[0].as_sequence_float();
            if(indices.size() == 1 && range.size() == reqArgSize){
              //values supplied
              Tractogram* tractogram = tractogram_list_model->get_tractogram (indices[0]);
              if(tractogram->get_color_type() == TrackColourType::ScalarFile){
                //prereq options supplied/executed
                scalar_file_options->set_tractogram(tractogram);
                return true;
              }
              else
              {
                std::cerr << "Could not apply TSF argument - tractography.load_tsf not supplied.\n";
              }
            }
            else
            {
              std::cerr << "Could not apply TSF argument - insufficient number of arguments provided.\n";
            }
          }
        return false;
        }
      }
    }
  }
}





