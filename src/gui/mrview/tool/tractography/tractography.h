/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#ifndef __gui_mrview_tool_tractography_h__
#define __gui_mrview_tool_tractography_h__

#include "gui/mrview/tool/base.h"
#include "gui/projection.h"
#include "gui/mrview/adjust_button.h"

namespace MR
{
  namespace GUI
  {
    namespace GL {
      class Lighting;
    }
    namespace Dialog {
      class Lighting;
    }

    namespace MRView
    {
      namespace Tool
      {
        class Tractography : public Base
        {
            Q_OBJECT

          public:

            class Model;

            Tractography (Window& main_window, Dock* parent);

            virtual ~Tractography ();

            void draw (const Projection& transform, bool is_3D);
            bool crop_to_slab () const { return (do_crop_to_slab && not_3D); }
            bool process_batch_command (const std::string& cmd, const std::string& args);

            QPushButton* hide_all_button;
            float line_thickness;
            bool do_crop_to_slab;
            bool use_lighting;
            bool not_3D;
            float slab_thickness;
            float line_opacity;
            Model* tractogram_list_model;
            QListView* tractogram_list_view;

            GL::Lighting* lighting;

          private slots:
            void tractogram_open_slot ();
            void tractogram_close_slot ();
            void toggle_shown_slot (const QModelIndex&, const QModelIndex&);
            void selection_changed_slot (const QItemSelection &, const QItemSelection &);
            void hide_all_slot ();
            void on_slab_thickness_slot ();
            void on_crop_to_slab_slot (bool is_checked);
            void on_use_lighting_slot (bool is_checked);
            void on_lighting_settings ();
            void opacity_slot (int opacity);
            void line_thickness_slot (int thickness);
            void right_click_menu_slot (const QPoint& pos);
            void colour_track_by_direction_slot ();
            void colour_track_by_ends_slot ();
            void set_track_colour_slot ();
            void randomise_track_colour_slot ();

          protected:
            AdjustButton* slab_entry;
            QMenu* track_option_menu;
            Dialog::Lighting *lighting_dialog;

        };
      }
    }
  }
}

#endif




