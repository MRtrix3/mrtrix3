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

#include "gui/mrview/tool/vector.h"

#include "mrtrix.h"
#include "gui/mrview/window.h"
#include "gui/mrview/tool/fixel.h"
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
                    fixel_image = new Fixel (filenames[i], fixel_tool);
                  else
                    fixel_image = new PackedFixel (filenames[i], fixel_tool);
                }
                catch(InvalidImageException& e)
                {
                  e.display();
                  continue;
                }

                items.push_back (std::unique_ptr<Displayable> (fixel_image));
              }

              beginInsertRows (QModelIndex(), old_size, items.size());
              endInsertRows();
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

            //colour_combobox = new QComboBox;
            colour_combobox = new ComboBoxWithErrorMsg (0, "  (variable)  ");
            hlayout->addWidget (new QLabel ("colour by "));
            main_box->addLayout (hlayout);
            colour_combobox->addItem ("Value");
            colour_combobox->addItem ("Direction");
            hlayout->addWidget (colour_combobox, 0);
            connect (colour_combobox, SIGNAL (activated(int)), this, SLOT (colour_changed_slot(int)));

            colourmap_option_group = new QGroupBox ("Colour map and scaling");
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
            hlayout->addWidget (new QLabel ("scale by "));
            //length_combobox = new QComboBox;
            length_combobox = new ComboBoxWithErrorMsg (0, "  (variable)  ");
            length_combobox->addItem ("Unity");
            length_combobox->addItem ("Fixel size");
            length_combobox->addItem ("Associated value");
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
            update_selection();
        }


        Vector::~Vector () {}


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
            update_selection();
          }
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
          length_multiplier->setEnabled (indices.size());
          length_combobox->setEnabled (indices.size());

          if (!indices.size()) {
            max_value->setValue (NAN);
            min_value->setValue (NAN);
            threshold_lower->setValue (NAN);
            threshold_upper->setValue (NAN);
            length_multiplier->setValue (NAN);
            return;
          }

          float rate = 0.0f, min_val = 0.0f, max_val = 0.0f;
          float lower_threshold_val = 0.0f, upper_threshold_val = 0.0f;
          float line_length_multiplier = 0.0f;
          float line_thickness(0.f);
          int num_lower_threshold = 0, num_upper_threshold = 0;
          int colourmap_index = -2;
          for (int i = 0; i < indices.size(); ++i) {
            AbstractFixel* fixel = dynamic_cast<AbstractFixel*> (fixel_list_model->get_fixel_image (indices[i]));
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
            line_thickness = fixel->get_line_thickenss();
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
              colourmap_button->colourmap_actions[i]->setChecked (false);
          } else {
              colourmap_button->colourmap_actions[colourmap_index]->setChecked (true);
          }

          // FIXME Intensity windowing display values are not correctly updated
          min_value->setRate (rate);
          max_value->setRate (rate);
          min_value->setValue (min_val);
          max_value->setValue (max_val);
          length_multiplier->setValue (line_length_multiplier);

          // Do a better job of setting colour / length with multiple inputs
          AbstractFixel* first_fixel = dynamic_cast<AbstractFixel*> (fixel_list_model->get_fixel_image (indices[0]));
          const FixelLengthType length_type = first_fixel->get_length_type();
          const FixelColourType colour_type = first_fixel->get_colour_type();
          bool consistent_length = true, consistent_colour = true;
          size_t colour_by_value_count = (first_fixel->get_colour_type() == CValue);
          for (int i = 1; i < indices.size(); ++i) {
            AbstractFixel* fixel = dynamic_cast<AbstractFixel*> (fixel_list_model->get_fixel_image (indices[i]));
            if (fixel->get_length_type() != length_type)
              consistent_length = false;
            if (fixel->get_colour_type() != colour_type)
              consistent_colour = false;
            if (fixel->get_colour_type() == CValue)
              ++colour_by_value_count;
          }

          if (consistent_length) {
            length_combobox->setCurrentIndex (length_type);
          } else {
            length_combobox->setError();
          }

          if (consistent_colour) {
            colour_combobox->setCurrentIndex (colour_type);
            colourmap_option_group->setEnabled (colour_type == CValue);
          } else {
            colour_combobox->setError();
            // Enable as long as there is at least one colour-by-value
            colourmap_option_group->setEnabled (colour_by_value_count);
          }

          threshold_lower->setValue (lower_threshold_val);
          if (num_lower_threshold) {
            if (num_lower_threshold == indices.size()) {
              threshold_lower_box->setTristate (false);
              threshold_lower_box->setCheckState (Qt::Checked);
              threshold_lower->setEnabled (true);
            } else {
              threshold_lower_box->setTristate (true);
              threshold_lower_box->setCheckState (Qt::PartiallyChecked);
              threshold_lower->setEnabled (true);
            }
          } else {
            threshold_lower_box->setTristate (false);
            threshold_lower_box->setCheckState (Qt::Unchecked);
            threshold_lower->setEnabled (false);
          }
          threshold_lower->setRate (rate);

          threshold_upper->setValue (upper_threshold_val);
          if (num_upper_threshold) {
            if (num_upper_threshold == indices.size()) {
              threshold_upper_box->setTristate (false);
              threshold_upper_box->setCheckState (Qt::Checked);
              threshold_upper->setEnabled (true);
            } else {
              threshold_upper_box->setTristate (true);
              threshold_upper_box->setCheckState (Qt::PartiallyChecked);
              threshold_upper->setEnabled (true);
            }
          } else {
            threshold_upper_box->setTristate (false);
            threshold_upper_box->setCheckState (Qt::Unchecked);
            threshold_upper->setEnabled (false);
          }
          threshold_upper->setRate (rate);

          line_thickness_slider->setValue(static_cast<int>(line_thickness * 1.0e5f));
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
          window().updateGL();
        }


        void Vector::selection_changed_slot (const QItemSelection &, const QItemSelection &)
        {
          update_selection ();
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
            fixel_list_model->get_fixel_image (indices[i])->set_colour_type (CValue);
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
          update_selection ();
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

          switch (selection) {
            case 0: {
              colourmap_option_group->setEnabled (true);
              for (int i = 0; i < indices.size(); ++i)
                fixel_list_model->get_fixel_image (indices[i])->set_colour_type (CValue);
              break;
            }
            case 1: {
              colourmap_option_group->setEnabled (false);
              for (int i = 0; i < indices.size(); ++i)
                fixel_list_model->get_fixel_image (indices[i])->set_colour_type (Direction);
              break;
            }
            default:
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
          for (int i = 0; i < indices.size(); ++i)
            fixel_list_model->get_fixel_image (indices[i])->set_use_discard_lower (threshold_lower_box->isChecked());
          window().updateGL();
        }


        void Vector::threshold_upper_changed (int)
        {
          if (threshold_upper_box->checkState() == Qt::PartiallyChecked) return;
          threshold_upper->setEnabled (threshold_upper_box->isChecked());
          QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i)
            fixel_list_model->get_fixel_image (indices[i])->set_use_discard_upper (threshold_upper_box->isChecked());
          window().updateGL();
        }


        void Vector::threshold_lower_value_changed ()
        {
          if (threshold_lower_box->checkState() == Qt::PartiallyChecked) return;
          if (threshold_lower_box->isChecked()) {
            QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
            for (int i = 0; i < indices.size(); ++i)
              fixel_list_model->get_fixel_image (indices[i])->lessthan = threshold_lower->value();
            window().updateGL();
          }
        }


        void Vector::threshold_upper_value_changed ()
        {
          if (threshold_upper_box->checkState() == Qt::PartiallyChecked) return;
          if (threshold_upper_box->isChecked()) {
            QModelIndexList indices = fixel_list_view->selectionModel()->selectedIndexes();
            for (int i = 0; i < indices.size(); ++i)
              fixel_list_model->get_fixel_image (indices[i])->greaterthan = threshold_upper->value();
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





