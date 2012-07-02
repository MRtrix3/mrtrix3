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

#include <QAction>
#include <QCursor>
#include <QMouseEvent>
#include <QMenu>

#include "gui/mrview/window.h"
#include "gui/mrview/transform.h"

#define EDGE_WIDTH 6

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {

      namespace Mode
      {

        const int FocusContrast = 0x00000001;
        const int MoveTarget = 0x00000002;
        const int TiltRotate = 0x00000004;
        const int MoveSlice = 0x00000008;
        const int ExtraControls = 0x00000010;

        class Base : public QObject
        {
          public:
            Base (Window& parent, int flags = FocusContrast | MoveTarget);
            virtual ~Base ();

            Window& window;
            Transform transform;
            const int mouse_actions;

            virtual void paint ();
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

            void paintGL ();

            const Image* image () const { 
              return window.image(); 
            }
            const Point<>& focus () const {
              return window.focus(); 
            }
            const Point<>& target () const {
              return window.target(); 
            }
            float FOV () const { 
              return window.FOV(); 
            }
            int projection () const { 
              return window.projection(); 
            }
            const Math::Quaternion<float>& orientation () const {
              return window.orientation(); 
            }

            Image* image () {
              return window.image(); 
            }
            void set_focus (const Point<>& p) {
              window.set_focus (p); 
            }
            void set_target (const Point<>& p) {
              window.set_target (p); 
            }
            void set_FOV (float value) {
              window.set_FOV (value); 
            }
            void set_projection (int p) {
              window.set_projection (p); 
            }
            void set_orientation (const Math::Quaternion<float>& Q) {
              window.set_orientation (Q); 
            }

            void change_FOV_fine (float factor) {
              window.set_FOV (FOV() * Math::exp (0.005*factor));
            }
            void change_FOV_scroll (float factor) {
              change_FOV_fine (20.0 * factor);
            }

            QGLWidget* glarea () const {
              return reinterpret_cast <QGLWidget*> (window.glarea);
            }

            Point<> move_in_out_displacement (float distance, const Transform& with_transform) {
              Point<> move (-with_transform.screen_normal());
              move.normalise();
              move *= distance;
              return move;
            }

            void move_in_out (float distance, const Transform& with_transform) {
              if (!image()) return;
              Point<> move = move_in_out_displacement (distance, with_transform);
              set_target (target() + move);
              set_focus (focus() + move);
            }
            void move_in_out_FOV (int increment, const Transform& with_transform) {
              move_in_out (1e-3 * increment * FOV(), with_transform);
            }

            void move_in_out (float distance) {
              move_in_out (distance, transform);
            }
            void move_in_out_FOV (int increment) {
              move_in_out (1e-3 * increment * FOV());
            }


            bool in_paint () const {
              return painting;
            }

            void updateGL () {
              window.updateGL();
            }


          protected:
            void adjust_projection_matrix (float* M, const float* Q, int proj) const;
            void adjust_projection_matrix (float* M, const float* Q) const { 
              adjust_projection_matrix (M, Q, projection()); 
            }

            bool painting;
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

            virtual Base* create (Window& parent) const = 0;
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

            virtual Base* create (Window& parent) const {
              return new T (parent);
            }
        };


      }


    }
  }
}


#endif


