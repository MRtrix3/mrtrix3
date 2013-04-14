/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 13/11/09.

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
#include <QSplitter>
#include <QListView>
#include <QCheckBox>
#include <QComboBox>

#include "mrtrix.h"
#include "gui/dialog/file.h"
#include "gui/dwi/render_frame.h"
#include "gui/mrview/window.h"
#include "gui/mrview/tool/odf.h"
#include "gui/mrview/tool/list_model_base.h"
#include "gui/mrview/mode/base.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        class ODF::Model : public ListModelBase
        {
          public:
            Model (QObject* parent) : 
              ListModelBase (parent) { }

            QVariant data (const QModelIndex& index, int role) const {
              if (!index.isValid()) return QVariant();
              if (role != Qt::DisplayRole) return QVariant();
              return shorten (items[index.row()]->get_filename(), 35, 0).c_str();
            }

            bool setData (const QModelIndex& index, const QVariant& value, int role) {
              return QAbstractItemModel::setData (index, value, role);
            }

            Qt::ItemFlags flags (const QModelIndex& index) const {
              if (!index.isValid()) return 0;
              return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
            }

            void add_items (const std::vector<std::string>& list) {
              beginInsertRows (QModelIndex(), items.size(), items.size()+list.size());
              for (size_t i = 0; i < list.size(); ++i) {
                try {
                  MR::Image::Header header (list[i]);
                  items.push_back (new Image (header));
                }
                catch (Exception& E) {
                  E.display();
                }
              }
              endInsertRows();
            }

            Image* get_image (QModelIndex& index) {
              return index.isValid() ? dynamic_cast<Image*>(items[index.row()]) : NULL;
            }
        };




        ODF::ODF (Window& main_window, Dock* parent) :
          Base (main_window, parent),
          overlay_renderer (NULL),
          overlay_lmax (0),
          overlay_level_of_detail (0) { 
            QVBoxLayout *main_box = new QVBoxLayout (this);
            main_box->setContentsMargins (0, 0, 0, 0);
            main_box->setSpacing (0);

            QSplitter* splitter = new QSplitter (Qt::Vertical, parent);
            main_box->addWidget (splitter);

            render_frame = new DWI::RenderFrame (this);
            splitter->addWidget (render_frame);



            QFrame* frame = new QFrame (this);
            frame->setFrameStyle (QFrame::NoFrame);
            splitter->addWidget (frame);

            main_box = new QVBoxLayout (frame);

            QHBoxLayout* layout = new QHBoxLayout;
            layout->setContentsMargins (0, 0, 0, 0);
            layout->setSpacing (0);

            QPushButton* button = new QPushButton (this);
            button->setToolTip (tr ("Open Image"));
            button->setIcon (QIcon (":/open.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (image_open_slot ()));
            layout->addWidget (button, 1);

            button = new QPushButton (this);
            button->setToolTip (tr ("Close Image"));
            button->setIcon (QIcon (":/close.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (image_close_slot ()));
            layout->addWidget (button, 1);

            main_box->addLayout (layout, 0);

            image_list_view = new QListView (this);
            image_list_view->setSelectionMode (QAbstractItemView::SingleSelection);
            image_list_view->setDragEnabled (true);
            image_list_view->viewport()->setAcceptDrops (true);
            image_list_view->setDropIndicatorShown (true);

            image_list_model = new Model (this);
            image_list_view->setModel (image_list_model);

            main_box->addWidget (image_list_view, 1);


            QGroupBox* group_box = new QGroupBox (tr("Display settings"));
            main_box->addWidget (group_box);
            QGridLayout* box_layout = new QGridLayout;
            box_layout->setContentsMargins (0, 0, 0, 0);
            box_layout->setSpacing (0);
            group_box->setLayout (box_layout);

            lock_orientation_to_image_box = new QCheckBox ("auto align");
            lock_orientation_to_image_box->setChecked (true);
            connect (lock_orientation_to_image_box, SIGNAL (stateChanged(int)), this, SLOT (lock_orientation_to_image_slot(int)));
            box_layout->addWidget (lock_orientation_to_image_box, 0, 0, 1, 2);

            interpolation_box = new QCheckBox ("interpolation");
            interpolation_box->setChecked (true);
            connect (interpolation_box, SIGNAL (stateChanged(int)), this, SLOT (interpolation_slot(int)));
            box_layout->addWidget (interpolation_box, 1, 0, 1, 2);

            show_axes_box = new QCheckBox ("show axes");
            show_axes_box->setChecked (true);
            connect (show_axes_box, SIGNAL (stateChanged(int)), this, SLOT (show_axes_slot(int)));
            box_layout->addWidget (show_axes_box, 2, 0, 1, 2);

            colour_by_direction_box = new QCheckBox ("colour by direction");
            colour_by_direction_box->setChecked (true);
            connect (colour_by_direction_box, SIGNAL (stateChanged(int)), this, SLOT (colour_by_direction_slot(int)));
            box_layout->addWidget (colour_by_direction_box, 3, 0, 1, 2);

            use_lighting_box = new QCheckBox ("use lighting");
            use_lighting_box->setChecked (true);
            connect (use_lighting_box, SIGNAL (stateChanged(int)), this, SLOT (use_lighting_slot(int)));
            box_layout->addWidget (use_lighting_box, 4, 0, 1, 2);

            hide_negative_lobes_box = new QCheckBox ("hide negative lobes");
            hide_negative_lobes_box->setChecked (true);
            connect (hide_negative_lobes_box, SIGNAL (stateChanged(int)), this, SLOT (hide_negative_lobes_slot(int)));
            box_layout->addWidget (hide_negative_lobes_box, 5, 0, 1, 2);


            box_layout->addWidget (new QLabel ("lmax"), 6, 0);
            lmax_selector = new QSpinBox (this);
            lmax_selector->setMinimum (2);
            lmax_selector->setMaximum (16);
            lmax_selector->setSingleStep (2);
            lmax_selector->setValue (8);
            connect (lmax_selector, SIGNAL (valueChanged(int)), this, SLOT(lmax_slot(int)));
            box_layout->addWidget (lmax_selector, 6, 1);

            box_layout->addWidget (new QLabel ("detail"), 7, 0);
            level_of_detail_selector = new QSpinBox (this);
            level_of_detail_selector->setMinimum (1);
            level_of_detail_selector->setMaximum (7);
            level_of_detail_selector->setSingleStep (1);
            level_of_detail_selector->setValue (4);
            connect (level_of_detail_selector, SIGNAL (valueChanged(int)), this, SLOT(level_of_detail_slot(int)));
            box_layout->addWidget (level_of_detail_selector, 7, 1);


            overlay_frame = new QGroupBox (tr("Overlay"));
            overlay_frame->setCheckable (true);
            overlay_frame->setChecked (false);
            connect (overlay_frame, SIGNAL (clicked()), this, SLOT(overlay_toggled_slot()));
            main_box->addWidget (overlay_frame);
            box_layout = new QGridLayout;
            overlay_frame->setLayout (box_layout);

            box_layout->addWidget (new QLabel ("scale"), 0, 0, 1, 1);
            overlay_scale = new AdjustButton (this, 1.0);
            overlay_scale->setValue (1.0);
            overlay_scale->setMin (0.0);
            connect (overlay_scale, SIGNAL (valueChanged()), this, SLOT (overlay_scale_slot()));
            box_layout->addWidget (overlay_scale, 0, 1, 1, 1);

            box_layout->addWidget (new QLabel ("detail"), 1, 0, 1,1);
            overlay_level_of_detail_selector = new QSpinBox (this);
            overlay_level_of_detail_selector->setMinimum (1);
            overlay_level_of_detail_selector->setMaximum (6);
            overlay_level_of_detail_selector->setSingleStep (1);
            overlay_level_of_detail_selector->setValue (3);
            connect (overlay_level_of_detail_selector, SIGNAL (valueChanged(int)), this, SLOT(overlay_update_slot(int)));
            box_layout->addWidget (overlay_level_of_detail_selector, 1, 1, 1, 1);

            box_layout->addWidget (new QLabel ("grid"), 2, 0, 1, 1);
            overlay_grid_selector = new QComboBox (this);
            overlay_grid_selector->addItem ("overlay");
            overlay_grid_selector->addItem ("main");
            connect (overlay_grid_selector, SIGNAL (activated(int)), this, SLOT(overlay_update_slot(int)));
            box_layout->addWidget (overlay_grid_selector, 2, 1, 1, 1);

            overlay_lock_to_grid_box = new QCheckBox ("lock to grid");
            overlay_lock_to_grid_box->setChecked (true);
            connect (overlay_lock_to_grid_box, SIGNAL (stateChanged(int)), this, SLOT (overlay_update_slot(int)));
            box_layout->addWidget (overlay_lock_to_grid_box, 3, 0, 1, 2);

            splitter->setStretchFactor (0, 1);
            splitter->setStretchFactor (1, 0);

            connect (image_list_view->selectionModel(),
                SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
                SLOT (selection_changed_slot(const QItemSelection &, const QItemSelection &)) );

            hide_negative_lobes_slot (0);
            show_axes_slot (0);
            colour_by_direction_slot (0);
            use_lighting_slot (0);
            lmax_slot (0);
            level_of_detail_slot (0);
            lock_orientation_to_image_slot (0);

            overlay_scale_slot ();
          }






        void ODF::draw2D (const Projection& projection) 
        {
          lock_orientation_to_image_slot(0);

          Image* image = get_image();
          if (!image)
            return;

          if (overlay_frame->isChecked()) {
          
            if (!overlay_renderer) {
              overlay_renderer = new DWI::Renderer;
              overlay_renderer->initGL();
            }

            if (overlay_lmax != lmax_selector->value() || 
                overlay_level_of_detail != overlay_level_of_detail_selector->value()) {
              overlay_lmax = lmax_selector->value();
              overlay_level_of_detail = overlay_level_of_detail_selector->value();
              overlay_renderer->update_mesh (overlay_level_of_detail, overlay_lmax);
            }

            overlay_renderer->start (projection, render_frame->lighting, overlay_scale->value(), 
              use_lighting_box->isChecked(), colour_by_direction_box->isChecked(), hide_negative_lobes_box->isChecked());

            glEnable (GL_DEPTH_TEST);
            glDepthMask (GL_TRUE);

            Point<> pos (window.target());
            pos += projection.screen_normal() * (projection.screen_normal().dot (window.focus() - window.target()));
            if (overlay_lock_to_grid_box->isChecked()) {
              Point<> p = image->interp.scanner2voxel (pos);
              p[0] = Math::round (p[0]);
              p[1] = Math::round (p[1]);
              p[2] = Math::round (p[2]);
              pos = image->interp.voxel2scanner (p);
            }
            
            Point<> x_dir = projection.screen_to_model_direction (1.0, 0.0, projection.depth_of (pos));
            x_dir.normalise();
            x_dir = image->interp.scanner2image_dir (x_dir);
            x_dir[0] *= image->interp.vox(0);
            x_dir[1] *= image->interp.vox(1);
            x_dir[2] *= image->interp.vox(2);
            x_dir = image->interp.image2scanner_dir (x_dir);

            Point<> y_dir = projection.screen_to_model_direction (0.0, 1.0, projection.depth_of (pos));
            y_dir.normalise();
            y_dir = image->interp.scanner2image_dir (y_dir);
            y_dir[0] *= image->interp.vox(0);
            y_dir[1] *= image->interp.vox(1);
            y_dir[2] *= image->interp.vox(2);
            y_dir = image->interp.image2scanner_dir (y_dir);

            Point<> x_width = projection.screen_to_model_direction (projection.width()/2.0, 0.0, projection.depth_of (pos));
            int nx = Math::ceil (x_width.norm() / x_dir.norm());
            Point<> y_width = projection.screen_to_model_direction (0.0, projection.height()/2.0, projection.depth_of (pos));
            int ny = Math::ceil (y_width.norm() / y_dir.norm());

            Math::Vector<float> values (image->interp.dim(3));
            Math::Vector<float> r_del_daz;

            for (int y = -ny; y < ny; ++y) {
              for (int x = -nx; x < nx; ++x) {
                Point<> p = pos + float(x)*x_dir + float(y)*y_dir;
                get_values (values, *image, p);
                if (!finite (values[0])) continue;
                if (values[0] == 0.0) continue;
                overlay_renderer->compute_r_del_daz (r_del_daz, values.sub (0, Math::SH::NforL (overlay_lmax)));
                overlay_renderer->set_data (r_del_daz);
                overlay_renderer->draw (p);
              }
            }

            overlay_renderer->stop();

            glDisable (GL_DEPTH_TEST);
            glDepthMask (GL_FALSE);
          }
        }




        void ODF::draw3D (const Projection& transform)
        { 
        }


        void ODF::showEvent (QShowEvent* event) 
        {
          connect (&window, SIGNAL (focusChanged()), this, SLOT (onFocusChanged()));
          onFocusChanged();
        }


        void ODF::closeEvent (QCloseEvent* event) { window.disconnect (this); }

        void ODF::onFocusChanged () 
        {
          Image* image = get_image();
          if (!image)
            return;

          Math::Vector<float> values (image->interp.dim(3));
          get_values (values, *image, window.focus());
          render_frame->set (values);
        }

        inline Image* ODF::get_image () 
        {
          QModelIndexList list = image_list_view->selectionModel()->selectedRows();
          if (!list.size())
            return NULL;
          return image_list_model->get_image (list[0]);
        }

        void ODF::get_values (Math::Vector<float>& values, Image& image, const Point<>& pos)
        {
          Point<> p = image.interp.scanner2voxel (pos);
          if (!interpolation_box->isChecked()) {
            p[0] = Math::round (p[0]);
            p[1] = Math::round (p[1]);
            p[2] = Math::round (p[2]);
          }
          image.interp.voxel (p);
          for (image.interp[3] = 0; image.interp[3] < image.interp.dim(3); ++image.interp[3])
            values[image.interp[3]] = image.interp.value().real(); 
        }



        void ODF::image_open_slot ()
        {
          std::vector<std::string> list = Dialog::File::get_images (this, "Select overlay images to open");
          if (list.empty())
            return;

          size_t previous_size = image_list_model->rowCount();
          image_list_model->add_items (list);
          QModelIndex first = image_list_model->index (previous_size, 0, QModelIndex());
          image_list_view->selectionModel()->select (first, QItemSelectionModel::ClearAndSelect);
          onFocusChanged();
        }



        void ODF::image_close_slot ()
        {
          QModelIndexList indexes = image_list_view->selectionModel()->selectedIndexes();
          while (indexes.size()) {
            image_list_model->remove_item (indexes.first());
            indexes = image_list_view->selectionModel()->selectedIndexes();
          }
        }



        void ODF::lock_orientation_to_image_slot (int unused) {
          if (lock_orientation_to_image_box->isChecked()) {
            render_frame->makeCurrent();
            render_frame->set_rotation (window.get_current_mode()->projection.modelview());
            window.makeGLcurrent();
          }
        }

        void ODF::colour_by_direction_slot (int unused) 
        { 
          render_frame->set_color_by_dir (colour_by_direction_box->isChecked()); 
          if (overlay_frame->isChecked())
            window.updateGL();
        }

        void ODF::hide_negative_lobes_slot (int unused) 
        {
          render_frame->set_hide_neg_lobes (hide_negative_lobes_box->isChecked()); 
          if (overlay_frame->isChecked())
            window.updateGL();
        }

        void ODF::use_lighting_slot (int unused) 
        { 
          render_frame->set_use_lighting (use_lighting_box->isChecked()); 
          if (overlay_frame->isChecked())
            window.updateGL();
        }

        void ODF::interpolation_slot (int unused) 
        { 
          onFocusChanged();
          if (overlay_frame->isChecked())
            window.updateGL();
        }


        void ODF::show_axes_slot (int unused) {
          render_frame->set_show_axes (show_axes_box->isChecked()); 
        }

        void ODF::level_of_detail_slot (int value) { render_frame->set_LOD (level_of_detail_selector->value()); }

        void ODF::lmax_slot (int value) 
        { 
          render_frame->set_lmax (lmax_selector->value()); 
          if (overlay_frame->isChecked())
            window.updateGL();
        }

        void ODF::update_slot (int unused) {
          window.updateGL();
        }



        void ODF::overlay_toggled_slot () { window.updateGL(); }


        void ODF::overlay_scale_slot () 
        { 
          overlay_scale->setRate (0.01 * overlay_scale->value());
          if (overlay_frame->isChecked())
            window.updateGL();
        }

        void ODF::overlay_update_slot (int value) 
        {
          if (overlay_frame->isChecked()) 
            window.updateGL();
        }

        void ODF::selection_changed_slot (const QItemSelection &, const QItemSelection &)
        {
          onFocusChanged();
          if (overlay_frame->isChecked())
            window.updateGL();
        }


      }
    }
  }
}





