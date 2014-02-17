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




