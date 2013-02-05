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
#include <QCheckBox>
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
            Image* overlay = new Image (*list[i]);
            overlay->set_allowed_features (true, false, false);
            items.push_back (overlay);
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

            image_list_view = new QListView (this);
            image_list_view->setSelectionMode (QAbstractItemView::ExtendedSelection);
            image_list_view->setDragEnabled (true);
            image_list_view->viewport()->setAcceptDrops (true);
            image_list_view->setDropIndicatorShown (true);

            image_list_model = new Model (this);
            image_list_view->setModel (image_list_model);

            main_box->addWidget (image_list_view, 1);

            QGridLayout* main_layout = new QGridLayout;

            main_layout->addWidget (new QLabel ("max value"), 0, 0);
            max_value = new AdjustButton (this, 0.1);
            connect (max_value, SIGNAL (valueChanged()), this, SLOT (values_changed()));
            main_layout->addWidget (max_value, 0, 1);

            main_layout->addWidget (new QLabel ("min value"), 1, 0);
            min_value = new AdjustButton (this, 0.1);
            connect (min_value, SIGNAL (valueChanged()), this, SLOT (values_changed()));
            main_layout->addWidget (min_value, 1, 1);

            threshold_upper_box = new QCheckBox ("upper threshold");
            //threshold_upper_box->setTristate (true);
            connect (threshold_upper_box, SIGNAL (stateChanged(int)), this, SLOT (threshold_upper_changed(int)));
            main_layout->addWidget (threshold_upper_box, 2, 0);
            threshold_upper = new AdjustButton (this, 0.1);
            connect (threshold_upper, SIGNAL (valueChanged()), this, SLOT (threshold_upper_value_changed()));
            main_layout->addWidget (threshold_upper, 2, 1);

            threshold_lower_box = new QCheckBox ("lower threshold");
            //threshold_lower_box->setTristate (true);
            connect (threshold_lower_box, SIGNAL (stateChanged(int)), this, SLOT (threshold_lower_changed(int)));
            main_layout->addWidget (threshold_lower_box, 3, 0);
            threshold_lower = new AdjustButton (this, 0.1);
            connect (threshold_lower, SIGNAL (valueChanged()), this, SLOT (threshold_lower_value_changed()));
            main_layout->addWidget (threshold_lower, 3, 1);

            opacity = new QSlider (Qt::Horizontal);
            opacity->setRange (1,1000);
            opacity->setSliderPosition (int (1000));
            connect (opacity, SIGNAL (valueChanged (int)), this, SLOT (update_slot (int)));
            main_layout->addWidget (new QLabel ("opacity"), 4, 0);
            main_layout->addWidget (opacity, 4, 1);

            main_box->addLayout (main_layout, 0);

            connect (image_list_view->selectionModel(),
                SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
                SLOT (selection_changed_slot(const QItemSelection &, const QItemSelection &)) );

            connect (image_list_view, SIGNAL (clicked (const QModelIndex&)), this, SLOT (toggle_shown_slot (const QModelIndex&)));

            update_selection();
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
          float overlay_opacity = opacity->value() / 1.0e3f;

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


        void Overlay::toggle_shown_slot (const QModelIndex& index) {
          window.updateGL();
        }


        void Overlay::update_slot (int unused) {
          window.updateGL();
        }


        void Overlay::values_changed ()
        {
          QModelIndexList indices = image_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            Image* overlay = dynamic_cast<Image*> (image_list_model->get_image (indices[i]));
            overlay->set_windowing (min_value->value(), max_value->value());
          }
          window.updateGL();
        }


        void Overlay::threshold_lower_changed (int unused) 
        {
          QModelIndexList indices = image_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            Image* overlay = dynamic_cast<Image*> (image_list_model->get_image (indices[i]));
            overlay->set_use_discard_lower (threshold_lower_box->isChecked());
          }
          threshold_lower->setEnabled (indices.size() && threshold_lower_box->isChecked());
          window.updateGL();
        }


        void Overlay::threshold_upper_changed (int unused)
        {
          QModelIndexList indices = image_list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            Image* overlay = dynamic_cast<Image*> (image_list_model->get_image (indices[i]));
            overlay->set_use_discard_upper (threshold_upper_box->isChecked());
          }
          threshold_upper->setEnabled (indices.size() && threshold_upper_box->isChecked());
          window.updateGL();
        }


        void Overlay::threshold_upper_value_changed ()
        {
          if (threshold_upper_box->isChecked()) {
            QModelIndexList indices = image_list_view->selectionModel()->selectedIndexes();
            for (int i = 0; i < indices.size(); ++i) {
              Image* overlay = dynamic_cast<Image*> (image_list_model->get_image (indices[i]));
              overlay->greaterthan = threshold_upper->value();
            }
          }
          window.updateGL();
        }


        void Overlay::threshold_lower_value_changed ()
        {
          if (threshold_lower_box->isChecked()) {
            QModelIndexList indices = image_list_view->selectionModel()->selectedIndexes();
            for (int i = 0; i < indices.size(); ++i) {
              Image* overlay = dynamic_cast<Image*> (image_list_model->get_image (indices[i]));
              overlay->lessthan = threshold_lower->value();
            }
          }
          window.updateGL();
        }


        void Overlay::selection_changed_slot (const QItemSelection &, const QItemSelection &)
        {
          update_selection();
        }



        void Overlay::update_selection () 
        {
          QModelIndexList indices = image_list_view->selectionModel()->selectedIndexes();
          max_value->setEnabled (indices.size());
          min_value->setEnabled (indices.size());
          threshold_lower_box->setEnabled (indices.size());
          threshold_upper_box->setEnabled (indices.size());
          threshold_lower->setEnabled (indices.size());
          threshold_upper->setEnabled (indices.size());

          if (!indices.size())
            return;

          float rate = 0.0f, min_val = 0.0f, max_val = 0.0f;
          float threshold_lower_val = 0.0f, threshold_upper_val = 0.0f;
          int num_threshold_lower = 0, num_threshold_upper = 0;
          for (int i = 0; i < indices.size(); ++i) {
            Image* overlay = dynamic_cast<Image*> (image_list_model->get_image (indices[i]));
            rate += overlay->scaling_rate();
            min_val += overlay->scaling_min();
            max_val += overlay->scaling_max();
            num_threshold_lower += overlay->use_discard_lower();
            num_threshold_upper += overlay->use_discard_upper();
            if (!finite (overlay->lessthan)) 
              overlay->lessthan = overlay->intensity_min();
            if (!finite (overlay->greaterthan)) 
              overlay->greaterthan = overlay->intensity_max();
            threshold_lower_val += overlay->lessthan;
            threshold_upper_val += overlay->greaterthan;
          }
          rate /= indices.size();
          min_val /= indices.size();
          max_val /= indices.size();
          threshold_lower_val /= indices.size();
          threshold_upper_val /= indices.size();

          min_value->setRate (rate);
          max_value->setRate (rate);
          min_value->setValue (min_val);
          max_value->setValue (max_val);

          threshold_lower_box->setCheckState (num_threshold_lower ? 
              ( num_threshold_lower == indices.size() ? 
                Qt::Checked :
                Qt::PartiallyChecked ) : 
              Qt::Unchecked);
          threshold_lower->setValue (threshold_lower_val);
          threshold_lower->setRate (rate);

          threshold_upper_box->setCheckState (num_threshold_upper ? 
              ( num_threshold_upper == indices.size() ? 
                Qt::Checked :
                Qt::PartiallyChecked ) : 
              Qt::Unchecked);
          threshold_upper->setValue (threshold_upper_val);
          threshold_upper->setRate (rate);
        }



      }
    }
  }
}





