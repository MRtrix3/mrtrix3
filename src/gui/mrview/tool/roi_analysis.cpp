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
#include "gui/mrview/volume.h"
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

            
       namespace {
         constexpr std::array<std::array<GLubyte,3>,6> preset_colours = {
           255, 255, 0,
           255, 0, 255,
           0, 255, 255,
           255, 0, 0,
           0, 255, 255,
           0, 0, 255
         };
       }


        class ROI::Item : public Volume {
          public:
            template <class InfoType>
              Item (const InfoType& info) : Volume (info) {
                type = gl::UNSIGNED_BYTE;
                format = gl::RED;
                internal_format = gl::R8;
                set_allowed_features (false, true, false);
                set_interpolate (false);
                set_use_transparency (true);
                set_min_max (0.0, 1.0);
                set_windowing (-1.0f, 0.0f);
                alpha = 1.0f;
                colour = preset_colours[current_preset_colour++];
                if (current_preset_colour >= 6)
                  current_preset_colour = 0;
                transparent_intensity = 0.4;
                opaque_intensity = 0.6;
                colourmap = ColourMap::index ("Colour");

                bind();
                allocate();
              }

            void load (const MR::Image::Header& header) {
              bind();
              MR::Image::Buffer<bool> buffer (header);
              auto vox = buffer.voxel();
              std::vector<GLubyte> data (vox.dim(0)*vox.dim(1));
              ProgressBar progress ("loading ROI image \"" + header.name() + "\"...");
              for (auto outer = MR::Image::Loop(2,3) (vox); outer; ++outer) {
                auto p = data.begin();
                for (auto inner = MR::Image::Loop (0,2) (vox); inner; ++inner) 
                  *(p++) = vox.value();
                upload_data ({ 0, 0, vox[2] }, { vox.dim(0), vox.dim(1), 1 }, reinterpret_cast<void*> (&data[0]));
                ++progress;
              }
            }

            Mode::Slice::Shader shader;

            static int current_preset_colour;
        };


        int ROI::Item::current_preset_colour = 0;



        class ROI::Model : public ListModelBase
        {
          public:
            Model (QObject* parent) : 
              ListModelBase (parent) { }

            void load (VecPtr<MR::Image::Header>& list);

            Item* get (QModelIndex& index) {
              return dynamic_cast<Item*>(items[index.row()]);
            }
        };


        void ROI::Model::load (VecPtr<MR::Image::Header>& list)
        {
          beginInsertRows (QModelIndex(), items.size(), items.size()+list.size());
          for (size_t i = 0; i < list.size(); ++i) {
            Item* roi = new Item (*list[i]);
            roi->load (*list[i]);
            items.push_back (roi);
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

            save_button = new QPushButton (this);
            save_button->setToolTip (tr ("Save ROI"));
            save_button->setIcon (QIcon (":/save.svg"));
            save_button->setEnabled (false);
            connect (save_button, SIGNAL (clicked()), this, SLOT (save_slot ()));
            layout->addWidget (save_button, 1);

            close_button = new QPushButton (this);
            close_button->setToolTip (tr ("Close ROI"));
            close_button->setIcon (QIcon (":/close.svg"));
            close_button->setEnabled (false);
            connect (close_button, SIGNAL (clicked()), this, SLOT (close_slot ()));
            layout->addWidget (close_button, 1);

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
            draw_button->setEnabled (false);
            connect (draw_button, SIGNAL (clicked()), this, SLOT (draw_slot ()));
            layout->addWidget (draw_button, 1);

            erase_button = new QPushButton (this);
            erase_button->setToolTip (tr ("Erase"));
            erase_button->setIcon (QIcon (":/erase.svg"));
            erase_button->setCheckable (true);
            erase_button->setEnabled (false);
            connect (erase_button, SIGNAL (clicked()), this, SLOT (erase_slot ()));
            layout->addWidget (erase_button, 1);

            main_box->addLayout (layout, 0);

            colour_button = new QColorButton;
            colour_button->setEnabled (false);
            main_box->addWidget (colour_button, 0);
            connect (colour_button, SIGNAL (clicked()), this, SLOT (colour_changed()));


            opacity_slider = new QSlider (Qt::Horizontal);
            opacity_slider->setRange (1,1000);
            opacity_slider->setSliderPosition (int (1000));
            connect (opacity_slider, SIGNAL (valueChanged (int)), this, SLOT (opacity_changed(int)));
            opacity_slider->setEnabled (false);
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
          //load (list);
        }



        void ROI::open_slot ()
        {
          std::vector<std::string> names = Dialog::File::get_images (this, "Select ROI images to open");
          if (names.empty())
            return;
          VecPtr<MR::Image::Header> list;
          for (size_t n = 0; n < names.size(); ++n)
            list.push_back (new MR::Image::Header (names[n]));

          load (list);
        }




        void ROI::save_slot ()
        {
          TRACE;
        }



        void ROI::load (VecPtr<MR::Image::Header>& list) 
        {
          size_t previous_size = list_model->rowCount();
          list_model->load (list);

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


        void ROI::draw (const Projection& projection, bool is_3D, int, int)
        {
          if (is_3D) return;

          if (!is_3D) {
            // set up OpenGL environment:
            gl::Enable (gl::BLEND);
            gl::Disable (gl::DEPTH_TEST);
            gl::DepthMask (gl::FALSE_);
            gl::ColorMask (gl::TRUE_, gl::TRUE_, gl::TRUE_, gl::TRUE_);
            gl::BlendFunc (gl::SRC_ALPHA, gl::ONE_MINUS_SRC_ALPHA);
            gl::BlendEquation (gl::FUNC_ADD);
          }

          for (int i = 0; i < list_model->rowCount(); ++i) {
            if (list_model->items[i]->show && !hide_all_button->isChecked()) {
              Item* roi = dynamic_cast<Item*>(list_model->items[i]);
              //if (is_3D) 
                //window.get_current_mode()->overlays_for_3D.push_back (image);
              //else
                roi->render (roi->shader, projection, projection.depth_of (window.focus()));
            }
          }

          if (!is_3D) {
            // restore OpenGL environment:
            gl::Disable (gl::BLEND);
            gl::Enable (gl::DEPTH_TEST);
            gl::DepthMask (gl::TRUE_);
          }
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
            Item* roi = dynamic_cast<Item*> (list_model->get (indices[i]));
            QColor c = colour_button->color();
            roi->colour = { GLubyte (c.red()), GLubyte (c.green()), GLubyte (c.blue()) };
          }
          updateGL();
        }




        void ROI::opacity_changed (int)
        {
          QModelIndexList indices = list_view->selectionModel()->selectedIndexes();
          for (int i = 0; i < indices.size(); ++i) {
            Item* roi = dynamic_cast<Item*> (list_model->get (indices[i]));
            roi->alpha = opacity_slider->value() / 1.0e3f;
          }
          window.updateGL();
        }


        void ROI::selection_changed_slot (const QItemSelection &, const QItemSelection &)
        {
          update_selection();
        }



        void ROI::update_selection () 
        {
          QModelIndexList indices = list_view->selectionModel()->selectedIndexes();
          opacity_slider->setEnabled (indices.size());
          save_button->setEnabled (indices.size());
          close_button->setEnabled (indices.size());
          draw_button->setEnabled (indices.size());
          erase_button->setEnabled (indices.size());
          colour_button->setEnabled (indices.size());

          if (!indices.size())
            return;

          float opacity = 0.0f;
          float color[3] = { 0.0f, 0.0f, 0.0f };

          for (int i = 0; i < indices.size(); ++i) {
            Item* roi = dynamic_cast<Item*> (list_model->get (indices[i]));
            opacity += roi->alpha;
            color[0] += roi->colour[0];
            color[1] += roi->colour[1];
            color[2] += roi->colour[2];
          }
          opacity /= indices.size();
          colour_button->setColor (QColor (
                std::round (color[0] / indices.size()),
                std::round (color[1] / indices.size()),
                std::round (color[2] / indices.size()) ));

          opacity_slider->setValue (1.0e3f * opacity);
        }







        bool ROI::process_batch_command (const std::string& cmd, const std::string& args)
        {
          (void)cmd;
          (void)args;
          /*

          // BATCH_COMMAND roi.load path # Loads the specified image on the roi tool.
          if (cmd == "roi.load") {
            VecPtr<MR::Image::Header> list;
            try { list.push_back (new MR::Image::Header (args)); }
            catch (Exception& e) { e.display(); }
            load (list);
            return true;
          }

          // BATCH_COMMAND roi.opacity value # Sets the roi opacity to floating value [0-1].
          else if (cmd == "roi.opacity") {
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




