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
#include "dwi/gradient.h"
#include "dwi/shells.h"
#include "dwi/directions/set.h"
#include "gui/dialog/file.h"
#include "gui/lighting_dock.h"
#include "gui/dwi/render_frame.h"
#include "gui/mrview/window.h"
#include "gui/mrview/tool/odf/item.h"
#include "gui/mrview/tool/odf/model.h"
#include "gui/mrview/tool/odf/odf.h"
#include "gui/mrview/tool/odf/preview.h"
#include "gui/mrview/mode/base.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {




        ODF::ODF (Dock* parent) :
          Base (parent),
          preview (nullptr),
          renderer (nullptr),
          lighting_dock (nullptr),
          lmax (0),
          level_of_detail (0) {
            lighting = new GL::Lighting (this);

            VBoxLayout *main_box = new VBoxLayout (this);

            HBoxLayout* layout = new HBoxLayout;
            layout->setContentsMargins (0, 0, 0, 0);
            layout->setSpacing (0);

            QPushButton* button = new QPushButton (this);
            button->setToolTip (tr ("Open ODF image"));
            button->setIcon (QIcon (":/open.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (image_open_slot ()));
            layout->addWidget (button, 1);

            button = new QPushButton (this);
            button->setToolTip (tr ("Close ODF image"));
            button->setIcon (QIcon (":/close.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (image_close_slot ()));
            layout->addWidget (button, 1);

            hide_all_button = new QPushButton (this);
            hide_all_button->setToolTip (tr ("Hide all ODFs"));
            hide_all_button->setIcon (QIcon (":/hide.svg"));
            hide_all_button->setCheckable (true);
            connect (hide_all_button, SIGNAL (clicked()), this, SLOT (hide_all_slot ()));
            layout->addWidget (hide_all_button, 1);

            main_box->addLayout (layout, 0);


            image_list_view = new QListView (this);
            image_list_view->setSelectionMode (QAbstractItemView::SingleSelection);
            image_list_view->setDragEnabled (true);
            image_list_view->viewport()->setAcceptDrops (true);
            image_list_view->setDropIndicatorShown (true);

            image_list_model = new ODF_Model (this);
            image_list_view->setModel (image_list_model);

            main_box->addWidget (image_list_view, 1);


            show_preview_button = new QPushButton ("Inspect ODF at focus",this);
            show_preview_button->setToolTip (tr ("Inspect ODF at focus<br>(opens separate window)"));
            show_preview_button->setIcon (QIcon (":/odf_preview.svg"));
            connect (show_preview_button, SIGNAL (clicked()), this, SLOT (show_preview_slot ()));
            main_box->addWidget (show_preview_button, 1);
            

            QGroupBox* group_box = new QGroupBox (tr("Display settings"));
            main_box->addWidget (group_box);
            GridLayout* box_layout = new GridLayout;
            group_box->setLayout (box_layout);

            QLabel* label = new QLabel ("data type");
            label->setAlignment (Qt::AlignHCenter);
            box_layout->addWidget (label, 0, 0);
            type_selector = new QComboBox (this);
            type_selector->addItem ("SH");
            type_selector->addItem ("tensor");
            type_selector->addItem ("amps");
            connect (type_selector, SIGNAL (currentIndexChanged(int)), this, SLOT(mode_change_slot()));
            box_layout->addWidget (type_selector, 0, 1);

            level_of_detail_label = new QLabel ("detail");
            level_of_detail_label->setAlignment (Qt::AlignHCenter);
            box_layout->addWidget (level_of_detail_label, 1, 0);
            level_of_detail_selector = new SpinBox (this);
            level_of_detail_selector->setMinimum (1);
            level_of_detail_selector->setMaximum (6);
            level_of_detail_selector->setSingleStep (1);
            level_of_detail_selector->setValue (3);
            connect (level_of_detail_selector, SIGNAL (valueChanged(int)), this, SLOT(updateGL()));
            box_layout->addWidget (level_of_detail_selector, 1, 1);

            lmax_label = new QLabel ("lmax");
            lmax_label->setAlignment (Qt::AlignHCenter);
            box_layout->addWidget (lmax_label, 1, 2);
            lmax_selector = new SpinBox (this);
            lmax_selector->setMinimum (2);
            lmax_selector->setMaximum (16);
            lmax_selector->setSingleStep (2);
            lmax_selector->setValue (8);
            connect (lmax_selector, SIGNAL (valueChanged(int)), this, SLOT(lmax_slot(int)));
            box_layout->addWidget (lmax_selector, 1, 3);

            dirs_label = new QLabel ("directions");
            dirs_label->setAlignment (Qt::AlignHCenter);
            dirs_label->setVisible (false);
            box_layout->addWidget (dirs_label, 2, 0);
            dirs_selector = new QComboBox (this);
            dirs_selector->addItem ("DW scheme");
            dirs_selector->addItem ("Header");
            dirs_selector->addItem ("Internal");
            dirs_selector->addItem ("None");
            dirs_selector->addItem ("From file");
            dirs_selector->setVisible (false);
            connect (dirs_selector, SIGNAL (currentIndexChanged(int)), this, SLOT(dirs_slot()));
            box_layout->addWidget (dirs_selector, 2, 1);

            shell_label = new QLabel ("shell");
            shell_label->setAlignment (Qt::AlignHCenter);
            shell_label->setVisible (false);
            box_layout->addWidget (shell_label, 2, 2);
            shell_selector = new QComboBox (this);
            shell_selector->setVisible (false);
            connect (shell_selector, SIGNAL (currentIndexChanged(int)), this, SLOT(shell_slot()));
            box_layout->addWidget (shell_selector, 2, 3);

            label = new QLabel ("scale");
            label->setAlignment (Qt::AlignHCenter);
            box_layout->addWidget (label, 3, 0);
            scale = new AdjustButton (this, 1.0);
            scale->setValue (1.0);
            scale->setMin (0.0);
            connect (scale, SIGNAL (valueChanged()), this, SLOT (adjust_scale_slot()));
            box_layout->addWidget (scale, 3, 1, 1, 3);
            
            interpolation_box = new QCheckBox ("interpolation");
            interpolation_box->setChecked (true);
            connect (interpolation_box, SIGNAL (stateChanged(int)), this, SLOT (updateGL()));
            box_layout->addWidget (interpolation_box, 4, 0, 1, 2);

            hide_negative_values_box = new QCheckBox ("hide negative values");
            hide_negative_values_box->setChecked (true);
            connect (hide_negative_values_box, SIGNAL (stateChanged(int)), this, SLOT (hide_negative_values_slot(int)));
            box_layout->addWidget (hide_negative_values_box, 4, 2, 1, 2);

            lock_to_grid_box = new QCheckBox ("lock to grid");
            lock_to_grid_box->setChecked (true);
            connect (lock_to_grid_box, SIGNAL (stateChanged(int)), this, SLOT (updateGL()));
            box_layout->addWidget (lock_to_grid_box, 5, 0, 1, 2);

            colour_by_direction_box = new QCheckBox ("colour by direction");
            colour_by_direction_box->setChecked (true);
            connect (colour_by_direction_box, SIGNAL (stateChanged(int)), this, SLOT (colour_by_direction_slot(int)));
            box_layout->addWidget (colour_by_direction_box, 5, 2, 1, 2);

            colour_button = new QColorButton;
            colour_button->setVisible (false);
            connect (colour_button, SIGNAL (clicked()), this, SLOT (colour_change_slot()));
            box_layout->addWidget (colour_button, 5, 3, 1, 1);


            main_grid_box = new QCheckBox ("use main grid");
            main_grid_box->setToolTip (tr ("Show individual ODFs at the spatial resolution of the main image instead of the ODF image's own spatial resolution"));
            main_grid_box->setChecked (false);
            connect (main_grid_box, SIGNAL (stateChanged(int)), this, SLOT (updateGL()));
            box_layout->addWidget (main_grid_box, 6, 0, 1, 2);

            use_lighting_box = new QCheckBox ("use lighting");
            use_lighting_box->setCheckable (true);
            use_lighting_box->setChecked (true);
            connect (use_lighting_box, SIGNAL (stateChanged(int)), this, SLOT (use_lighting_slot(int)));
            box_layout->addWidget (use_lighting_box, 6, 2, 1, 2);

            QPushButton *lighting_settings_button = new QPushButton ("ODF lighting...", this);
            connect (lighting_settings_button, SIGNAL(clicked(bool)), this, SLOT (lighting_settings_slot (bool)));
            box_layout->addWidget (lighting_settings_button, 7, 0, 1, 4);


            connect (image_list_view->selectionModel(),
                SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
                SLOT (selection_changed_slot(const QItemSelection &, const QItemSelection &)) );

            connect (lighting, SIGNAL (changed()), this, SLOT (updateGL()));


            renderer = new DWI::Renderer ((QGLWidget*)Window::main->glarea);
            renderer->initGL();
            colour_button->setColor (renderer->get_colour());


            hide_negative_values_slot (0);
            colour_by_direction_slot (0);
            lmax_slot (0);
            adjust_scale_slot ();

          }

        ODF::~ODF()
        {
          if (renderer) {
            delete renderer;
            renderer = nullptr;
          }
          if (preview) {
            delete preview;
            preview = nullptr;
          }
          if (lighting_dock) {
            delete lighting_dock;
            lighting_dock = nullptr;
          }
        }






        void ODF::draw (const Projection& projection, bool is_3D, int, int)
        {
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
          if (is_3D) 
            return;

          ODF_Item* settings = get_image();
          if (!settings)
            return;

          if (settings->mode == GUI::DWI::Renderer::mode_t::DIXEL && settings->dixel.dir_type == ODF_Item::DixelPlugin::dir_t::NONE)
            return;

          MRView::Image& image (main_grid_box->isChecked() ? *window().image() : settings->image);

          if (!hide_all_button->isChecked()) {

            if (settings->mode == GUI::DWI::Renderer::mode_t::SH) {
              if (lmax != settings->lmax || level_of_detail != level_of_detail_selector->value()) {
                lmax = settings->lmax;
                level_of_detail = level_of_detail_selector->value();
                renderer->sh.update_mesh (level_of_detail, lmax);
              }
            } else if (settings->mode == GUI::DWI::Renderer::mode_t::TENSOR) {
              if (level_of_detail != level_of_detail_selector->value()) {
                level_of_detail = level_of_detail_selector->value();
                renderer->tensor.update_mesh (level_of_detail);
              }
            }

            renderer->set_mode (settings->mode);

            renderer->start (projection, *lighting, settings->scale, 
                use_lighting_box->isChecked(), settings->color_by_direction, settings->hide_negative, true);

            gl::Enable (gl::DEPTH_TEST);
            gl::DepthMask (gl::TRUE_);

            Eigen::Vector3f pos (window().target());
            pos += projection.screen_normal() * (projection.screen_normal().dot (window().focus() - window().target()));
            if (lock_to_grid_box->isChecked()) {
              Eigen::Vector3f p = image.transform().scanner2voxel.cast<float>() * pos;
              p[0] = std::round (p[0]);
              p[1] = std::round (p[1]);
              p[2] = std::round (p[2]);
              pos = image.transform().voxel2scanner.cast<float>() * p;
            }

            Eigen::Vector3f x_dir = projection.screen_to_model_direction (1.0, 0.0, projection.depth_of (pos));
            x_dir.normalize();
            x_dir = image.transform().scanner2image.rotation().cast<float>() * x_dir;
            x_dir[0] *= image.header().spacing (0);
            x_dir[1] *= image.header().spacing (1);
            x_dir[2] *= image.header().spacing (2);
            x_dir = image.transform().image2scanner.rotation().cast<float>() * x_dir;

            Eigen::Vector3f y_dir = projection.screen_to_model_direction (0.0, 1.0, projection.depth_of (pos));
            y_dir.normalize();
            y_dir = image.transform().scanner2image.rotation().cast<float>() * y_dir;
            y_dir[0] *= image.header().spacing (0);
            y_dir[1] *= image.header().spacing (1);
            y_dir[2] *= image.header().spacing (2);
            y_dir = image.transform().image2scanner.rotation().cast<float>() * y_dir;

            const Eigen::Vector3f x_width = projection.screen_to_model_direction (projection.width()/2.0, 0.0, projection.depth_of (pos));
            const int nx = std::ceil (x_width.norm() / x_dir.norm());
            const Eigen::Vector3f y_width = projection.screen_to_model_direction (0.0, projection.height()/2.0, projection.depth_of (pos));
            const int ny = std::ceil (y_width.norm() / y_dir.norm());

            Eigen::VectorXf values;
            switch (settings->mode) {
              case GUI::DWI::Renderer::mode_t::SH:
                values.resize (Math::SH::NforL (settings->lmax));
                break;
              case GUI::DWI::Renderer::mode_t::TENSOR:
                values.resize (6);
                break;
              case GUI::DWI::Renderer::mode_t::DIXEL:
                values.resize (settings->image.header().size (3));
                break;
            }
            Eigen::VectorXf r_del_daz;

            for (int y = -ny; y <= ny; ++y) {
              for (int x = -nx; x <= nx; ++x) {
                Eigen::Vector3f p = pos + float(x)*x_dir + float(y)*y_dir;

                // values gets shrunk by the previous get_values() call
                if (settings->mode == GUI::DWI::Renderer::mode_t::DIXEL && settings->dixel.dir_type == ODF_Item::DixelPlugin::dir_t::DW_SCHEME)
                  values.resize (settings->image.header().size (3));

                get_values (values, *settings, p, interpolation_box->isChecked());
                if (!std::isfinite (values[0])) continue;

                switch (settings->mode) {
                  case GUI::DWI::Renderer::mode_t::SH:
                    if (values[0] == 0.0) continue;
                    renderer->sh.compute_r_del_daz (r_del_daz, values.topRows (Math::SH::NforL (lmax)));
                    renderer->sh.set_data (r_del_daz);
                    break;
                  case GUI::DWI::Renderer::mode_t::TENSOR:
                    renderer->tensor.set_data (values);
                    break;
                  case GUI::DWI::Renderer::mode_t::DIXEL:
                    renderer->dixel.set_data (values);
                    break;
                }

                GL_CHECK_ERROR;
                renderer->draw (p);
                GL_CHECK_ERROR;
              }
            }

            renderer->stop();

            gl::Disable (gl::DEPTH_TEST);
            gl::DepthMask (gl::FALSE_);
          }
          ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT;
        }







        inline ODF_Item* ODF::get_image ()
        {
          QModelIndexList list = image_list_view->selectionModel()->selectedRows();
          if (!list.size())
            return nullptr;
          return image_list_model->get_image (list[0]);
        }






        void ODF::get_values (Eigen::VectorXf& values, ODF_Item& item, const Eigen::Vector3f& pos, const bool interp)
        {
          MRView::Image& image (item.image);
          values.setZero();
          if (interp) {
            if (image.linear_interp.scanner (pos)) {
              for (image.linear_interp.index(3) = 0; image.linear_interp.index(3) < std::min (ssize_t(values.size()), image.linear_interp.size(3)); ++image.linear_interp.index(3))
                values[image.linear_interp.index(3)] = image.linear_interp.value().real();
            }
          } else {
            if (image.nearest_interp.scanner (pos)) {
              for (image.nearest_interp.index(3) = 0; image.nearest_interp.index(3) < std::min (ssize_t(values.size()), image.nearest_interp.size(3)); ++image.nearest_interp.index(3))
                values[image.nearest_interp.index(3)] = image.nearest_interp.value().real();
            }
          }
          if (item.mode == GUI::DWI::Renderer::mode_t::DIXEL && item.dixel.dir_type == ODF_Item::DixelPlugin::dir_t::DW_SCHEME) {
            Eigen::VectorXf shell_values = item.dixel.get_shell_data (values);
            std::swap (values, shell_values);
          }
        }




        void ODF::setup_ODFtype_UI (const ODF_Item* image)
        {
          assert (image);
          type_selector->blockSignals (true);
          type_selector->setCurrentIndex (image->mode == GUI::DWI::Renderer::mode_t::SH ? 0 :
                                         (image->mode == GUI::DWI::Renderer::mode_t::TENSOR ? 1 : 2));
          type_selector->blockSignals (false);
          lmax_label->setVisible (image->mode == GUI::DWI::Renderer::mode_t::SH);
          lmax_selector->setVisible (image->mode == GUI::DWI::Renderer::mode_t::SH);
          type_selector->blockSignals (true);
          if (image->mode == GUI::DWI::Renderer::mode_t::SH)
            lmax_selector->setValue (image->lmax);
          type_selector->blockSignals (false);
          level_of_detail_label->setVisible (image->mode == GUI::DWI::Renderer::mode_t::SH || image->mode == GUI::DWI::Renderer::mode_t::TENSOR);
          level_of_detail_selector->setVisible (image->mode == GUI::DWI::Renderer::mode_t::SH || image->mode == GUI::DWI::Renderer::mode_t::TENSOR);
          dirs_label->setVisible (image->mode == GUI::DWI::Renderer::mode_t::DIXEL);
          shell_label->setVisible (image->mode == GUI::DWI::Renderer::mode_t::DIXEL);
          dirs_selector->setVisible (image->mode == GUI::DWI::Renderer::mode_t::DIXEL);
          if (image->mode == GUI::DWI::Renderer::mode_t::DIXEL)
            dirs_selector->setCurrentIndex (image->dixel.dir_type);
          shell_selector->setVisible (image->mode == GUI::DWI::Renderer::mode_t::DIXEL);
          shell_selector->blockSignals (true);
          shell_selector->clear();
          if (image->dixel.shells) {
            for (size_t i = 0; i != image->dixel.shells->count(); ++i) {
              if (!(*image->dixel.shells)[i].is_bzero())
                shell_selector->addItem (QString::fromStdString (str (int (std::round ((*image->dixel.shells)[i].get_mean())))));
            }
            if (image->mode == GUI::DWI::Renderer::mode_t::DIXEL && shell_selector->count() && image->dixel.dir_type == ODF_Item::DixelPlugin::dir_t::DW_SCHEME)
              shell_selector->setCurrentIndex (image->dixel.shell_index - (image->dixel.shells->smallest().is_bzero() ? 1 : 0));
          }
          shell_selector->blockSignals (false);
          shell_selector->setEnabled (image->mode == GUI::DWI::Renderer::mode_t::DIXEL && image->dixel.dir_type == ODF_Item::DixelPlugin::dir_t::DW_SCHEME && image->dixel.shells && image->dixel.shells->count() > 1);
          if (preview)
            preview->set_lod_enabled (image->mode != GUI::DWI::Renderer::mode_t::DIXEL);
        }





        void ODF::add_images (std::vector<std::string>& list)
        {
          size_t previous_size = image_list_model->rowCount();
          if (!image_list_model->add_items (list,
                                            colour_by_direction_box->isChecked(),
                                            hide_negative_values_box->isChecked(),
                                            scale->value()))
            return;
          QModelIndex first = image_list_model->index (previous_size, 0, QModelIndex());
          image_list_view->selectionModel()->select (first, QItemSelectionModel::ClearAndSelect);
          ODF_Item* settings = get_image();
          setup_ODFtype_UI (settings);
          assert (renderer);
          if (settings->mode == GUI::DWI::Renderer::mode_t::DIXEL) {
            assert (settings->dixel.dirs);
            renderer->dixel.update_mesh (*(settings->dixel.dirs));
            if (preview)
              preview->render_frame->set_dixels (*(settings->dixel.dirs));
          }
          updateGL();
        }



        void ODF::showEvent (QShowEvent*)
        {
          connect (&window(), SIGNAL (focusChanged()), this, SLOT (onWindowChange()));
          connect (&window(), SIGNAL (targetChanged()), this, SLOT (onWindowChange()));
          connect (&window(), SIGNAL (orientationChanged()), this, SLOT (onWindowChange()));
          connect (&window(), SIGNAL (planeChanged()), this, SLOT (onWindowChange()));
          onWindowChange();
        }

        void ODF::closeEvent (QCloseEvent*) {
          window().disconnect (this);
        }




        void ODF::onWindowChange ()
        {
          update_preview();
        }



        void ODF::onPreviewClosed ()
        {
          show_preview_button->setChecked (false);
        }





        void ODF::image_open_slot ()
        {
          std::vector<std::string> list = Dialog::File::get_images (&window(), "Select overlay images to open");
          if (list.empty())
            return;

          add_images (list);
        }






        void ODF::image_close_slot ()
        {
          QModelIndexList indexes = image_list_view->selectionModel()->selectedIndexes();
          if (indexes.size())
            image_list_model->remove_item (indexes.first());
          updateGL();
        }


        void ODF::show_preview_slot ()
        {
          if (!preview) {
            preview = new ODF_Preview (this);
            connect (lighting, SIGNAL (changed()), preview, SLOT (lighting_update_slot()));
          }

          ODF_Item* settings = get_image();
          if (settings) {
            preview->render_frame->set_mode (settings->mode);
            preview->render_frame->set_scale (settings->scale);
            preview->render_frame->set_hide_neg_values (settings->hide_negative);
            preview->render_frame->set_color_by_dir (settings->color_by_direction);
            preview->set_lod_enabled (settings->mode != GUI::DWI::Renderer::mode_t::DIXEL);

            if (settings->mode == GUI::DWI::Renderer::mode_t::SH)
              preview->render_frame->set_lmax (settings->lmax);
            else if (settings->dixel.dirs)
              preview->render_frame->set_dixels (*(settings->dixel.dirs));
          }
          preview->render_frame->set_colour (renderer->get_colour());

          preview->show();
          update_preview();
        }


        void ODF::hide_all_slot ()
        {
          window().updateGL();
        }



        void ODF::colour_by_direction_slot (int) 
        {
          if (colour_by_direction_box->isChecked()) {
            colour_by_direction_box->setText ("colour by direction");
            colour_button->setVisible (false);
          } else {
            colour_by_direction_box->setText ("colour");
            colour_button->setVisible (true);
          }
          ODF_Item* settings = get_image();
          if (!settings)
            return;
          settings->color_by_direction = colour_by_direction_box->isChecked();
          if (preview)
            preview->render_frame->set_color_by_dir (colour_by_direction_box->isChecked());
          updateGL();
          update_preview();
        }





        void ODF::hide_negative_values_slot (int)
        {
          ODF_Item* settings = get_image();
          if (!settings)
            return;
          settings->hide_negative = hide_negative_values_box->isChecked();
          if (preview) 
            preview->render_frame->set_hide_neg_values (hide_negative_values_box->isChecked());
          updateGL();
          update_preview();
        }


        void ODF::colour_change_slot()
        {
          assert (!colour_by_direction_box->isChecked());
          const QColor c = colour_button->color();
          renderer->set_colour (c);
          if (preview)
            preview->render_frame->set_colour (c);
          updateGL();
          update_preview();
        }





        void ODF::mode_change_slot ()
        {
          ODF_Item* settings = get_image();
          if (!settings)
            return;
          const GUI::DWI::Renderer::mode_t mode = (!type_selector->currentIndex() ? GUI::DWI::Renderer::mode_t::SH :
                                                  (type_selector->currentIndex() == 1 ? GUI::DWI::Renderer::mode_t::TENSOR : GUI::DWI::Renderer::mode_t::DIXEL));
          if (settings->mode == mode)
            return;

          auto reset_GUI = [&] (const GUI::DWI::Renderer::mode_t mode) {
            switch (mode) {
              case GUI::DWI::Renderer::mode_t::SH:     type_selector->setCurrentIndex (0); break;
              case GUI::DWI::Renderer::mode_t::TENSOR: type_selector->setCurrentIndex (1); break;
              case GUI::DWI::Renderer::mode_t::DIXEL:  type_selector->setCurrentIndex (2); break;
            }
          };

          // Mode change combobox is now always enabled;
          // Need to detect bad selections here and act accordingly
          switch (mode) {

            case GUI::DWI::Renderer::mode_t::SH:
              try {
                Math::SH::check (settings->image.header());
              } catch (Exception& e) {
                e.display();
                reset_GUI (settings->mode);
                return;
              }
              renderer->sh.update_mesh (level_of_detail, lmax);
              break;

            case GUI::DWI::Renderer::mode_t::TENSOR:
              if (settings->image.header().size(3) != 6) {
                Exception e ("Cannot use image " + settings->image.header().name() + " as tensor overlay; must have 6 volumes");
                e.display();
                reset_GUI (settings->mode);
                return;
              }
              renderer->tensor.update_mesh (level_of_detail);
              break;

            case GUI::DWI::Renderer::mode_t::DIXEL:
              break;

          }

          settings->mode = mode;
          setup_ODFtype_UI (settings);
          if (preview)
            preview->render_frame->set_mode (mode);
          updateGL();
          update_preview();
        }



        void ODF::lmax_slot (int) 
        { 
          ODF_Item* settings = get_image();
          if (!settings)
            return;
          assert (settings->mode == GUI::DWI::Renderer::mode_t::SH);
          settings->lmax = lmax_selector->value();
          if (preview) 
            preview->render_frame->set_lmax (lmax_selector->value());
          updateGL();
        }



        void ODF::dirs_slot ()
        {
          ODF_Item* settings = get_image();
          if (!settings)
            return;
          assert (settings->mode == GUI::DWI::Renderer::mode_t::DIXEL);
          const size_t dir_type = dirs_selector->currentIndex();
          if (dir_type == settings->dixel.dir_type)
            return;
          try {
            switch (dir_type) {
              case 0: // DW scheme
                if (!settings->dixel.num_DW_shells())
                  throw Exception ("Cannot draw orientation information from DW scheme: no such scheme stored in header");
                settings->dixel.set_shell (settings->dixel.shell_index);
                break;
              case 1: // Header
                if (!settings->dixel.header_dirs.rows())
                  throw Exception ("Cannot draw orientation information from header: no such data exist");
                settings->dixel.set_header();
                break;
              case 2: // Internal
                settings->dixel.set_internal (settings->image.header().size (3));
                break;
              case 3: // None
                settings->dixel.set_none();
                if (preview)
                  preview->render_frame->clear_dixels();
                break;
              case 4: // From file
                const std::string path = Dialog::File::get_file (this, "Select directions file", "Text files (*.txt)");
                if (!path.size()) {
                  dirs_selector->setCurrentIndex (settings->dixel.dir_type);
                  return;
                }
                settings->dixel.set_from_file (path);
                break;
            }
            shell_selector->setEnabled (dir_type == 0 && settings->dixel.shells && settings->dixel.shells->count() > 1);
            if (dir_type != 3) {
              assert (settings->dixel.dirs);
              assert (renderer);
              renderer->dixel.update_mesh (*(settings->dixel.dirs));
              if (preview)
                preview->render_frame->set_dixels (*(settings->dixel.dirs));
            }
          } catch (Exception& e) {
            e.display();
            dirs_selector->setCurrentIndex (settings->dixel.dir_type);
          }

          updateGL();
        }



        void ODF::shell_slot ()
        {
          ODF_Item* settings = get_image();
          if (!settings)
            return;
          assert (settings->mode == GUI::DWI::Renderer::mode_t::DIXEL);
          assert (settings->dixel.dir_type == ODF_Item::DixelPlugin::dir_t::DW_SCHEME);
          size_t index = shell_selector->currentIndex();
          if (settings->dixel.shells->smallest().is_bzero())
            ++index;
          assert (index < settings->dixel.num_DW_shells());
          settings->dixel.set_shell (index);
          assert (settings->dixel.dirs);
          assert (renderer);
          renderer->dixel.update_mesh (*(settings->dixel.dirs));
          if (preview)
            preview->render_frame->set_dixels (*(settings->dixel.dirs));
          updateGL();
        }



        void ODF::adjust_scale_slot ()
        {
          scale->setRate (0.01 * scale->value());
          ODF_Item* settings = get_image();
          if (!settings)
            return;
          settings->scale = scale->value();
          if (preview)
            preview->render_frame->set_scale (scale->value());
          updateGL();
        }



        void ODF::use_lighting_slot (int)
        {
          if (preview) 
            preview->render_frame->set_use_lighting (use_lighting_box->isChecked());
          updateGL();
          update_preview();
        }




        void ODF::lighting_settings_slot (bool)
        {
          if (!lighting_dock) {
            lighting_dock = new LightingDock("ODF lighting", *lighting);
            window().addDockWidget (Qt::RightDockWidgetArea, lighting_dock);
          }
          lighting_dock->show();
        }






        void ODF::close_event ()
        {
          if (preview) 
            preview->hide();
          if (lighting_dock)
            lighting_dock->hide();
        }



        void ODF::updateGL () 
        {
          if (!hide_all_button->isChecked())
            window().updateGL();
        }

        void ODF::update_preview()
        {
          if (!preview) 
            return;
          if (!preview->isVisible())
            return;
          ODF_Item* settings = get_image();
          if (!settings)
            return;
          Eigen::VectorXf values;
          switch (settings->mode) {
            case GUI::DWI::Renderer::mode_t::SH:
              values.resize (Math::SH::NforL (lmax_selector->value()));
              break;
            case GUI::DWI::Renderer::mode_t::TENSOR:
              values.resize (6);
              break;
            case GUI::DWI::Renderer::mode_t::DIXEL:
              values.resize (settings->image.header().size (3));
              break;
          }
          get_values (values, *settings, window().focus(), preview->interpolate());
          preview->set (values);
          preview->set_lod_enabled (settings->mode != GUI::DWI::Renderer::mode_t::DIXEL);
          preview->lock_orientation_to_image_slot (0);
        }




        void ODF::selection_changed_slot (const QItemSelection &, const QItemSelection &)
        {
          ODF_Item* settings = get_image();
          if (!settings)
            return;
          if (settings->mode == GUI::DWI::Renderer::mode_t::DIXEL && settings->dixel.dirs)
            renderer->dixel.update_mesh (*(settings->dixel.dirs));
          setup_ODFtype_UI (settings);
          scale->setValue (settings->scale);
          hide_negative_values_box->setChecked (settings->hide_negative);
          colour_by_direction_box->setChecked (settings->color_by_direction);
          if (preview) {
            preview->render_frame->set_mode (settings->mode);
            preview->render_frame->set_scale (settings->scale);
            preview->render_frame->set_hide_neg_values (settings->hide_negative);
            preview->render_frame->set_color_by_dir (settings->color_by_direction);
            preview->set_lod_enabled (settings->mode != GUI::DWI::Renderer::mode_t::DIXEL);
            if (settings->mode == GUI::DWI::Renderer::mode_t::SH) {
              preview->render_frame->set_lmax (settings->lmax);
            } else if (settings->mode == GUI::DWI::Renderer::mode_t::DIXEL) {
              if (settings->dixel.dirs)
                preview->render_frame->set_dixels (*(settings->dixel.dirs));
              else
                preview->render_frame->clear_dixels();
            }
          }
          updateGL();
          update_preview();
        }





        void ODF::add_commandline_options (MR::App::OptionList& options) 
        { 
          using namespace MR::App;
          options
            + OptionGroup ("ODF tool options")

            + Option ("odf.load", "Loads the specified image on the ODF tool.")
            +   Argument ("image").type_image_in();
            
        }





        bool ODF::process_commandline_option (const MR::App::ParsedOption& opt) 
        {
          if (opt.opt->is ("odf.load")) {
            try {
              std::vector<std::string> list (1, opt[0]);
              add_images (list);
            }
            catch (Exception& e) {
              e.display();
            }
            return true;
          }

          return false;
        }



      }
    }
  }
}





