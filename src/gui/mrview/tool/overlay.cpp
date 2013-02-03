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
#include <QListView>
#include <QStringListModel>

#include "mrtrix.h"
#include "gui/mrview/window.h"
#include "gui/mrview/tool/overlay.h"
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


        class Overlay::Model : public ListModelBase
        {
          public:
            Model (QObject* parent) : 
              ListModelBase (parent) { }

            void add_items (VecPtr<MR::Image::Header>& list);

            Image* get_image (QModelIndex& index) {
              return dynamic_cast<Image*>(items[index.row()]);
            }
        };


        void Overlay::Model::add_items (VecPtr<MR::Image::Header>& list)
        {
          beginInsertRows (QModelIndex(), items.size(), items.size()+list.size());
          for (size_t i = 0; i < list.size(); ++i) {
            items.push_back (new Image (*list[i]));
            //setData (createIndex (images.size()-1,0), Qt::Checked, Qt::CheckStateRole);
          }
          shown.resize (items.size(), true);
          endInsertRows();
        }




        Overlay::Overlay (Window& main_window, Dock* parent) :
          Base (main_window, parent) { 
            QVBoxLayout* main_box = new QVBoxLayout (this);
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

            image_list_view = new QListView(this);
            image_list_view->setSelectionMode (QAbstractItemView::MultiSelection);
            image_list_view->setDragEnabled (true);
            image_list_view->viewport()->setAcceptDrops (true);
            image_list_view->setDropIndicatorShown (true);

            image_list_model = new Model (this);
            image_list_view->setModel (image_list_model);

            main_box->addWidget (image_list_view, 1);

            QGridLayout* default_opt_grid = new QGridLayout;

            default_opt_grid->addWidget (new QLabel ("max value"), 0, 0);
            max_value = new AdjustButton (this, 0.1);
            max_value->setValue (1.0f);
            //max_value->setMin (0.0f);
            connect (max_value, SIGNAL (valueChanged()), this, SLOT (update_slot()));
            default_opt_grid->addWidget (max_value, 0, 1);

            default_opt_grid->addWidget (new QLabel ("min value"), 1, 0);
            min_value = new AdjustButton (this, 0.1);
            min_value->setValue (0.0f);
            //min_value->setMin (0.0f);
            connect (min_value, SIGNAL (valueChanged()), this, SLOT (update_slot()));
            default_opt_grid->addWidget (min_value, 1, 1);

            opacity = new QSlider (Qt::Horizontal);
            opacity->setRange (1,1000);
            opacity->setSliderPosition (int (1000));
            connect (opacity, SIGNAL (valueChanged (int)), this, SLOT (update_slot (int)));
            default_opt_grid->addWidget (new QLabel ("opacity"), 2, 0);
            default_opt_grid->addWidget (opacity, 2, 1);

            main_box->addLayout (default_opt_grid, 0);
          }


        void Overlay::image_open_slot ()
        {
          Dialog::File dialog (this, "Select overlay images to open", true, true);
          if (dialog.exec()) {
            VecPtr<MR::Image::Header> list;
            dialog.get_images (list);
            image_list_model->add_items (list);
          }
        }



        void Overlay::image_close_slot ()
        {
          QModelIndexList indexes = image_list_view->selectionModel()->selectedIndexes();
          while (indexes.size()) {
            image_list_model->remove_item (indexes.first());
            indexes = image_list_view->selectionModel()->selectedIndexes();
          }
        }




        void Overlay::draw2D (const Projection& projection) 
        {
          float overlay_opacity = Math::pow2(static_cast<float>(opacity->value())) / 1.0e6f;

          // set up OpenGL environment:
          glEnable (GL_BLEND);
          glEnable (GL_TEXTURE_3D);
          glDisable (GL_DEPTH_TEST);
          glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
          glDepthMask (GL_FALSE);
          glColorMask (GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
          glBlendFunc (GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
          glBlendEquation (GL_FUNC_ADD);
          glBlendColor (1.0f, 1.0f, 1.0f, overlay_opacity);

          for (int i = 0; i < image_list_model->rowCount(); ++i) {
            if (image_list_model->shown[i]) {
              Image* image = dynamic_cast<Image*>(image_list_model->items[i]);
              image->set_windowing (min_value->value(), max_value->value());
              image->render3D (projection, projection.depth_of (window.focus()));
            }
          }

          DEBUG_OPENGL;

          glDisable (GL_TEXTURE_3D);
        }




        void Overlay::draw3D (const Projection& transform)
        { 
          TEST;
        }


        void Overlay::update_slot () {
          window.updateGL();
        }

        void Overlay::update_slot (int unused) {
          window.updateGL();
        }


      }
    }
  }
}





