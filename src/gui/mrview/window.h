#ifndef __gui_mrview_window_h__
#define __gui_mrview_window_h__

#include <QMainWindow>

#ifdef Complex
# undef Complex
#endif

#include "ptr.h"
#include "math/quaternion.h"
#include "gui/cursor.h"
#include "gui/mrview/image.h"

class QMenu;
class QAction;
class QActionGroup;

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {

      namespace Mode
      {
        class Base;
        class __Entry__;
      }
      namespace Tool
      {
        class Base;
      }






      class Window : public QMainWindow
      {
          Q_OBJECT

        public:
          Window();
          ~Window();

          void add_images (VecPtr<MR::Image::Header>& list);

          static const App::OptionGroup options;


          const Image* image () const { 
            return static_cast<const Image*> (image_group->checkedAction());
          }
          const Point<>& focus () const {
            return focal_point; 
          }
          const Point<>& target () const {
            return camera_target; 
          }
          float FOV () const {
            return field_of_view; 
          }
          int projection () const {
            return proj; 
          }
          const Math::Quaternion<float>& orientation () const {
            return orient; 
          }

          Image* image () {
            return static_cast<Image*> (image_group->checkedAction());
          }
          void set_focus (const Point<>& p) {
            focal_point = p; emit focusChanged(); 
          }
          void set_target (const Point<>& p) {
            camera_target = p; emit targetChanged(); 
          }
          void set_FOV (float value) {
            field_of_view = value; emit fieldOfViewChanged(); 
          }
          void set_projection (int p) {
            proj = p; emit projectionChanged(); 
          }
          void set_orientation (const Math::Quaternion<float>& Q) {
            orient = Q; emit orientationChanged(); 
          }

          void set_scaling (float min, float max) 
          {
            if (!image()) return;
            image()->set_windowing (min, max);
          }
          void scaling_updated () {
            emit scalingChanged();
          }

          void set_scaling_all (float min, float max) 
          {
            QList<QAction*> list = image_group->actions();
            for (int n = 0; n < list.size(); ++n) 
              static_cast<const Image*> (list[n])->set_windowing (min, max);
          }

          void updateGL ();

        signals: 
          void focusChanged ();
          void targetChanged ();
          void sliceChanged ();
          void projectionChanged ();
          void orientationChanged ();
          void fieldOfViewChanged ();
          void modeChanged ();
          void imageChanged ();
          void scalingChanged ();

        private slots:
          void image_open_slot ();
          void image_save_slot ();
          void image_close_slot ();
          void image_properties_slot ();

          void select_mode_slot (QAction* action);
          void select_tool_slot (QAction* action);
          void select_colourmap_slot ();
          void invert_colourmap_slot ();
          void invert_scaling_slot ();
          void full_screen_slot ();

          void image_next_slot ();
          void image_previous_slot ();
          void image_next_volume_slot ();
          void image_previous_volume_slot ();
          void image_next_volume_group_slot ();
          void image_previous_volume_group_slot ();
          void image_reset_slot ();
          void image_interpolate_slot ();
          void image_select_slot (QAction* action);

          void OpenGL_slot ();
          void about_slot ();
          void aboutQt_slot ();


        private:
          Cursor cursors_do_not_use;

          class GLArea;

          GLArea* glarea;
          Ptr<Mode::Base> mode;

          Point<> focal_point, camera_target;
          Math::Quaternion<float> orient;
          float field_of_view;
          int proj;

          QMenu *file_menu, *view_menu, *tool_menu, *image_menu, *help_menu, *colourmap_menu;
          QAction *open_action, *save_action, *close_action, *properties_action, *quit_action;
          QAction *view_menu_mode_area, *view_menu_mode_common_area, *reset_windowing_action;
          QAction *image_interpolate_action, *full_screen_action;
          QAction **tool_actions, **colourmap_actions, *invert_colourmap_action, *invert_scale_action;
          QAction *next_image_action, *prev_image_action, *next_image_volume_action, *prev_image_volume_action;
          QAction *next_image_volume_group_action, *prev_image_volume_group_action, *image_list_area;
          QAction *OpenGL_action, *about_action, *aboutQt_action;
          QActionGroup *mode_group, *tool_group, *image_group, *colourmap_group;

          void paintGL ();
          void initGL ();
          void mousePressEventGL (QMouseEvent* event);
          void mouseMoveEventGL (QMouseEvent* event);
          void mouseDoubleClickEventGL (QMouseEvent* event);
          void mouseReleaseEventGL (QMouseEvent* event);
          void wheelEventGL (QWheelEvent* event);

          void set_image_menu ();
          void set_image_navigation_menu ();

          friend class Image;
          friend class Mode::Base;
          friend class Window::GLArea;
      };

    }
  }
}

#endif
