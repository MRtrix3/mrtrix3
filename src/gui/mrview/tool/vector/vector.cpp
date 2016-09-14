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

#include "gui/mrview/tool/vector/vector.h"

#include "mrtrix.h"
#include "gui/mrview/window.h"
#include "gui/mrview/tool/vector/fixel.h"
#include "gui/mrview/tool/vector/sparsefixel.h"
#include "gui/mrview/tool/vector/packedfixel.h"
#include "gui/mrview/tool/vector/fixelfolder.h"
#include "gui/dialog/file.h"
#include "gui/mrview/tool/list_model_base.h"
#include "math/rng.h"

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

              size_t old_size = items.size();
              for (size_t i = 0, N = filenames.size(); i < N; ++i) {
                AbstractFixel* fixel_image(nullptr);

                try
                {
                  if(Path::has_suffix (filenames[i], {".msf", ".msh"}))
                    fixel_image = new SparseFixel (filenames[i], fixel_tool);
                  else
                    fixel_image = new FixelFolder (filenames[i], fixel_tool);
                }
                catch (InvalidFixelDirectoryException &)
                {
                  try
                  {
                    fixel_image = new PackedFixel (filenames[i], fixel_tool);
                  }
                  catch (InvalidImageException& e)
                  {
                    e.display();
                    continue;
                  }
                }
                catch(InvalidImageException& e)
                {
                  e.display();
                  continue;
                }

                items.push_back (std::unique_ptr<Displayable> (fixel_image));
              }

              beginInsertRows (QModelIndex(), old_size, items.size());
              endInsertRows ();
            }

            AbstractFixel* get_fixel_image (QModelIndex& index) {
              return dynamic_cast<AbstractFixel*>(items[index.row()].get());
            }
        };



        Vector::Vector (Dock* parent) :
          Base (parent),
          do_lock_to_grid (true),
          do_crop_to_slice (true),
          not_3D (true),
          line_opacity (1.0) {

            VBoxLayout* main_box = new VBoxLayout (this);
            HBoxLayout* layout = new HBoxLayout;
            layout->setContentsMargins (0, 0, 0, 0);
            layout->setSpacing (0);

            QPushButton* button = new QPushButton (this);
            button->setToolTip (tr ("Open fixel image"));
            button->setIcon (QIcon (":/open.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (fixel_open_slot ()));
            layout->addWidget (button, 1);

            button = new QPushButton (this);
            button->setToolTip (tr ("Close fixel image"));
            button->setIcon (QIcon (":/close.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (fixel_close_slot ()));
            layout->addWidget (button, 1);

            hide_all_button = new QPushButton (this);
            hide_all_button->setToolTip (tr ("Hide all fixel images"));
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
            main_box->addLayout (hlayout);

            // Colouring
            colour_combobox = new ComboBoxWithErrorMsg (0, "  (variable)  ");
            hlayout->addWidget (new QLabel ("colour by "));
            colour_combobox->addItem ("direction");
            colour_combobox->addItem ("value");
            hlayout->addWidget (colour_combobox, 0);
            connect (colour_combobox, SIGNAL (activated(int)), this, SLOT (colour_changed_slot(int)));

            colourmap_option_group = new QGroupBox ("Colour map and intensity windowing");
            main_box->addWidget (colourmap_option_group);
            hlayout = new HBoxLayout;
            colourmap_option_group->setLayout (hlayout);

            colourmap_button = new ColourMapButton (this, *this, false);

            hlayout->addWidget (colourmap_button);

            min_value = new AdjustButton (this);
            connect (min_value, SIGNAL (valueChanged()), this, SLOT (on_set_scaling_slot()));
            hlayout->addWidget (min_value);

            max_value = new AdjustButton (this);
            connect (max_value, SIGNAL (valueChanged()), this, SLOT (on_set_scaling_slot()));
            hlayout->addWidget (max_value);

            // Thresholding
            hlayout = new HBoxLayout;
            main_box->addLayout (hlayout);
            hlayout->addWidget (new QLabel ("threshold by "));

            threshold_combobox = new ComboBoxWithErrorMsg (0, "  (variable)  ");
            threshold_combobox->addItem ("Fixel size");
            threshold_combobox->addItem ("Associated value");
            hlayout->addWidget (threshold_combobox, 0);
            connect (threshold_combobox, SIGNAL (activated(int)), this, SLOT (threshold_type_slot(int)));

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

            // Scaling
            hlayout = new HBoxLayout;
            main_box->addLayout (hlayout);

            hlayout->addWidget (new QLabel ("scale by "));
            length_combobox = new ComboBoxWithErrorMsg (0, "  (variable)  ");
            length_combobox->addItem ("unity");
            length_combobox->addItem ("fixel size");
            length_combobox->addItem ("associated value");
            hlayout->addWidget (length_combobox, 0);
            connect (length_combobox, SIGNAL (activated(int)), this, SLOT (length_type_slot(int)));

            hlayout = new HBoxLayout;
            main_box->addLayout (hlayout);
            hlayout->addWidget (new QLabel ("length multiplier"));
            length_multiplier = new AdjustButton (this, 0.01);
            length_multiplier->setMin (0.1);
            length_multiplier->setValue (1.0);
            connect (length_multiplier, SIGNAL (valueChanged()), this, SLOT (length_multiplier_slot()));
            hlayout->addWidget (length_multiplier);

            GridLayout* default_opt_grid = new GridLayout;
            line_thickness_slider = new QSlider (Qt::Horizontal);
            line_thickness_slider->setRange (10,1000);
            line_thickness_slider->setSliderPosition (200);
            connect (line_thickness_slider, SIGNAL (valueChanged (int)), this, SLOT (line_thickness_slot (int)));
            default_opt_grid->addWidget (new QLabel ("line thickness"), 0, 0);
            default_opt_grid->addWidget (line_thickness_slider, 0, 1);

            opacity_slider = new QSlider (Qt::Horizontal);
            opacity_slider->setRange (1, 1000);
            opacity_slider->setSliderPosition (int (1000));
            connect (opacity_slider, SIGNAL (valueChanged (int)), this, SLOT (opacity_slot (int)));
            default_opt_grid->addWidget (new QLabel ("opacity"), 1, 0);
            default_opt_grid->addWidget (opacity_slider, 1, 1);

            lock_to_grid = new QGroupBox (tr("lock to grid"));
            lock_to_grid->setCheckable (true);
            lock_to_grid->setChecked (true);
            connect (lock_to_grid, SIGNAL (clicked (bool)), this, SLOT (on_lock_to_grid_slot (bool)));
            default_opt_grid->addWidget (lock_to_grid, 2, 0, 1, 2);

            crop_to_slice = new QGroupBox (tr("crop to slice"));
            crop_to_slice->setCheckable (true);
            crop_to_slice->setChecked (true);
            connect (crop_to_slice, SIGNAL (clicked (bool)), this, SLOT (on_crop_to_slice_slot (bool)));
            default_opt_grid->addWidget (crop_to_slice, 3, 0, 1, 2);

            main_box->addLayout (default_opt_grid, 0);

            main_box->addStretch ();
            setMinimumSize (main_box->minimumSize());
            update_gui_controls ();
        }



        void Vector::draw (const Projection& transform, bool is_3D, int, int)
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          not_3D = !is_3D;
          for (int i = 0; i < fixel_list_model->rowCount(); ++i) {
            if (fixel_list_model->items[i]->show && !hide_all_button->isChecked())
              dynamic_cast<AbstractFixel*>(fixel_list_model->items[i].get())->render (transform);
          }
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }


        void Vector::draw_colourbars ()
        {
          if(hide_all_button->isChecked()) return;

          for (size_t i = 0, N = fixel_list_model->rowCount(); i < N; ++i) {
            if (fixel_list_model->items[i]->show)
              dynamic_cast<AbstractFixel*>(fixel_list_model->items[i].get())->request_render_colourbar(*this);
          }
        }


        size_t Vector::visible_number_colourbars () {
           size_t total_visible(0);

           if(!hide_all_button->isChecked()) {
             for (size_t i = 0, N = fixel_list_model->rowCount(); i < N; ++i) {
               AbstractFixel* fixel = dynamic_cast<AbstractFixel*>(fixel_list_model->items[i].get());
               if (fixel && fixel->show && !ColourMap::maps[fixel->colourmap].special)
                 total_visible += 1;
             }
           }

           return total_visible;
        }



        void Vector::render_fixel_colourbar(const Tool::AbstractFixel& fixel)
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          float min_value = fixel.use_discard_lower() ?
                      fixel.scaling_min_thresholded() :
                      fixel.scaling_min();

          float max_value = fixel.use_discard_upper() ?
                      fixel.scaling_max_thresholded() :
                      fixel.scaling_max();

          window().colourbar_renderer.render (fixel.colourmap, fixel.scale_inverted(),
                                              min_value, max_value,
                                              fixel.scaling_min(), fixel.display_range,
                                              Eigen::Array3f { fixel.colour[0] / 255.0f, fixel.colour[1] / 255.0f, fixel.colour[2] / 255.0f });
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }


        void Vector::fixel_open_slot ()
        {
          std::vector<std::string> list = Dialog::File::get_files (this,
                                                                   "Select fixel images to open",
                                                                   GUI::Dialog::File::image_filter_string);
          add_images (list);
        }


        void Vector::add_images (std::vector<std::string> &list)
        {
          if (list.empty())
            return;
          size_t previous_size = fixel_list_model->rowCount();
          fixel_list_model->add_items (list, *this);

          // Some of the images may be invalid, so it could be the case that no images were added
          size_t new_size = fixel_list_model->rowCount();
          if(previous_size < new_size) {
            QModelIndex first = fixel_list_model->index (previous_size, 0, QModelIndex());
            QModelIndex last = fixel_list_model->index (new_size -1, 0, QModelIndex());
            fixel_list_view->selectionModel()->select (QItemSelection (first, last), QItemSelectionModel::Select);
            update_gui_controls ();
          }

          window().updateGL ();
        }


        void Vector::dropEvent (QDropEvent* event)
        {
          static constexpr int max_files = 32;

          const QMimeData* mimeData = event->mimeData();
          if (mimeData->hasUrls()) {
            std::vector<std::string> list;
            QList<QUrl> urlList = mimeData->urls();
            for (int i = 0; i < urlList.size() && i < max_files; ++i) {
                list.push_back (urlList.at (i).path().toUtf8().constData());
            }
            try {
              add_images (list);
            }
            catch (Exception& e) {
              e.display();
            }
          }
        }


        void Vector::fixel_close_slot ()
        {
          QModelIndexList indexes = fixel_list_view->selectionModel()->selectedIndexes();
          while (indexes.size()) {
            fixel_list_model->remove_item (indexes.first());
            indexes = fixel_list_view->selectionModel()->selectedIndexes();
          }
          window().updateGL();
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
          window().updateGL();
        }


        void Vector::hide_all_slot ()
        {
          window().updateGL();
        }


        void Vector::update_gui_controls ()
        {
          update_gui_scaling_controls ();
          update_gui_threshold_controls ();
          update_gui_colour_controls ();
        }


        void Vector::update_gui_colour_controls (bool reload_colour_types)
        {
          QModelIndexList indices = fixel_list_view->selectionModel ()->selectedIndexes ();
          size_t n_images (indices.size ());

          colour_combobox->setEnabled (n_images == 1);
          colourmap_button->setEnabled (n_images);

          max_value->setEnabled (n_images);
          min_value->setEnabled (n_images);

          if (!n_images) {
            max_value->setValue (NAN);
            min_value->setValue (NAN);
            length_multiplier->setValue (NAN);
            return;
          }


          if (!n_images)
            return;

          int colourmap_index = -2;
          for (size_t i = 0; i < n_images; ++i) {
            AbstractFixel* fixel = dynamic_cast<AbstractFixel*> (fixel_list_model->get_fixel_image (indices[i]));
            if (colourmap_index != int (fixel->colourmap)) {
              if (colourmap_index == -2)
                colourmap_index = fixel->colourmap;
              else
                colourmap_index = -1;
            }
          }

          // Not all colourmaps are added to this list; therefore need to find out
          // how many menu elements were actually created by ColourMap::create_menu()
          static size_t colourmap_count = 0;
          if (!colourmap_count) {
            for (size_t i = 0; MR::GUI::MRView::ColourMap::maps[i].name; ++i) {
              if (!MR::GUI::MRView::ColourMap::maps[i].special)
                ++colourmap_count;
            }
          }

          if (colourmap_index < 0) {
            for (size_t i = 0; i != colourmap_count; ++i )
              colourmap_button->colourmap_actions[i]->setChecked (false);
          } else {
            colourmap_button->colourmap_actions[colourmap_index]->setChecked (true);
          }

          AbstractFixel* first_fixel = dynamic_cast<AbstractFixel*> (fixel_list_model->get_fixel_image (indices[0]));

          if (n_images == 1 && reload_colour_types)
            first_fixel->load_colourby_combobox_options (*colour_combobox);

          const FixelColourType colour_type = first_fixel->get_colour_type ();

          colour_combobox->setCurrentIndex (first_fixel->get_colour_type_index ());
          colourmap_option_group->setEnabled (colour_type == CValue);

          max_value->setEnabled (colour_type == CValue);
          min_value->setEnabled (colour_type == CValue);

          if (colour_type == CValue) {
            min_value->setRate (first_fixel->scaling_rate ());
            max_value->setRate (first_fixel->scaling_rate ());
            min_value->setValue (first_fixel->scaling_min ());
            max_value->setValue (first_fixel->scaling_max ());
          }
        }


        void Vector::update_gui_scaling_controls (bool reload_scaling_types)
        {
          QModelIndexList indices = fixel_list_view->selectionModel ()->selectedIndexes ();
          size_t n_images (indices.size ());

          length_multiplier->setEnabled (n_images);
          length_combobox->setEnabled (n_images == 1);

          if (!n_images) {
            length_multiplier->setValue (NAN);
            return;
          }

          AbstractFixel* first_fixel = dynamic_cast<AbstractFixel*> (fixel_list_model->get_fixel_image (indices[0]));

          if (n_images == 1 && reload_scaling_types)
            first_fixel->load_scaleby_combobox_options (*length_combobox);

          length_multiplier->setValue (first_fixel->get_line_length_multiplier ());
          line_thickness_slider->setValue (static_cast<int>(first_fixel->get_line_thickenss () * 1.0e5f));

          length_combobox->setCurrentIndex (first_fixel->get_scale_type_index ());
        }


        void Vector::update_gui_threshold_controls (bool reload_threshold_types)
        {
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          size_t n_images (indices.size ());

          threshold_lower->setEnabled (n_images);
          threshold_upper->setEnabled (n_images);
          threshold_upper_box->setEnabled (n_images);
          threshold_lower_box->setEnabled (n_images);

          threshold_combobox->setEnabled (n_images == 1);

          if (!n_images) {
            threshold_lower->setValue (NAN);
            threshold_upper->setValue (NAN);
            return;
          }

          AbstractFixel* first_fixel = dynamic_cast<AbstractFixel*> (fixel_list_model->get_fixel_image (indices[0]));

          bool has_val = first_fixel->has_values ();

          if (n_images == 1 && reload_threshold_types && has_val)
            first_fixel->load_threshold_combobox_options (*threshold_combobox);

          threshold_lower->setEnabled (has_val);
          threshold_upper->setEnabled (has_val);
          threshold_lower_box->setEnabled (has_val);
          threshold_upper_box->setEnabled (has_val);
          threshold_combobox->setEnabled (has_val);

          if (!has_val) {
            threshold_lower_box->setChecked (false);
            threshold_upper_box->setChecked (false);
            return;
          }

          if (!std::isfinite (first_fixel->get_unscaled_threshold_lower ()))
            first_fixel->lessthan = first_fixel->intensity_min ();
          if (!std::isfinite (first_fixel->get_unscaled_threshold_upper ()))
            first_fixel->greaterthan = first_fixel->intensity_max ();

          threshold_lower->setValue (first_fixel->get_unscaled_threshold_lower ());
          threshold_lower->setRate (first_fixel->get_unscaled_threshold_rate ());
          threshold_lower->setEnabled (first_fixel->use_discard_lower ());
          threshold_lower_box->setChecked (first_fixel->use_discard_lower ());

          threshold_upper->setValue (first_fixel->get_unscaled_threshold_upper ());
          threshold_upper->setRate (first_fixel->get_unscaled_threshold_rate ());
          threshold_upper->setEnabled (first_fixel->use_discard_upper ());
          threshold_upper_box->setChecked (first_fixel->use_discard_upper ());

          threshold_combobox->setCurrentIndex (first_fixel->get_threshold_type_index ());
        }


        void Vector::opacity_slot (int opacity)
        {
          line_opacity = Math::pow2 (static_cast<float>(opacity)) / 1.0e6f;
          window().updateGL();
        }


        void Vector::line_thickness_slot (int thickness)
        {
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i)
            fixel_list_model->get_fixel_image (indices[i])->set_line_thickness (static_cast<float>(thickness) / 1.0e5f);
          window().updateGL();
        }


        void Vector::length_multiplier_slot ()
        {
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i)
            fixel_list_model->get_fixel_image (indices[i])->set_line_length_multiplier (length_multiplier->value());
          window().updateGL();
        }


        void Vector::length_type_slot (int selection)
        {
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();

          for (int i = 0; i < indices.size(); ++i) {
            const auto fixel = fixel_list_model->get_fixel_image (indices[i]);
            fixel->set_scale_type_index (selection);
            update_gui_scaling_controls (false);
            break;
          }

          window ().updateGL ();
        }


        void Vector::threshold_type_slot (int selection)
        {
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();

          for (int i = 0; i < indices.size(); ++i) {
            const auto fixel = fixel_list_model->get_fixel_image (indices[i]);
            fixel->set_threshold_type_index (selection);
            update_gui_threshold_controls (false);
            break;
          }

          window ().updateGL ();
        }


        void Vector::selection_changed_slot (const QItemSelection &, const QItemSelection &)
        {
          update_gui_controls ();
        }


        void Vector::on_lock_to_grid_slot(bool is_checked)
        {
          do_lock_to_grid = is_checked;
          window().updateGL();
        }


        void Vector::on_crop_to_slice_slot (bool is_checked)
        {
          do_crop_to_slice = is_checked;         
          lock_to_grid->setEnabled(do_crop_to_slice);

          window().updateGL();
        }


        void Vector::toggle_show_colour_bar (bool visible, const ColourMapButton&)
        {
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i)
            fixel_list_model->get_fixel_image (indices[i])->show_colour_bar = visible;
          window().updateGL();
        }


        void Vector::selected_colourmap (size_t index, const ColourMapButton&)
        {
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            fixel_list_model->get_fixel_image (indices[i])->colourmap = index;
          }
          window().updateGL();
        }

        void Vector::selected_custom_colour(const QColor& colour, const ColourMapButton&)
        {
          if (colour.isValid()) {
            QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
            std::array<GLubyte, 3> c_colour{{GLubyte(colour.red()), GLubyte(colour.green()), GLubyte(colour.blue())}};
            for (int i = 0; i < indices.size(); ++i) {
              fixel_list_model->get_fixel_image (indices[i])->set_colour (c_colour);
            }
            window().updateGL();
          }
        }

        void Vector::reset_colourmap (const ColourMapButton&)
        {
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i)
            fixel_list_model->get_fixel_image (indices[i])->reset_windowing ();
          update_gui_controls ();
          window().updateGL();
        }


        void Vector::toggle_invert_colourmap (bool inverted, const ColourMapButton&)
        {
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i)
            fixel_list_model->get_fixel_image (indices[i])->set_invert_scale (inverted);
          window().updateGL();
        }


        void Vector::colour_changed_slot (int selection)
        {
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();

          colourmap_option_group->setEnabled (selection == 0);
          for (int i = 0; i < indices.size(); ++i) {
            fixel_list_model->get_fixel_image (indices[i])->set_colour_type_index (selection);
            update_gui_colour_controls (false);
            break;
          }

          window().updateGL();

        }


        void Vector::on_set_scaling_slot ()
        {
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i)
            fixel_list_model->get_fixel_image (indices[i])->set_windowing (min_value->value(), max_value->value());
          window().updateGL();
        }


        void Vector::threshold_lower_changed (int)
        {
          if (threshold_lower_box->checkState() == Qt::PartiallyChecked) return;
          threshold_lower->setEnabled (threshold_lower_box->isChecked());
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            auto& fixel_image = *fixel_list_model->get_fixel_image (indices[i]);
            fixel_image.set_use_discard_lower (threshold_lower_box->isChecked() && fixel_image.has_values ());
          }
          window().updateGL();
        }


        void Vector::threshold_upper_changed (int)
        {
          if (threshold_upper_box->checkState() == Qt::PartiallyChecked) return;
          threshold_upper->setEnabled (threshold_upper_box->isChecked());
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            auto& fixel_image = *fixel_list_model->get_fixel_image (indices[i]);
            fixel_image.set_use_discard_upper (threshold_upper_box->isChecked() && fixel_image.has_values ());
          }
          window().updateGL();
        }


        void Vector::threshold_lower_value_changed ()
        {
          if (threshold_lower_box->checkState() == Qt::PartiallyChecked) return;
          if (threshold_lower_box->isChecked()) {
            QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
            for (int i = 0; i < indices.size(); ++i) {
              auto& fixel_image = *fixel_list_model->get_fixel_image (indices[i]);
              if (fixel_image.has_values ()) {
                fixel_image.set_threshold_lower (threshold_lower->value());
                fixel_image.set_use_discard_lower (threshold_lower_box->isChecked());
              }
            }
            window().updateGL();
          }
        }


        void Vector::threshold_upper_value_changed ()
        {
          if (threshold_upper_box->checkState() == Qt::PartiallyChecked) return;
          if (threshold_upper_box->isChecked()) {
            QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
            for (int i = 0; i < indices.size(); ++i) {
              auto& fixel_image = *fixel_list_model->get_fixel_image (indices[i]);
              if (fixel_image.has_values ()) {
                fixel_image.set_threshold_upper (threshold_upper->value());
                fixel_image.set_use_discard_upper (threshold_upper_box->isChecked());
              }
            }
            window().updateGL();
          }
        }


        void Vector::add_commandline_options (MR::App::OptionList& options) 
        { 
          using namespace MR::App;
          options
            + OptionGroup ("Vector plot tool options")

            + Option ("vector.load", "Load the specified MRtrix sparse image file (.msf) into the fixel tool.")
            +   Argument ("image").type_image_in();
        }

        bool Vector::process_commandline_option (const MR::App::ParsedOption& opt) 
        {
          if (opt.opt->is ("vector.load")) {
            std::vector<std::string> list (1, std::string(opt[0]));
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





