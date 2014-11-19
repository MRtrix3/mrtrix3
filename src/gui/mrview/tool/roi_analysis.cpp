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

#include "mrtrix.h"
#include "gui/mrview/window.h"
#include "gui/mrview/tool/roi_analysis.h"
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

        class Item : public Image {
          public:
            Item (const MR::Image::Header& H) : Image (H) { }
            Mode::Slice::Shader slice_shader; 
        };




        class ROI::Model : public ListModelBase
        {
          public:
            Model (QObject* parent) : 
              ListModelBase (parent) { }

            void add_items (VecPtr<MR::Image::Header>& list);

            Item* get_image (QModelIndex& index) {
              return dynamic_cast<Item*>(items[index.row()]);
            }
        };


        void ROI::Model::add_items (VecPtr<MR::Image::Header>& list)
        {
          beginInsertRows (QModelIndex(), items.size(), items.size()+list.size());
          for (size_t i = 0; i < list.size(); ++i) {
            Item* overlay = new Item (*list[i]);
            overlay->set_allowed_features (true, true, false);
            if (!overlay->colourmap) 
              overlay->colourmap = 1;
            overlay->alpha = 1.0f;
            overlay->set_use_transparency (true);
            items.push_back (overlay);
          }
          endInsertRows();
        }


        ROI::ROI (Window& main_window, Dock* parent) :
          Base (main_window, parent) { 
            VBoxLayout* main_box = new VBoxLayout (this);
            HBoxLayout* layout = new HBoxLayout;
            layout->setContentsMargins (0, 0, 0, 0);
            layout->setSpacing (0);

            QPushButton* button = new QPushButton (this);
            button->setToolTip (tr ("New ROI"));
            button->setIcon (QIcon (":/new.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (new_slot ()));
            layout->addWidget (button, 1);

            button = new QPushButton (this);
            button->setToolTip (tr ("Open ROI"));
            button->setIcon (QIcon (":/open.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (open_slot ()));
            layout->addWidget (button, 1);

            button = new QPushButton (this);
            button->setToolTip (tr ("Save ROI"));
            button->setIcon (QIcon (":/save.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (save_slot ()));
            layout->addWidget (button, 1);

            button = new QPushButton (this);
            button->setToolTip (tr ("Close ROI"));
            button->setIcon (QIcon (":/close.svg"));
            connect (button, SIGNAL (clicked()), this, SLOT (close_slot ()));
            layout->addWidget (button, 1);

            hide_all_button = new QPushButton (this);
            hide_all_button->setToolTip (tr ("Hide All"));
            hide_all_button->setIcon (QIcon (":/hide.svg"));
            hide_all_button->setCheckable (true);
            connect (hide_all_button, SIGNAL (clicked()), this, SLOT (hide_all_slot ()));
            layout->addWidget (hide_all_button, 1);

            main_box->addLayout (layout, 0);

            list_view = new QListView (this);
            list_view->setSelectionMode (QAbstractItemView::ExtendedSelection);
            list_view->setDragEnabled (true);
            list_view->viewport()->setAcceptDrops (true);
            list_view->setDropIndicatorShown (true);

            list_model = new Model (this);
            list_view->setModel (list_model);

            main_box->addWidget (list_view, 1);

            layout = new HBoxLayout;
            layout->setContentsMargins (0, 0, 0, 0);
            layout->setSpacing (0);

            draw_button = new QPushButton (this);
            draw_button->setToolTip (tr ("Draw"));
            draw_button->setIcon (QIcon (":/draw.svg"));
            draw_button->setCheckable (true);
            connect (draw_button, SIGNAL (clicked()), this, SLOT (draw_slot ()));
            layout->addWidget (draw_button, 1);

            erase_button = new QPushButton (this);
            erase_button->setToolTip (tr ("Erase"));
            erase_button->setIcon (QIcon (":/erase.svg"));
            erase_button->setCheckable (true);
            connect (erase_button, SIGNAL (clicked()), this, SLOT (erase_slot ()));
            layout->addWidget (erase_button, 1);

            main_box->addLayout (layout, 0);

            colour_button = new QColorButton;
            main_box->addWidget (colour_button, 0);
            connect (colour_button, SIGNAL (clicked()), this, SLOT (colour_changed()));


            opacity_slider = new QSlider (Qt::Horizontal);
            opacity_slider->setRange (1,1000);
            opacity_slider->setSliderPosition (int (1000));
            connect (opacity_slider, SIGNAL (valueChanged (int)), this, SLOT (opacity_changed(int)));
            main_box->addWidget (new QLabel ("opacity"), 0);
            main_box->addWidget (opacity_slider, 0);

            connect (list_view->selectionModel(),
                SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
                SLOT (selection_changed_slot(const QItemSelection &, const QItemSelection &)) );

            connect (list_model, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
                     this, SLOT (toggle_shown_slot (const QModelIndex&, const QModelIndex&)));

            update_selection();
          }


        void ROI::new_slot ()
        {
          TRACE;
          //add_images (list);
        }



        void ROI::open_slot ()
        {
          std::vector<std::string> overlay_names = Dialog::File::get_images (this, "Select overlay images to open");
          if (overlay_names.empty())
            return;
          VecPtr<MR::Image::Header> list;
          for (size_t n = 0; n < overlay_names.size(); ++n)
            list.push_back (new MR::Image::Header (overlay_names[n]));

          add_images (list);
        }




        void ROI::save_slot ()
        {
          TRACE;
        }



        void ROI::add_images (VecPtr<MR::Image::Header>& list) 
        {
          size_t previous_size = list_model->rowCount();
          list_model->add_items (list);

          QModelIndex first = list_model->index (previous_size, 0, QModelIndex());
          QModelIndex last = list_model->index (list_model->rowCount()-1, 0, QModelIndex());
          list_view->selectionModel()->select (QItemSelection (first, last), QItemSelectionModel::Select);
        }



        void ROI::close_slot ()
        {
          QModelIndexList indexes = list_view->selectionModel()->selectedIndexes();
          while (indexes.size()) {
            list_model->remove_item (indexes.first());
            indexes = list_view->selectionModel()->selectedIndexes();
          }
          updateGL();
        }


        void ROI::draw_slot () 
        {
          TRACE;
          updateGL();
        }

        void ROI::erase_slot () 
        {
          TRACE;
          updateGL();
        }

        void ROI::hide_all_slot () 
        {
          updateGL();
        }


        void ROI::draw (const Projection& projection, bool is_3D)
        {
          /*

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
          for (int i = 0; i < list_model->rowCount(); ++i) {
            if (list_model->items[i]->show && !hide_all_button->isChecked()) {
              Item* image = dynamic_cast<Item*>(list_model->items[i]);
              need_to_update |= !std::isfinite (image->intensity_min());
              image->transparent_intensity = image->opaque_intensity = image->intensity_min();
              if (is_3D) 
                window.get_current_mode()->overlays_for_3D.push_back (image);
              else
                image->render3D (image->slice_shader, projection, projection.depth_of (window.focus()));
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
          */
        }





        void ROI::toggle_shown_slot (const QModelIndex& index, const QModelIndex& index2) {
          if (index.row() == index2.row()) {
            list_view->setCurrentIndex(index);
          } else {
            for (size_t i = 0; i < list_model->items.size(); ++i) {
              if (list_model->items[i]->show) {
                list_view->setCurrentIndex (list_model->index (i, 0));
                break;
              }
            }
          }
          updateGL();
        }


        void ROI::update_slot (int) {
          updateGL();
        }

        void ROI::colour_changed () 
        {
          QModelIndexList indices = list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            Image* overlay = dynamic_cast<Image*> (list_model->get_image (indices[i]));
            //overlay->set_colourmap (index);
          }
          updateGL();
        }




        void ROI::opacity_changed (int)
        {
          QModelIndexList indices = list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            Image* overlay = dynamic_cast<Image*> (list_model->get_image (indices[i]));
            overlay->alpha = opacity_slider->value() / 1.0e3f;
          }
          window.updateGL();
        }


        void ROI::selection_changed_slot (const QItemSelection &, const QItemSelection &)
        {
          update_selection();
        }



        void ROI::update_selection () 
        {
          /*
          QModelIndexList indices = list_view->selectionModel()->selectedIndexes();
          //colourmap_combobox->setEnabled (indices.size());
          opacity_slider->setEnabled (indices.size());

          if (!indices.size())
            return;

          float opacity = 0.0f;

          for (int i = 0; i < indices.size(); ++i) {
            Image* overlay = dynamic_cast<Image*> (list_model->get_image (indices[i]));
            opacity += overlay->alpha;
          }
          opacity /= indices.size();

          //colourmap_combobox->setCurrentIndex (colourmap_index);
          opacity_slider->setValue (1.0e3f * opacity);

          */
        }







        bool ROI::process_batch_command (const std::string& cmd, const std::string& args)
        {
          /*

          // BATCH_COMMAND overlay.load path # Loads the specified image on the overlay tool.
          if (cmd == "overlay.load") {
            VecPtr<MR::Image::Header> list;
            try { list.push_back (new MR::Image::Header (args)); }
            catch (Exception& e) { e.display(); }
            add_images (list);
            return true;
          }

          // BATCH_COMMAND overlay.opacity value # Sets the overlay opacity to floating value [0-1].
          else if (cmd == "overlay.opacity") {
            try {
              float n = to<float> (args);
              opacity_slider->setSliderPosition(int(1.e3f*n));
            }
            catch (Exception& e) { e.display(); }
            return true;
          }

          */
          return false;
        }



      }
    }
  }
}




