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


#ifndef __gui_mrview_tool_overlay_h__
#define __gui_mrview_tool_overlay_h__

#include "gui/mrview/mode/base.h"
#include "gui/mrview/tool/base.h"
#include "gui/mrview/adjust_button.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        class Overlay : public Base
        {
            Q_OBJECT

          public:

            Overlay (Window& main_window, Dock* parent);

            void draw (const Projection& projection, bool is_3D);
            bool process_batch_command (const std::string& cmd, const std::string& args);

          private slots:
            void image_open_slot ();
            void image_close_slot ();
            void hide_all_slot ();
            void toggle_shown_slot (const QModelIndex&, const QModelIndex&);
            void selection_changed_slot (const QItemSelection &, const QItemSelection &);
            void update_slot (int unused);
            void values_changed ();
            void colourmap_changed (int index);
            void upper_threshold_changed (int unused);
            void lower_threshold_changed (int unused);
            void upper_threshold_value_changed ();
            void lower_threshold_value_changed ();
            void interpolate_changed ();

          protected:
             class Model;
             QPushButton* hide_all_button;
             Model* image_list_model;
             QListView* image_list_view;
             QComboBox* colourmap_combobox;
             AdjustButton *min_value, *max_value, *lower_threshold, *upper_threshold;
             QCheckBox *lower_threshold_check_box, *upper_threshold_check_box;
             QCheckBox* interpolate_check_box;
             QSlider *opacity;

             void update_selection ();
             void updateGL() { 
               window.get_current_mode()->update_overlays = true;
               window.updateGL();
             }
             
             void add_images (VecPtr<MR::Image::Header>& list); 
        };

      }
    }
  }
}

#endif




