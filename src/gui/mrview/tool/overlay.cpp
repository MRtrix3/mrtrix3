/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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

#include "gui/mrview/tool/overlay.h"

#include "mrtrix.h"
#include "gui/mrview/colourmap.h"
#include "gui/mrview/gui_image.h"
#include "gui/mrview/window.h"
#include "gui/mrview/mode/slice.h"
#include "gui/dialog/file.h"
#include "gui/mrview/tool/list_model_base.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {



        class Overlay::Item : public Image { MEMALIGN(Overlay::Item)
          public:
            Item (MR::Header&& H) : Image (std::move (H)) { }
            Mode::Slice::Shader slice_shader;
        };


        class Overlay::Model : public ListModelBase
        { MEMALIGN(Overlay::Model)
          public:
            Model (QObject* parent) :
              ListModelBase (parent) { }

            void add_items (vector<std::unique_ptr<MR::Header>>& list);

            Item* get_image (QModelIndex& index) {
              return dynamic_cast<Item*>(items[index.row()].get());
            }
        };


        void Overlay::Model::add_items (vector<std::unique_ptr<MR::Header>>& list)
        {
          beginInsertRows (QModelIndex(), items.size(), items.size()+list.size());
          for (size_t i = 0; i < list.size(); ++i) {
            Item* overlay = new Item (std::move (*list[i]));
            overlay->set_allowed_features (true, true, false);
            if (!overlay->colourmap)
              overlay->colourmap = 1;
            overlay->alpha = 1.0f;
            overlay->set_use_transparency (true);
            items.push_back (std::unique_ptr<Displayable> (overlay));
          }
          endInsertRows();
        }




        Overlay::Overlay (Dock* parent) :
          Base (parent) {
            VBoxLayout* main_box = new VBoxLayout (this);
            HBoxLayout* layout = new HBoxLayout;
            layout->setContentsMargins (0, 0, 0, 0);
            layout->setSpacing (0);

            QPushButton* button = new QPushButton (this);
            button->setToolTip (tr ("Open overlay image"));
            button->setIcon (QIcon (":/open.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (image_open_slot ()));
            layout->addWidget (button, 1);

            button = new QPushButton (this);
            button->setToolTip (tr ("Close overlay image"));
            button->setIcon (QIcon (":/close.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (image_close_slot ()));
            layout->addWidget (button, 1);

            hide_all_button = new QPushButton (this);
            hide_all_button->setToolTip (tr ("Hide all overlays"));
            hide_all_button->setIcon (QIcon (":/hide.svg"));
            hide_all_button->setCheckable (true);
            connect (hide_all_button, SIGNAL (clicked()), this, SLOT (hide_all_slot ()));
            layout->addWidget (hide_all_button, 1);

            main_box->addLayout (layout, 0);

            image_list_view = new QListView (this);
            image_list_view->setSelectionMode (QAbstractItemView::ExtendedSelection);
            image_list_view->setDragEnabled (true);
            image_list_view->setDragDropMode (QAbstractItemView::InternalMove);
            image_list_view->setAcceptDrops (true);
            image_list_view->viewport()->setAcceptDrops (true);
            image_list_view->setDropIndicatorShown (true);

            image_list_model = new Model (this);
            image_list_view->setModel (image_list_model);

            image_list_view->setContextMenuPolicy (Qt::CustomContextMenu);
            connect (image_list_view, SIGNAL (customContextMenuRequested (const QPoint&)),
                     this, SLOT (right_click_menu_slot (const QPoint&)));

            main_box->addWidget (image_list_view, 1);

            // Volume selecter
            volume_box = new QGroupBox ("Volume indices (dimension: index)");
            main_box->addWidget (volume_box);
            volume_index_layout = new GridLayout;
            volume_box->setLayout (volume_index_layout);


            QGroupBox* group_box = new QGroupBox (tr("Colour map and scaling"));
            main_box->addWidget (group_box);
            HBoxLayout* hlayout = new HBoxLayout;
            group_box->setLayout (hlayout);

            colourmap_button = new ColourMapButton(this, *this);
            hlayout->addWidget (colourmap_button);

            min_value = new AdjustButton (this);
            connect (min_value, SIGNAL (valueChanged()), this, SLOT (values_changed()));
            hlayout->addWidget (min_value);

            max_value = new AdjustButton (this);
            connect (max_value, SIGNAL (valueChanged()), this, SLOT (values_changed()));
            hlayout->addWidget (max_value);


            QGroupBox* threshold_box = new QGroupBox (tr("Thresholds"));
            main_box->addWidget (threshold_box);
            hlayout = new HBoxLayout;
            threshold_box->setLayout (hlayout);

            lower_threshold_check_box = new QCheckBox (this);
            connect (lower_threshold_check_box, SIGNAL (stateChanged(int)), this, SLOT (lower_threshold_changed(int)));
            hlayout->addWidget (lower_threshold_check_box);
            lower_threshold = new AdjustButton (this, 0.1);
            lower_threshold->setEnabled (false);
            connect (lower_threshold, SIGNAL (valueChanged()), this, SLOT (lower_threshold_value_changed()));
            hlayout->addWidget (lower_threshold);

            upper_threshold_check_box = new QCheckBox (this);
            hlayout->addWidget (upper_threshold_check_box);
            upper_threshold = new AdjustButton (this, 0.1);
            upper_threshold->setEnabled (false);
            connect (upper_threshold_check_box, SIGNAL (stateChanged(int)), this, SLOT (upper_threshold_changed(int)));
            connect (upper_threshold, SIGNAL (valueChanged()), this, SLOT (upper_threshold_value_changed()));
            hlayout->addWidget (upper_threshold);


            opacity_slider = new QSlider (Qt::Horizontal);
            opacity_slider->setRange (1,1000);
            opacity_slider->setSliderPosition (int (1000));
            connect (opacity_slider, SIGNAL (valueChanged (int)), this, SLOT (opacity_changed(int)));
            main_box->addWidget (new QLabel ("opacity"), 0);
            main_box->addWidget (opacity_slider, 0);

            interpolate_check_box = new InterpolateCheckBox (tr ("interpolate"));
            interpolate_check_box->setTristate (true);
            interpolate_check_box->setCheckState (Qt::Checked);
            connect (interpolate_check_box, SIGNAL (clicked ()), this, SLOT (interpolate_changed ()));
            main_box->addWidget (interpolate_check_box, 0);

            connect (image_list_view->selectionModel(),
                SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
                SLOT (selection_changed_slot(const QItemSelection &, const QItemSelection &)) );

            connect (image_list_model, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
                     this, SLOT (toggle_shown_slot (const QModelIndex&, const QModelIndex&)));

            update_selection();
          }


        void Overlay::image_open_slot ()
        {
          vector<std::string> overlay_names = Dialog::File::get_images (this, "Select overlay images to open");
          if (overlay_names.empty())
            return;
          vector<std::unique_ptr<MR::Header>> list;
          for (size_t n = 0; n < overlay_names.size(); ++n)
            list.push_back (make_unique<MR::Header> (MR::Header::open (overlay_names[n])));

          add_images (list);
        }





        void Overlay::add_images (vector<std::unique_ptr<MR::Header>>& list)
        {
          size_t previous_size = image_list_model->rowCount();
          image_list_model->add_items (list);

          QModelIndex first = image_list_model->index (previous_size, 0, QModelIndex());
          QModelIndex last = image_list_model->index (image_list_model->rowCount()-1, 0, QModelIndex());
          image_list_view->selectionModel()->select (QItemSelection (first, last), QItemSelectionModel::ClearAndSelect);
        }




        void Overlay::dropEvent (QDropEvent* event)
        {
          static constexpr int max_files = 32;

          const QMimeData* mimeData = event->mimeData();
          if (mimeData->hasUrls()) {
            vector<std::unique_ptr<MR::Header>> list;
            QList<QUrl> urlList = mimeData->urls();
            for (int i = 0; i < urlList.size() && i < max_files; ++i) {
              try {
                list.push_back (make_unique<MR::Header> (MR::Header::open (urlList.at (i).path().toUtf8().constData())));
              }
              catch (Exception& e) {
                e.display();
              }
            }
            if (list.size())
              add_images (list);
          }
        }




        void Overlay::image_close_slot ()
        {
          QModelIndexList indexes = image_list_view->selectionModel()->selectedIndexes();
          while (indexes.size()) {
            image_list_model->remove_item (indexes.first());
            indexes = image_list_view->selectionModel()->selectedIndexes();
          }
          updateGL();
        }


        void Overlay::hide_all_slot ()
        {
          updateGL();
        }


        void Overlay::draw (const Projection& projection, bool is_3D, int, int)
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          if (!is_3D) {
            // set up OpenGL environment:
            gl::Enable (gl::BLEND);
            gl::Disable (gl::DEPTH_TEST);
            gl::DepthMask (gl::FALSE_);
            gl::ColorMask (gl::TRUE_, gl::TRUE_, gl::TRUE_, gl::TRUE_);
            gl::BlendFunc (gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA);
            gl::BlendEquation (gl::FUNC_ADD);
          }

          bool need_to_update = false;
          for (int i = 0; i < image_list_model->rowCount(); ++i) {
            if (image_list_model->items[i]->show && !hide_all_button->isChecked()) {
              Overlay::Item* image = dynamic_cast<Overlay::Item*>(image_list_model->items[i].get());
              need_to_update |= !std::isfinite (image->intensity_min());
              image->transparent_intensity = image->opaque_intensity = image->intensity_min();
              if (is_3D)
                window().get_current_mode()->overlays_for_3D.push_back (image);
              else
                image->render3D (image->slice_shader, projection, projection.depth_of (window().focus()));
            }
          }

          if (need_to_update)
            update_selection();

          if (!is_3D) {
            // restore OpenGL environment:
            gl::Disable (gl::BLEND);
            gl::Enable (gl::DEPTH_TEST);
            gl::DepthMask (gl::TRUE_);
          }
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }


        size_t Overlay::visible_number_colourbars () {
           size_t total_visible(0);

           if(!hide_all_button->isChecked()) {
             for (size_t i = 0, N = image_list_model->rowCount(); i < N; ++i) {
               Image* image  = dynamic_cast<Image*>(image_list_model->items[i].get());
               if (image && image->show && !ColourMap::maps[image->colourmap].special)
                 total_visible += 1;
             }
           }

           return total_visible;
        }


        void Overlay::draw_colourbars ()
        {
          if(hide_all_button->isChecked())
            return;

          for (size_t i = 0, N = image_list_model->rowCount(); i < N; ++i) {
            if (image_list_model->items[i]->show)
              image_list_model->items[i]->request_render_colourbar(*this);
          }
        }


        int Overlay::draw_tool_labels (int position, int start_line_num, const Projection& transform) const
        {
          if(hide_all_button->isChecked()) return 0;

          int num_of_new_lines = 0;

          for (size_t i = 0, N = image_list_model->rowCount(); i < N; ++i) {

            Image* image = dynamic_cast<Image*>(image_list_model->items[i].get());
            if (image && image->show) {
              std::string value_str = Path::basename(image->get_filename()) + " ";
              cfloat value;
              if (image->interpolate()) {
                value_str += "interp value: ";
                value = image->trilinear_value (window().focus());
              } else {
                value_str += "voxel value: ";
                value = image->nearest_neighbour_value (window().focus());
              }
              if (std::isnan(abs(value)))
                value_str += "?";
              else
                value_str += str(value);
              transform.render_text (value_str, position, start_line_num + num_of_new_lines);
              num_of_new_lines += 1;
            }
          }

          return num_of_new_lines;
        }


        void Overlay::selected_colourmap (size_t index, const ColourMapButton&)
        {
            QModelIndexList indices = image_list_view->selectionModel()->selectedIndexes();
            for (size_t i = 0, N = indices.size(); i < N; ++i) {
              Image* overlay = dynamic_cast<Image*> (image_list_model->get_image (indices[i]));
              overlay->set_colourmap (index);
            }
            updateGL();
        }

        void Overlay::selected_custom_colour(const QColor& colour, const ColourMapButton&)
        {
            QModelIndexList indices = image_list_view->selectionModel()->selectedIndexes();
            for (size_t i = 0, N = indices.size(); i < N; ++i) {
              Image* overlay = dynamic_cast<Image*> (image_list_model->get_image (indices[i]));
              std::array<GLubyte, 3> c_colour{{GLubyte(colour.red()), GLubyte(colour.green()), GLubyte(colour.blue())}};
              overlay->set_colour(c_colour);
            }
            updateGL();
        }

        void Overlay::toggle_show_colour_bar(bool visible, const ColourMapButton&)
        {
            QModelIndexList indices = image_list_view->selectionModel()->selectedIndexes();
            for (size_t i = 0, N = indices.size(); i < N; ++i) {
              Image* overlay = dynamic_cast<Image*> (image_list_model->get_image (indices[i]));
              overlay->show_colour_bar = visible;
            }
            updateGL();
        }


        void Overlay::toggle_invert_colourmap(bool invert, const ColourMapButton&)
        {
            QModelIndexList indices = image_list_view->selectionModel()->selectedIndexes();
            for (size_t i = 0, N = indices.size(); i < N; ++i) {
              Image* overlay = dynamic_cast<Image*> (image_list_model->get_image (indices[i]));
              overlay->set_invert_scale(invert);
            }
            updateGL();
        }


        void Overlay::reset_colourmap(const ColourMapButton&)
        {
            QModelIndexList indices = image_list_view->selectionModel()->selectedIndexes();
            Displayable* overlay = nullptr;
            for (size_t i = 0, N = indices.size(); i < N; ++i) {
              overlay = dynamic_cast<Displayable*> (image_list_model->get_image (indices[i]));
              overlay->reset_windowing();
            }

            // Reset the min/max adjust button fields of last selected overlay
            if(overlay) {
             min_value->setValue(overlay->intensity_min());
             max_value->setValue(overlay->intensity_max());
            }

            updateGL();
        }


        void Overlay::render_image_colourbar (const Image& image)
        {
            float min_value = image.use_discard_lower() ?
                        image.scaling_min_thresholded() :
                        image.scaling_min();

            float max_value = image.use_discard_upper() ?
                        image.scaling_max_thresholded() :
                        image.scaling_max();

            window().colourbar_renderer.render (image.colourmap, image.scale_inverted(),
                                                min_value, max_value,
                                                image.scaling_min(), image.display_range,
                                                Eigen::Vector3f { image.colour[0] / 255.0f, image.colour[1] / 255.0f, image.colour[2] / 255.0f });
        }


        void Overlay::toggle_shown_slot (const QModelIndex& index, const QModelIndex& index2)
        {
          if (index.row() == index2.row()) {
            image_list_view->setCurrentIndex(index);
          } else {
            for (size_t i = 0; i < image_list_model->items.size(); ++i) {
              if (image_list_model->items[i]->show) {
                image_list_view->setCurrentIndex (image_list_model->index (i, 0));
                break;
              }
            }
          }
          updateGL();
        }


        void Overlay::onSetVolumeIndex ()
        {
          QModelIndexList indices = image_list_view->selectionModel()->selectedIndexes();
          if (indices.size() != 1) return;
          Image* overlay = dynamic_cast<Image*> (image_list_model->get_image (indices[0]));
          if (overlay->header().ndim() < 4) return;
          assert (overlay->header().ndim() == size_t(volume_index_layout->count()+3));

          for (int i = 0; i < volume_index_layout->count(); ++i) {
            auto* box = dynamic_cast<SpinBox*> (volume_index_layout->itemAt(i)->widget());
            if (overlay->header().ndim() <= size_t(i+3))
              break;
            overlay->image.index(i+3) = box->value();
          }
          if (overlay->show)
            updateGL();
        }


        void Overlay::update_slot (int)
        {
          updateGL();
        }



        void Overlay::values_changed ()
        {
          QModelIndexList indices = image_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            Image* overlay = dynamic_cast<Image*> (image_list_model->get_image (indices[i]));
            overlay->set_windowing (min_value->value(), max_value->value());
          }
          updateGL();
        }


        void Overlay::lower_threshold_changed (int)
        {
          QModelIndexList indices = image_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            Image* overlay = dynamic_cast<Image*> (image_list_model->get_image (indices[i]));
            overlay->lessthan = lower_threshold->value();
            overlay->set_use_discard_lower (lower_threshold_check_box->isChecked());
          }
          lower_threshold->setEnabled (indices.size() && lower_threshold_check_box->isChecked());
          updateGL();
        }


        void Overlay::upper_threshold_changed (int)
        {
          QModelIndexList indices = image_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            Image* overlay = dynamic_cast<Image*> (image_list_model->get_image (indices[i]));
            overlay->greaterthan = upper_threshold->value();
            overlay->set_use_discard_upper (upper_threshold_check_box->isChecked());
          }
          upper_threshold->setEnabled (indices.size() && upper_threshold_check_box->isChecked());
          updateGL();
        }



        void Overlay::lower_threshold_value_changed ()
        {
          if (lower_threshold_check_box->isChecked()) {
            QModelIndexList indices = image_list_view->selectionModel()->selectedIndexes();
            for (int i = 0; i < indices.size(); ++i) {
              Image* overlay = dynamic_cast<Image*> (image_list_model->get_image (indices[i]));
              overlay->lessthan = lower_threshold->value();
            }
          }
          updateGL();
        }



        void Overlay::upper_threshold_value_changed ()
        {
          if (upper_threshold_check_box->isChecked()) {
            QModelIndexList indices = image_list_view->selectionModel()->selectedIndexes();
            for (int i = 0; i < indices.size(); ++i) {
              Image* overlay = dynamic_cast<Image*> (image_list_model->get_image (indices[i]));
              overlay->greaterthan = upper_threshold->value();
            }
          }
          updateGL();
        }


        void Overlay::opacity_changed (int)
        {
          QModelIndexList indices = image_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            Image* overlay = dynamic_cast<Image*> (image_list_model->get_image (indices[i]));
            overlay->alpha = opacity_slider->value() / 1.0e3f;
          }
          window().updateGL();
        }

        void Overlay::interpolate_changed ()
        {
          QModelIndexList indices = image_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            Image* overlay = dynamic_cast<Image*> (image_list_model->get_image (indices[i]));
            overlay->set_interpolate (interpolate_check_box->isChecked());
          }
          window().updateGL();
        }


        void Overlay::selection_changed_slot (const QItemSelection &, const QItemSelection &)
        {
          update_selection();
        }


        void Overlay::right_click_menu_slot (const QPoint& pos)
        {
          QModelIndex index = image_list_view->indexAt (pos);
          if (index.isValid()) {
            QPoint globalPos = image_list_view->mapToGlobal (pos);
            image_list_view->selectionModel()->select(index, QItemSelectionModel::Select);
            colourmap_button->open_menu (globalPos);
          }
        }



        void Overlay::update_selection ()
        {
          QModelIndexList indices = image_list_view->selectionModel()->selectedIndexes();
          while (volume_index_layout->count())
            delete volume_index_layout->takeAt (volume_index_layout->count()-1)->widget();
          colourmap_button->setEnabled (indices.size());
          max_value->setEnabled (indices.size());
          min_value->setEnabled (indices.size());
          lower_threshold_check_box->setEnabled (indices.size());
          upper_threshold_check_box->setEnabled (indices.size());
          lower_threshold->setEnabled (indices.size());
          upper_threshold->setEnabled (indices.size());
          opacity_slider->setEnabled (indices.size());
          interpolate_check_box->setEnabled (indices.size());

          if (!indices.size()) {
            max_value->setValue (NAN);
            min_value->setValue (NAN);
            lower_threshold->setValue (NAN);
            upper_threshold->setValue (NAN);
            updateGL();
            return;
          }

          float rate = 0.0f, min_val = 0.0f, max_val = 0.0f;
          float lower_threshold_val = 0.0f, upper_threshold_val = 0.0f;
          float opacity = 0.0f;
          int num_lower_threshold = 0, num_upper_threshold = 0;
          int colourmap_index = -2;
          int num_interp = 0;
          for (int i = 0; i < indices.size(); ++i) {
            Image* overlay = dynamic_cast<Image*> (image_list_model->get_image (indices[i]));
            if (colourmap_index != int(overlay->colourmap)) {
              if (colourmap_index == -2)
                colourmap_index = overlay->colourmap;
              else
                colourmap_index = -1;
            }
            rate += overlay->scaling_rate();
            min_val += overlay->scaling_min();
            max_val += overlay->scaling_max();
            num_lower_threshold += overlay->use_discard_lower();
            num_upper_threshold += overlay->use_discard_upper();
            opacity += overlay->alpha;
            if (overlay->interpolate())
              ++num_interp;
            if (!std::isfinite (overlay->lessthan))
              overlay->lessthan = overlay->intensity_min();
            if (!std::isfinite (overlay->greaterthan))
              overlay->greaterthan = overlay->intensity_max();
            lower_threshold_val += overlay->lessthan;
            upper_threshold_val += overlay->greaterthan;
          }

          rate /= indices.size();
          min_val /= indices.size();
          max_val /= indices.size();
          lower_threshold_val /= indices.size();
          upper_threshold_val /= indices.size();
          opacity /= indices.size();

          if (indices.size() == 1) {
            Image* overlay = dynamic_cast<Image*> (image_list_model->get_image (indices[0]));

            // volume_box->setVisible(overlay->header().ndim() > 3); // causes shift in FOV due to resizing of tool pane
            for (size_t d = 3; d < overlay->image.ndim(); ++d) {
              SpinBox* vol_index = new SpinBox (this);
              vol_index->setMinimum (0);
              vol_index->setPrefix (tr((str(d+1) + ": ").c_str()));;
              vol_index->setValue (overlay->image.index(d));
              vol_index->setMaximum (overlay->image.size(d) - 1);
              vol_index->setEnabled (overlay->image.size(d) > 1);
              volume_index_layout->addWidget (vol_index, volume_index_layout->count()/3, volume_index_layout->count()%3);
              connect (vol_index, SIGNAL (valueChanged(int)), this, SLOT (onSetVolumeIndex()));
            }
          }

          colourmap_button->set_colourmap_index(colourmap_index);
          opacity_slider->setValue (1.0e3f * opacity);
          if (num_interp == 0)
            interpolate_check_box->setCheckState (Qt::Unchecked);
          else if (num_interp == indices.size())
            interpolate_check_box->setCheckState (Qt::Checked);
          else
            interpolate_check_box->setCheckState (Qt::PartiallyChecked);

          min_value->setRate (rate);
          max_value->setRate (rate);
          min_value->setValue (min_val);
          max_value->setValue (max_val);

          lower_threshold->setValue (lower_threshold_val);
          lower_threshold_check_box->setCheckState (num_lower_threshold ?
              ( num_lower_threshold == indices.size() ?
                Qt::Checked :
                Qt::PartiallyChecked ) :
              Qt::Unchecked);
          lower_threshold->setRate (rate);

          upper_threshold->setValue (upper_threshold_val);
          upper_threshold_check_box->setCheckState (num_upper_threshold ?
              ( num_upper_threshold == indices.size() ?
                Qt::Checked :
                Qt::PartiallyChecked ) :
              Qt::Unchecked);
          upper_threshold->setRate (rate);
        }




        void Overlay::add_commandline_options (MR::App::OptionList& options)
        {
          using namespace MR::App;
          options
            + OptionGroup ("Overlay tool options")

            + Option ("overlay.load", "Loads the specified image on the overlay tool.").allow_multiple()
            +   Argument ("image").type_image_in()

            + Option ("overlay.opacity", "Sets the overlay opacity to floating value [0-1].").allow_multiple()
            +   Argument ("value").type_float (0.0, 1.0)

            + Option ("overlay.colourmap", "Sets the colourmap of the overlay as indexed in the colourmap dropdown menu.").allow_multiple()
            +   Argument ("index").type_integer()

            + Option ("overlay.colour", "Specify a manual colour for the overlay, as three comma-separated values").allow_multiple()
            +   Argument ("R,G,B").type_sequence_float()

            + Option ("overlay.intensity", "Set the intensity windowing of the overlay").allow_multiple()
            +   Argument ("Min,Max").type_sequence_float()

            + Option ("overlay.threshold_min", "Set the lower threshold value of the overlay").allow_multiple()
            +   Argument ("value").type_float()

            + Option ("overlay.threshold_max", "Set the upper threshold value of the overlay").allow_multiple()
            +   Argument ("value").type_float()

            + Option ("overlay.no_threshold_min", "Disable the lower threshold for the overlay").allow_multiple()
            + Option ("overlay.no_threshold_max", "Disable the upper threshold for the overlay").allow_multiple()

            + Option ("overlay.interpolation", "Enable or disable overlay image interpolation.").allow_multiple()
            +   Argument ("value").type_bool();

        }

        bool Overlay::process_commandline_option (const MR::App::ParsedOption& opt)
        {
          if (opt.opt->is ("overlay.load")) {
            vector<std::unique_ptr<MR::Header>> list;
            try { list.push_back (make_unique<MR::Header> (MR::Header::open (opt[0]))); }
            catch (Exception& e) { e.display(); }
            add_images (list);
            return true;
          }

          if (opt.opt->is ("overlay.opacity")) {
            try {
              float value = opt[0];
              opacity_slider->setSliderPosition(int(1.e3f*value));
            }
            catch (Exception& e) { e.display(); }
            return true;
          }

          if (opt.opt->is ("overlay.colourmap")) {
            try {
              int n = opt[0];
              if (n < 0 || !ColourMap::maps[n].name)
                throw Exception ("invalid overlay colourmap index \"" + std::string (opt[0]) + "\" for -overlay.colourmap option");
              colourmap_button->set_colourmap_index(n);
            }
            catch (Exception& e) { e.display(); }
            return true;
          }

          if (opt.opt->is ("overlay.colour")) {
            try {
              auto values = parse_floats (opt[0]);
              if (values.size() != 3)
                throw Exception ("must provide exactly three comma-separated values to the -overlay.colour option");
              const float max_value = std::max ({ values[0], values[1], values[2] });
              if (std::min ({ values[0], values[1], values[2] }) < 0.0 || max_value > 255)
                throw Exception ("values provided to -overlay.colour must be either between 0.0 and 1.0, or between 0 and 255");
              const float multiplier = max_value <= 1.0 ? 255.0 : 1.0;
              QColor colour (int(values[0] * multiplier), int(values[1]*multiplier), int(values[2]*multiplier));
              selected_custom_colour (colour, *colourmap_button);
              colourmap_button->set_fixed_colour();
            }
            catch (Exception& e) { e.display(); }
            return true;
          }

          if (opt.opt->is ("overlay.intensity")) {
            try {
              auto values = parse_floats (opt[0]);
              if (values.size() != 2)
                throw Exception ("must provide exactly two comma-separated values to the -overlay.intensity option");
              min_value->blockSignals (true);
              min_value->setValue (values[0]);
              min_value->blockSignals (false);
              max_value->setValue (values[1]);
            }
            catch (Exception& e) { e.display(); }
            return true;
          }

          if (opt.opt->is ("overlay.threshold_min")) {
            try {
              float value = opt[0];
              lower_threshold->setValue (value);
              lower_threshold_check_box->setChecked (true);
            }
            catch (Exception& e) { e.display(); }
            return true;
          }

          if (opt.opt->is ("overlay.threshold_max")) {
            try {
              float value = opt[0];
              upper_threshold->setValue (value);
              upper_threshold_check_box->setChecked (true);
            }
            catch (Exception& e) { e.display(); }
            return true;
          }

          if (opt.opt->is ("overlay.no_threshold_min")) {
            lower_threshold_check_box->setChecked (false);
            return true;
          }

          if (opt.opt->is ("overlay.no_threshold_max")) {
            upper_threshold_check_box->setChecked (false);
            return true;
          }

          if (opt.opt->is ("overlay.interpolation")) {
            interpolate_check_box->setCheckState (bool(opt[0]) ? Qt::Checked : Qt::Unchecked);
            interpolate_changed();
            return true;
          }


          return false;
        }




      }
    }
  }
}





