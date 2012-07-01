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
    namespace GL {
      class Lighting;
    }

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

          const QPoint& mouse_position () const { return mouse_position_; }
          const QPoint& mouse_displacement () const { return mouse_displacement_; }
          Qt::MouseButtons mouse_buttons () const { return buttons_; }
          Qt::KeyboardModifiers modifiers () const { return modifiers_; }

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

          bool show_crosshairs () const { 
            return show_crosshairs_action->isChecked();
          }

          bool show_comments () const { 
            return show_comments_action->isChecked();
          }

          bool show_voxel_info () const { 
            return show_voxel_info_action->isChecked();
          }

          bool show_orientation_labels () const { 
            return show_orientation_labels_action->isChecked();
          }

          void updateGL () {
            glarea->updateGL();
          }

          GL::Lighting& lighting () { return *lighting_; }

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
          void select_mouse_mode_slot (QAction* action);
          void select_tool_slot (QAction* action);
          void select_projection_slot (QAction* action);
          void mode_control_slot ();
          void select_colourmap_slot ();
          void invert_colourmap_slot ();
          void invert_scaling_slot ();
          void full_screen_slot ();
          void toggle_annotations_slot ();

          void slice_next_slot ();
          void slice_previous_slot ();
          void image_next_slot ();
          void image_previous_slot ();
          void image_next_volume_slot ();
          void image_previous_volume_slot ();
          void image_next_volume_group_slot ();
          void image_previous_volume_group_slot ();
          void image_reset_slot ();
          void image_interpolate_slot ();
          void image_select_slot (QAction* action);

          void reset_view_slot ();

          void OpenGL_slot ();
          void about_slot ();
          void aboutQt_slot ();


        private:
          Cursor cursors_do_not_use;
          QPoint mouse_position_, mouse_displacement_;
          Qt::MouseButtons buttons_;
          Qt::KeyboardModifiers modifiers_;


          class GLArea : public QGLWidget {
            public:
              GLArea (Window& parent);
              QSize minimumSizeHint () const;
              QSize sizeHint () const;
            protected:
              void dragEnterEvent (QDragEnterEvent* event);
              void dragMoveEvent (QDragMoveEvent* event);
              void dragLeaveEvent (QDragLeaveEvent* event);
              void dropEvent (QDropEvent* event);
            private:
              Window& main;

              void initializeGL ();
              void paintGL ();
              void mousePressEvent (QMouseEvent* event);
              void mouseMoveEvent (QMouseEvent* event);
              void mouseReleaseEvent (QMouseEvent* event);
              void wheelEvent (QWheelEvent* event);
          };

          enum MouseAction { NoAction, SetFocus, Contrast, Pan, PanThrough, Tilt, Rotate };

          GLArea* glarea;
          Ptr<Mode::Base> mode;
          GL::Lighting* lighting_;

          const Qt::KeyboardModifiers FocusModifier, MoveModifier, RotateModifier;
          MouseAction mouse_action;

          Point<> focal_point, camera_target;
          Math::Quaternion<float> orient;
          float field_of_view;
          int proj, annotations;

          QMenu *image_menu, *colourmap_menu;
          QAction *save_action, *close_action, *properties_action;
          QAction *reset_windowing_action, *toggle_annotations_action;
          QAction **tool_actions, **colourmap_actions, *invert_colourmap_action, *invert_scale_action;
          QAction *next_image_action, *prev_image_action, *next_image_volume_action, *prev_image_volume_action;
          QAction *next_slice_action, *prev_slice_action, *reset_view_action;
          QAction *next_image_volume_group_action, *prev_image_volume_group_action, *image_list_area;
          QAction *axial_action, *sagittal_action, *coronal_action, *extra_controls_action;
          QAction *show_comments_action, *show_voxel_info_action, *show_orientation_labels_action, *show_crosshairs_action;
          QAction *image_interpolate_action, *full_screen_action;
          QAction *OpenGL_action, *about_action, *aboutQt_action;
          QActionGroup *mode_group, *tool_group, *image_group, *colourmap_group, *mode_action_group, *projection_group;

          void paintGL ();
          void initGL ();
          void keyPressEvent (QKeyEvent* event);
          void keyReleaseEvent (QKeyEvent* event);
          void mousePressEventGL (QMouseEvent* event);
          void mouseMoveEventGL (QMouseEvent* event);
          void mouseReleaseEventGL (QMouseEvent* event);
          void wheelEventGL (QWheelEvent* event);

          int get_mouse_mode ();
          void set_cursor ();
          void set_image_menu ();
          void set_mode_actions ();
          void set_image_navigation_menu ();

          template <class Event> void grab_mouse_state (Event* event);
          template <class Event> void update_mouse_state (Event* event);

          friend class Image;
          friend class Mode::Base;
          friend class Window::GLArea;
      };

    }
  }
}

#endif
