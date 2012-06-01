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
#include <QGLWidget>
#include <QFontMetrics>

#include "gui/mrview/window.h"

#define EDGE_WIDTH 6

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Mode
      {

        class Base : public QObject
        {
            Q_OBJECT

          public:
            Window& window;


            Base (Window& parent);
            virtual ~Base ();

            virtual void paint ();

            virtual bool mouse_click ();
            virtual bool mouse_move ();
            virtual bool mouse_doubleclick ();
            virtual bool mouse_release ();
            virtual bool mouse_wheel (float delta, Qt::Orientation orientation);

            void paintGL ();

            static const int TopEdge = 1;
            static const int BottomEdge = 1<<1;
            static const int LeftEdge = 1<<2;
            static const int RightEdge = 1<<3;


            const QPoint& mouse_pos () const {
              return currentPos;
            }
            QPoint mouse_dpos () const {
              return currentPos - lastPos;
            }
            QPoint mouse_dpos_static () const {
              QCursor::setPos (reinterpret_cast<QWidget*> (window.glarea)->mapToGlobal (initialPos));
              return currentPos - initialPos;
            }
            Qt::MouseButtons mouse_buttons () const {
              return buttons_;
            }
            Qt::KeyboardModifiers mouse_modifiers () const {
              return modifiers_;
            }
            int mouse_edge () const {
              return edge_;
            }

            void add_action (QAction* action) {
              window.view_menu->insertAction (window.view_menu_mode_area, action);
            }
            void add_action_common (QAction* action) {
              window.view_menu->insertAction (window.view_menu_mode_common_area, action);
            }

            Point<> model_to_screen (const Point<>& pos, const GLint* gl_viewport, const GLdouble* gl_modelview, const GLdouble* gl_projection) const {
              double wx, wy, wz;
              gluProject (pos[0], pos[1], pos[2], gl_modelview,
                          gl_projection, gl_viewport, &wx, &wy, &wz);
              return Point<> (wx, wy, wz);
            }

            Point<> model_to_screen (const Point<>& pos) const {
              return model_to_screen (pos, viewport_matrix, modelview_matrix, projection_matrix);
            }

            Point<> model_to_screen_direction (const Point<>& pos, const GLint* gl_viewport, const GLdouble* gl_modelview, const GLdouble* gl_projection) const {
              return model_to_screen (pos, gl_viewport, gl_modelview, gl_projection)  
                - model_to_screen (Point<> (0.0, 0.0, 0.0), gl_viewport, gl_modelview, gl_projection);
            }

            Point<> model_to_screen_direction (const Point<>& pos) const {
              return model_to_screen_direction (pos, viewport_matrix, modelview_matrix, projection_matrix);
            }

            Point<> screen_to_model (const Point<>& pos, const GLint* gl_viewport, const GLdouble* gl_modelview, const GLdouble* gl_projection) const {
              double wx, wy, wz;
              gluUnProject (pos[0], pos[1], pos[2], gl_modelview,
                            gl_projection, gl_viewport, &wx, &wy, &wz);
              return Point<> (wx, wy, wz);
            }

            Point<> screen_to_model (const Point<>& pos) const {
              return screen_to_model (pos, viewport_matrix, modelview_matrix, projection_matrix);
            }

            Point<> screen_to_model (const QPoint& pos, const GLint* gl_viewport, const GLdouble* gl_modelview, const GLdouble* gl_projection) const {
              Point<> f (model_to_screen (focus(), gl_viewport, gl_modelview, gl_projection));
              f[0] = pos.x();
              f[1] = glarea()->height() - pos.y();
              return screen_to_model (f, gl_viewport, gl_modelview, gl_projection);
            }

            Point<> screen_to_model (const QPoint& pos) const {
              return screen_to_model (pos, viewport_matrix, modelview_matrix, projection_matrix);
            }

            Point<> screen_to_model () const {
              return screen_to_model (currentPos);
            }

            Point<> screen_to_model_direction (const Point<>& pos, const GLint* gl_viewport, const GLdouble* gl_modelview, const GLdouble* gl_projection) const {
              return screen_to_model (pos, gl_viewport, gl_modelview, gl_projection) - screen_to_model (Point<> (0.0, 0.0, 0.0), gl_viewport, gl_modelview, gl_projection);
            }

            Point<> screen_to_model_direction (const Point<>& pos) const {
              return screen_to_model (pos) - screen_to_model (Point<> (0.0, 0.0, 0.0));
            }

            Point<> screen_to_model_direction (const QPoint& pos, const GLint* gl_viewport, const GLdouble* gl_modelview, const GLdouble* gl_projection) const {
              return screen_to_model (Point<> (pos.x(), -pos.y(), 0.0), gl_viewport, gl_modelview, gl_projection) - screen_to_model (Point<> (0.0, 0.0, 0.0), gl_viewport, gl_modelview, gl_projection);
            }

            Point<> screen_to_model_direction (const QPoint& pos) const {
              return screen_to_model (Point<> (pos.x(), -pos.y(), 0.0)) - screen_to_model (Point<> (0.0, 0.0, 0.0));
            }



            const Image* image () const {
              return window.current_image();
            }
            Image* image () {
              return window.current_image();
            }

            const Math::Quaternion<float>& orientation () const {
              return window.orient;
            }
            float FOV () const {
              return window.field_of_view;
            }
            const Point<>& focus () const {
              return window.focal_point;
            }
            const Point<>& target () const {
              return window.camera_target;
            }
            int projection () const {
              return window.proj;
            }

            void set_focus (const Point<>& p) {
              window.focal_point = p;
            }
            void set_target (const Point<>& p) {
              window.camera_target = p;
            }
            void set_projection (int p) {
              window.proj = p;
            }
            void set_orientation (const Math::Quaternion<float>& Q) {
              window.orient = Q;
            }
            void set_FOV (float value) {
              window.field_of_view = value;
            }
            void change_FOV_fine (float factor) {
              window.field_of_view *= Math::exp (0.005*factor);
            }
            void change_FOV_scroll (float factor) {
              change_FOV_fine (20.0 * factor);
            }

            int width () const {
              return viewport_matrix[2];
            }
            int height () const {
              return viewport_matrix[3];
            }
            QGLWidget* glarea () const {
              return reinterpret_cast <QGLWidget*> (window.glarea);
            }

            void renderText (int x, int y, const std::string& text) {
              glarea()->renderText (x+viewport_matrix[0], glarea()->height()-y-viewport_matrix[1], text.c_str(), font_);
            }

            void renderTextInset (int x, int y, const std::string& text, int inset = -1) {
              QFontMetrics fm (font_);
              QString s (text.c_str());
              if (inset < 0) inset = fm.height() / 2;
              if (x < inset) x = inset;
              if (x + fm.width (s) + inset > width()) x = width() - fm.width (s) - inset;
              if (y < inset) y = inset;
              if (y + fm.height() + inset > height()) y = height() - fm.height() / 2 - inset;
              renderText (x, y, text);
            }

            void renderText (const std::string& text, int position, int line = 0) {
              QFontMetrics fm (font_);
              QString s (text.c_str());
              int x, y;

              if (position & RightEdge) x = width() - fm.height() / 2 - fm.width (s);
              else if (position & LeftEdge) x = fm.height() / 2;
              else x = (width() - fm.width (s)) / 2;

              if (position & TopEdge) y = height() - fm.height() - line * fm.lineSpacing();
              else if (position & BottomEdge) y = fm.height() / 2 + line * fm.lineSpacing();
              else y = (height() + fm.height()) / 2 - line * fm.lineSpacing();

              renderText (x, y, text);
            }

            void move_in_out (float distance);
            void move_in_out_FOV (int increment) {
              move_in_out (1e-3 * increment * FOV());
            }


            bool in_paint () const {
              return painting;
            }


          public slots:
            void updateGL ();
            virtual void reset ();
            virtual void toggle_show_xyz ();

          protected:
            QAction* reset_action, *show_focus_action;
            QAction* show_image_info_action, *show_position_action, *show_orientation_action;

            void update_modelview_projection_viewport () const {
              glGetIntegerv (GL_VIEWPORT, viewport_matrix);
              glGetDoublev (GL_MODELVIEW_MATRIX, modelview_matrix);
              glGetDoublev (GL_PROJECTION_MATRIX, projection_matrix);
            }

            void adjust_projection_matrix (float* M, const float* Q, int proj) const;
            void adjust_projection_matrix (float* M, const float* Q) const { 
              adjust_projection_matrix (M, Q, projection()); 
            }

            void draw_focus () const;

            mutable GLdouble modelview_matrix[16], projection_matrix[16];
            mutable GLint viewport_matrix[4];

            QPoint currentPos, lastPos, initialPos;
            Qt::MouseButtons buttons_;
            Qt::KeyboardModifiers modifiers_;
            int edge_;
            bool painting;

            QFont font_;

            void mousePressEvent (QMouseEvent* event) {
              if (buttons_ != Qt::NoButton) return;
              buttons_ = event->buttons();
              modifiers_ = event->modifiers();
              lastPos = currentPos = initialPos = event->pos();
              if (mouse_click()) event->accept();
              else event->ignore();
            }

            void mouseMoveEvent (QMouseEvent* event) {
              lastPos = currentPos;
              currentPos = event->pos();
              if (buttons_ == Qt::NoButton)
                edge_ =
                  (EDGE_WIDTH*currentPos.x() < width() ? LeftEdge : 0) |
                  (EDGE_WIDTH* (width()-currentPos.x()) < width() ? RightEdge : 0) |
                  (EDGE_WIDTH*currentPos.y() < height() ? TopEdge : 0) |
                  (EDGE_WIDTH* (height()-currentPos.y()) < height() ? BottomEdge : 0);
              if (mouse_move()) event->accept();
              else event->ignore();
            }


            void mouseDoubleClickEvent (QMouseEvent* event) {
              if (mouse_doubleclick())
                event->accept();
              else
                event->ignore();
            }

            void mouseReleaseEvent (QMouseEvent* event) {
              if (event->buttons() != Qt::NoButton) return;
              if (mouse_release())
                event->accept();
              else
                event->ignore();

              buttons_ = Qt::NoButton;
              modifiers_ = Qt::NoModifier;
            }

            void wheelEvent (QWheelEvent* event) {
              buttons_ = event->buttons();
              modifiers_ = event->modifiers();
              lastPos = currentPos = event->pos();
              if (mouse_wheel (event->delta() / 120.0, event->orientation())) 
                event->accept();
              else 
                event->ignore();
            }

            friend class MR::GUI::MRView::Window;
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
              setShortcut (tr (std::string ("Shift+F"+str (index)).c_str()));
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


