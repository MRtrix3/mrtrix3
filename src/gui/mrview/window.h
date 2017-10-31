/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __gui_mrview_window_h__
#define __gui_mrview_window_h__

#include "image.h"
#include "memory.h"
#include "math/versor.h"
#include "gui/cursor.h"
#include "gui/gui.h"
#include "gui/mrview/gui_image.h"
#include "gui/opengl/font.h"
#include "gui/mrview/colourmap.h"
#include "gui/mrview/colourmap_button.h"


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
        class ODF;
        class CameraInteractor;
      }


      class Window : public QMainWindow, ColourMapButtonObserver
      { MEMALIGN(Window)
          Q_OBJECT

        private:
          Cursor cursors_do_not_use;

          class GLArea : public GL::Area { MEMALIGN(GLArea)
            public:
              GLArea (Window& parent);
              QSize sizeHint () const override;

            protected:
              void dragEnterEvent (QDragEnterEvent* event) override;
              void dragMoveEvent (QDragMoveEvent* event) override;
              void dragLeaveEvent (QDragLeaveEvent* event) override;
              void dropEvent (QDropEvent* event) override;
              bool event (QEvent* event) override;
            private:
              void initializeGL () override;
              void paintGL () override;
              void mousePressEvent (QMouseEvent* event) override;
              void mouseMoveEvent (QMouseEvent* event) override;
              void mouseReleaseEvent (QMouseEvent* event) override;
              void wheelEvent (QWheelEvent* event) override;
          };
          GLArea* glarea;

        public:
          Window();
          ~Window();

          void parse_arguments ();
          void add_images (vector<std::unique_ptr<MR::Header>>& list);

          const QPoint& mouse_position () const { return mouse_position_; }
          const QPoint& mouse_displacement () const { return mouse_displacement_; }
          Qt::MouseButtons mouse_buttons () const { return buttons_; }
          Qt::KeyboardModifiers modifiers () const { return modifiers_; }

          void selected_colourmap(size_t colourmap, const ColourMapButton&) override;
          void selected_custom_colour(const QColor&colour, const ColourMapButton&) override;

          const Image* image () const {
            return static_cast<const Image*> (image_group->checkedAction());
          }
          QActionGroup* tools () const {
            return tool_group;
          }

          int slice () const {
            if (!image())
              return -1;
            else
              return std::round ((image()->transform().scanner2voxel.cast<float>() * focus())[anatomical_plane]);
          }

          Mode::Base* get_current_mode () const { return mode.get(); }
          const Eigen::Vector3f& focus () const { return focal_point; }
          const Eigen::Vector3f& target () const { return camera_target; }
          float FOV () const { return field_of_view; }
          int plane () const { return anatomical_plane; }
          const Math::Versorf& orientation () const { return orient; }
          bool snap_to_image () const { return snap_to_image_axes_and_voxel; }
          Image* image () { return static_cast<Image*> (image_group->checkedAction()); }

          void set_focus (const Eigen::Vector3f& p) { focal_point = p; emit focusChanged(); }
          void set_target (const Eigen::Vector3f& p) { camera_target = p; emit targetChanged(); }
          void set_FOV (float value) { field_of_view = value; emit fieldOfViewChanged(); }
          void set_plane (int p) { anatomical_plane = p; emit planeChanged(); }
          void set_orientation (const Math::Versorf& V) { orient = V; emit orientationChanged(); }
          void set_scaling (float min, float max) { if (!image()) return; image()->set_windowing (min, max); }
          void set_snap_to_image (bool onoff) { snap_to_image_axes_and_voxel = onoff; snap_to_image_action->setChecked(onoff);  emit focusChanged(); }

          void set_scaling_all (float min, float max) {
            QList<QAction*> list = image_group->actions();
            for (int n = 0; n < list.size(); ++n)
              static_cast<Image*> (list[n])->set_windowing (min, max);
          }

          void set_image_volume (size_t axis, ssize_t index)
          {
            assert (image());
            image()->image.index (axis) = index;
            set_image_navigation_menu();
            updateGL();
          }

          bool get_image_visibility () const { return ! image_hide_action->isChecked(); }
          void set_image_visibility (bool flag);

          bool show_crosshairs () const { return show_crosshairs_action->isChecked(); }
          bool show_comments () const { return show_comments_action->isChecked(); }
          bool show_voxel_info () const { return show_voxel_info_action->isChecked(); }
          bool show_orientation_labels () const { return show_orientation_labels_action->isChecked(); }
          bool show_colourbar () const { return show_colourbar_action->isChecked(); }

          void captureGL (std::string filename) {
            QImage image (glarea->grabFramebuffer());
            image.save (filename.c_str());
          }

          GL::Area* glwidget () const { return glarea; }
          GL::Lighting& lighting () { return *lighting_; }
          ColourMap::Renderer colourbar_renderer;

          void register_camera_interactor (Tool::CameraInteractor* agent = nullptr);
          Tool::CameraInteractor* active_camera_interactor () { return camera_interactor; }

          static void add_commandline_options (MR::App::OptionList& options);
          static Window* main;

        signals:
          void focusChanged ();
          void targetChanged ();
          void sliceChanged ();
          void planeChanged ();
          void orientationChanged ();
          void fieldOfViewChanged ();
          void modeChanged ();
          void imageChanged ();
          void imageVisibilityChanged (bool);
          void scalingChanged ();
          void volumeChanged (size_t);
          void volumeGroupChanged (size_t);

        public slots:
          void on_scaling_changed ();
          void updateGL ();
          void drawGL ();

        private slots:
          void image_open_slot ();
          void image_import_DICOM_slot ();
          void image_save_slot ();
          void image_close_slot ();
          void image_properties_slot ();

          void select_mode_slot (QAction* action);
          void select_mouse_mode_slot (QAction* action);
          void select_tool_slot (QAction* action);
          void select_plane_slot (QAction* action);
          void zoom_in_slot ();
          void zoom_out_slot ();
          void invert_scaling_slot ();
          void full_screen_slot ();
          void toggle_annotations_slot ();
          void snap_to_image_slot ();

          void hide_image_slot ();
          void slice_next_slot ();
          void slice_previous_slot ();
          void image_next_slot ();
          void image_previous_slot ();
          void image_next_volume_slot ();
          void image_previous_volume_slot ();
          void image_goto_volume_slot ();
          void image_next_volume_group_slot ();
          void image_goto_volume_group_slot ();
          void image_previous_volume_group_slot ();
          void image_reset_slot ();
          void image_interpolate_slot ();
          void image_select_slot (QAction* action);

          void reset_view_slot ();
          void background_colour_slot ();

          void OpenGL_slot ();
          void about_slot ();
          void aboutQt_slot ();

          void process_commandline_option_slot ();


        private:
          QPoint mouse_position_, mouse_displacement_;
          Qt::MouseButtons buttons_;
          Qt::KeyboardModifiers modifiers_;

          enum MouseAction {
            NoAction,
            SetFocus,
            Contrast,
            Pan,
            PanThrough,
            Tilt,
            Rotate
          };

          std::unique_ptr<Mode::Base> mode;
          GL::Lighting* lighting_;
          GL::Font font;

          const Qt::KeyboardModifiers FocusModifier, MoveModifier, RotateModifier;
          MouseAction mouse_action;

          Eigen::Vector3f focal_point, camera_target;
          Math::Versorf orient;
          float field_of_view;
          int anatomical_plane, annotations;
          ColourMap::Position colourbar_position, tools_colourbar_position;
          bool snap_to_image_axes_and_voxel;

          float background_colour[3];

          Tool::CameraInteractor* camera_interactor;

          QMenu *image_menu;

          ColourMapButton *colourmap_button;

          QActionGroup *mode_group,
                       *tool_group,
                       *image_group,
                       *mode_action_group,
                       *plane_group;

          QAction *save_action,
                  *close_action,
                  *properties_action,

                  **tool_actions,
                  *invert_scale_action,
                  *extra_controls_action,
                  *snap_to_image_action,
                  *image_hide_action,
                  *next_image_action,
                  *prev_image_action,
                  *next_image_volume_action,
                  *prev_image_volume_action,
                  *goto_image_volume_action,
                  *next_slice_action,
                  *prev_slice_action,
                  *reset_view_action,
                  *next_image_volume_group_action,
                  *prev_image_volume_group_action,
                  *goto_image_volume_group_action,
                  *image_list_area,

                  *reset_windowing_action,
                  *axial_action,
                  *sagittal_action,
                  *coronal_action,

                  *toggle_annotations_action,
                  *show_comments_action,
                  *show_voxel_info_action,
                  *show_orientation_labels_action,
                  *show_crosshairs_action,
                  *show_colourbar_action,
                  *image_interpolate_action,
                  *full_screen_action,

                  *OpenGL_action,
                  *about_action,
                  *aboutQt_action;

          static ColourMap::Position parse_colourmap_position_str (const std::string& position_str);

          void paintGL ();
          void initGL ();
          void keyPressEvent (QKeyEvent* event) override;
          void keyReleaseEvent (QKeyEvent* event) override;
          void mousePressEventGL (QMouseEvent* event);
          void mouseMoveEventGL (QMouseEvent* event);
          void mouseReleaseEventGL (QMouseEvent* event);
          void wheelEventGL (QWheelEvent* event);
          bool gestureEventGL (QGestureEvent* event);

          int get_mouse_mode ();
          void set_cursor ();
          void set_image_menu ();
          void set_mode_features ();
          void set_image_navigation_menu ();

          void closeEvent (QCloseEvent* event) override;
          void create_tool (QAction* action, bool show);

          void process_commandline_option ();

          template <class Event> void grab_mouse_state (Event* event);
          template <class Event> void update_mouse_state (Event* event);

          Tool::Base* tool_has_focus;

          vector<double> render_times;
          double best_FPS, best_FPS_time;
          bool show_FPS;
          size_t current_option;

          friend class ImageBase;
          friend class Mode::Base;
          friend class Tool::Base;
          friend class Tool::ODF;
          friend class Window::GLArea;
          friend class GrabContext;
      };


      class GrabContext : private Context::Grab { NOMEMALIGN
        public:
          GrabContext () : Context::Grab (Window::main->glarea) { }
      };


#ifndef NDEBUG
# define ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT ASSERT_GL_CONTEXT_IS_CURRENT (::MR::GUI::MRView::Window::main->glwidget())
#else
# define ASSERT_GL_MRVIEW_CONTEXT_IS_CURRENT
#endif


    }
  }
}

#endif
