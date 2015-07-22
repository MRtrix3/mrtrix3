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

#ifndef __gui_mrview_mode_base_h__
#define __gui_mrview_mode_base_h__

#include "gui/opengl/gl.h"
#include "gui/opengl/transformation.h"
#include "gui/projection.h"
#include "gui/mrview/window.h"
#include "gui/mrview/tool/base.h"

#define ROTATION_INC 0.002
#define MOVE_IN_OUT_FOV_MULTIPLIER 1.0e-3f

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {

      namespace Tool {
        class Dock;
      }

      namespace Mode
      {

        const int FocusContrast = 0x00000001;
        const int MoveTarget = 0x00000002;
        const int TiltRotate = 0x00000004;
        const int MoveSlice = 0x00000008;
        const int ShaderThreshold = 0x10000000;
        const int ShaderTransparency = 0x20000000;
        const int ShaderLighting = 0x40000000;
        const int ShaderClipping = 0x80000008;



        class Slice;
        class Ortho;
        class Volume;
        class LightBox;
        class ModeGuiVisitor
        {
          public:
            virtual void update_base_mode_gui(const Base&) {}
            virtual void update_slice_mode_gui(const Slice&) {}
            virtual void update_ortho_mode_gui(const Ortho&) {}
            virtual void update_volume_mode_gui(const Volume&) {}
            virtual void update_lightbox_mode_gui(const LightBox&) {}
        };



        class Base : public QObject
        {
          public:
            Base (int flags = FocusContrast | MoveTarget);
            virtual ~Base ();

            Window& window () const { return *Window::main; }
            Projection projection;
            const int features;
            QList<ImageBase*> overlays_for_3D;
            bool update_overlays;

            virtual void paint (Projection& projection);
            virtual void mouse_press_event ();
            virtual void mouse_release_event ();
            virtual void reset_event ();
            virtual void slice_move_event (int x);
            virtual void set_focus_event ();
            virtual void contrast_event ();
            virtual void pan_event ();
            virtual void panthrough_event ();
            virtual void tilt_event ();
            virtual void rotate_event ();
            virtual void image_changed_event () {}
            virtual const Projection* get_current_projection() const;

            virtual void request_update_mode_gui(ModeGuiVisitor& visitor) const {
              visitor.update_base_mode_gui(*this); }

            void paintGL ();

            const Image* image () const { return window().image(); }
            const Point<>& focus () const { return window().focus(); }
            const Point<>& target () const { return window().target(); }
            float FOV () const { return window().FOV(); }
            int plane () const { return window().plane(); }
            Math::Versor<float> orientation () const { 
              if (snap_to_image()) 
                return Math::Versor<float>(1.0f, 0.0f, 0.0f, 0.0f);
              return window().orientation(); 
            }

            int width () const { return glarea()->width(); }
            int height () const { return glarea()->height(); }
            bool snap_to_image () const { return window().snap_to_image(); }

            Image* image () { return window().image(); }

            void move_target_to_focus_plane (const Projection& projection) {
              Point<> in_plane_target = projection.model_to_screen (target());
              in_plane_target[2] = projection.depth_of (focus());
              set_target (projection.screen_to_model (in_plane_target));
            }
            void set_visible (bool v) { if(visible != v) { visible = v; updateGL(); } }
            void set_focus (const Point<>& p) { window().set_focus (p); }
            void set_target (const Point<>& p) { window().set_target (p); }
            void set_FOV (float value) { window().set_FOV (value); }
            void set_plane (int p) { window().set_plane (p); }
            void set_orientation (const Math::Versor<float>& Q) { window().set_orientation (Q); }
            void reset_orientation () { 
              Math::Versor<float> orient;
              if (image()) 
                orient.from_matrix (image()->header().transform());
              set_orientation (orient);
            }

            QGLWidget* glarea () const {
              return reinterpret_cast <QGLWidget*> (window().glarea);
            }

            Point<> move_in_out_displacement (float distance, const Projection& projection) const {
              Point<> move (projection.screen_normal());
              move.normalise();
              move *= distance;
              return move;
            }

            void move_in_out (float distance, const Projection& projection) {
              if (!image()) return;
              Point<> move = move_in_out_displacement (distance, projection);
              set_focus (focus() + move);
            }

            void move_in_out_FOV (int increment, const Projection& projection) {
              move_in_out (MOVE_IN_OUT_FOV_MULTIPLIER * increment * FOV(), projection);
            }

            void render_tools (const Projection& projection, bool is_3D = false, int axis = 0, int slice = 0) {
              QList<QAction*> tools = window().tools()->actions();
              for (int i = 0; i < tools.size(); ++i) {
                Tool::Dock* dock = dynamic_cast<Tool::__Action__*>(tools[i])->dock;
                if (dock)
                  dock->tool->draw (projection, is_3D, axis, slice);
              }
            }

            Math::Versor<float> get_tilt_rotation () const;
            Math::Versor<float> get_rotate_rotation () const;

            Point<> voxel_at (const Point<>& pos) const {
              if (!image()) return Point<>();
              return image()->transform().scanner2voxel (pos);
            }

            void draw_crosshairs (const Projection& with_projection) const {
              if (window().show_crosshairs())
                with_projection.render_crosshairs (focus());
            }

            void draw_orientation_labels (const Projection& with_projection) const {
              if (window().show_orientation_labels())
                with_projection.draw_orientation_labels();
            }

            int slice (int axis) const { return std::round (voxel_at (focus())[axis]); }
            int slice () const { return slice (plane()); }

            void updateGL () { window().updateGL(); } 

          protected:

            GL::mat4 adjust_projection_matrix (const GL::mat4& Q, int proj) const;
            GL::mat4 adjust_projection_matrix (const GL::mat4& Q) const { 
              return adjust_projection_matrix (Q, plane()); 
            }

            void reset_view ();

            bool visible;
        };




        //! \cond skip
        class __Action__ : public QAction
        {
          public:
            __Action__ (QActionGroup* parent,
                        const char* const name,
                        const char* const description,
                        int index) :
              QAction (name, parent) {
              setCheckable (true);
              setShortcut (tr (std::string ("F"+str (index)).c_str()));
              setStatusTip (tr (description));
            }

            virtual Base* create() const = 0;
        };
        //! \endcond



        template <class T> class Action : public __Action__
        {
          public:
            Action (QActionGroup* parent,
                    const char* const name,
                    const char* const description,
                    int index) :
              __Action__ (parent, name, description, index) { }

            virtual Base* create() const {
              return new T;
            }
        };


      }


    }
  }
}


#endif


