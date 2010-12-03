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

#ifndef __viewer_mode_base_h__
#define __viewer_mode_base_h__

#include "opengl/gl.h"

#include <QCursor>
#include <QMouseEvent>
#include <QMenu>
#include <QGLWidget>
#include <QFontMetrics>

#include "mrview/window.h"

namespace MR {
  namespace Viewer {
    namespace Mode {

      class Base : public QObject
      {
        Q_OBJECT

        public:
          Base (Window& parent);
          virtual ~Base ();

          virtual void paint ();

          virtual bool mouse_click ();
          virtual bool mouse_move ();
          virtual bool mouse_doubleclick ();
          virtual bool mouse_release ();
          virtual bool mouse_wheel (float delta, Qt::Orientation orientation);

          void paintGL () 
          { 
            modelview_matrix[0] = NAN; 
            paint(); 
            get_modelview_projection_viewport();
          }

        public slots:
          void updateGL ();
          virtual void reset ();
          virtual void toggle_show_xyz ();

        protected:
          Window& window;

          QAction *reset_action, *show_focus_action;
          QAction *show_image_info_action, *show_position_action, *show_orientation_action;

          static const int TopEdge = 1;
          static const int BottomEdge = 1<<1;
          static const int LeftEdge = 1<<2;
          static const int RightEdge = 1<<3;

          const QPoint& mouse_pos () const { return (currentPos); }
          QPoint mouse_dpos () const { return (currentPos - lastPos); }
          QPoint mouse_dpos_static () const 
          {
            QCursor::setPos (reinterpret_cast<QWidget*> (window.glarea)->mapToGlobal (initialPos));
            return (currentPos - initialPos); 
          }
          Qt::MouseButtons mouse_buttons () const { return (buttons_); }
          Qt::KeyboardModifiers mouse_modifiers () const { return (modifiers_); }
          int mouse_edge () const { return (edge_); }

          void add_action (QAction* action) { window.view_menu->insertAction (window.view_menu_mode_area, action); }
          void add_action_common (QAction* action) { window.view_menu->insertAction (window.view_menu_mode_common_area, action); }

          Point<> model_to_screen (const Point<>& pos) const
          {
            double wx, wy, wz;
            get_modelview_projection_viewport();
            gluProject (pos[0], pos[1], pos[2], modelview_matrix, 
                projection_matrix, viewport_matrix, &wx, &wy, &wz);
            return (Point<> (wx, wy, wz));
          }

          Point<> screen_to_model (const Point<>& pos) const 
          {
            double wx, wy, wz;
            get_modelview_projection_viewport();
            gluUnProject (pos[0], height()-pos[1], pos[2], modelview_matrix, 
                projection_matrix, viewport_matrix, &wx, &wy, &wz);
            return (Point<> (wx, wy, wz));
          }

          Point<> screen_to_model (const QPoint& pos) const
          {
            Point<> f (model_to_screen (focus()));
            f[0] = pos.x();
            f[1] = pos.y();
            return (screen_to_model (f));
          }

          Point<> screen_to_model () const { return (screen_to_model (currentPos)); } 

          Point<> screen_to_model_direction (const Point<>& pos) const
          { return (screen_to_model (pos) - screen_to_model (Point<> (0.0, 0.0, 0.0))); } 


          Image* image () { return (window.current_image()); }

          const Math::Quaternion& orientation () const { return (window.orient); }
          float FOV () const { return (window.field_of_view); }
          const Point<>& focus () const { return (window.focal_point); }
          const Point<>& target () const { return (window.camera_target); }
          int projection () const { return (window.proj); }

          void set_focus (const Point<>& p) { window.focal_point = p; }
          void set_target (const Point<>& p) { window.camera_target = p; }
          void set_projection (int p) { window.proj = p; }
          void set_FOV (float value) { window.field_of_view = value; }
          void change_FOV_fine (float factor) { window.field_of_view *= Math::exp (0.005*factor); }
          void change_FOV_scroll (float factor) { change_FOV_fine (20.0 * factor); }

          int width () const { get_modelview_projection_viewport(); return (viewport_matrix[2]); }
          int height () const { get_modelview_projection_viewport(); return (viewport_matrix[3]); }
          QWidget* glarea () const { return (reinterpret_cast <QWidget*>(window.glarea)); }

          void renderText (int x, int y, const std::string& text)
          { reinterpret_cast<QGLWidget*>(window.glarea)->renderText (x, height()-y, text.c_str(), font_); }

          void renderText (const std::string& text, int position, int line = 0)
          {
            QFontMetrics fm (font_);
            QString s (text.c_str());
            int x, y;

            if (position & RightEdge) x = width() - fm.height()/2 - fm.width (s);
            else if (position & LeftEdge) x = fm.height()/2;
            else x = (width() - fm.width (s)) / 2;

            if (position & TopEdge) y = 2 * fm.height()/2 + line * fm.lineSpacing();
            else if (position & BottomEdge) y = height() - fm.height()/2 - line * fm.lineSpacing();
            else y = (height() + fm.height()) / 2 + line * fm.lineSpacing();

            reinterpret_cast<QGLWidget*>(window.glarea)->renderText (x, y, text.c_str(), font_); 
          }

          //void renderText (const std::string& text, 

        private:
          mutable GLdouble modelview_matrix[16], projection_matrix[16];
          mutable GLint viewport_matrix[4];

          QPoint currentPos, lastPos, initialPos;
          Qt::MouseButtons buttons_;
          Qt::KeyboardModifiers modifiers_;
          int edge_;

          QFont font_;

          void mousePressEvent (QMouseEvent* event)
          {
            if (buttons_ != Qt::NoButton) return;
            buttons_ = event->buttons();
            modifiers_ = event->modifiers();
            lastPos = currentPos = initialPos = event->pos();
            if (mouse_click()) event->accept();
            else event->ignore();
          }

          void mouseMoveEvent (QMouseEvent* event)
          {
            lastPos = currentPos;
            currentPos = event->pos();
            if (buttons_ == Qt::NoButton) 
              edge_ = 
                ( 10*currentPos.x() < width() ? LeftEdge : 0 ) |
                ( 10*(width()-currentPos.x()) < width() ? RightEdge : 0 ) |
                ( 10*currentPos.y() < width() ? TopEdge : 0 ) |
                ( 10*(width()-currentPos.y()) < width() ? BottomEdge : 0 );
            if (mouse_move()) event->accept();
            else event->ignore();
          }


          void mouseDoubleClickEvent (QMouseEvent* event) 
          {
            if (mouse_doubleclick()) 
              event->accept();
            else
              event->ignore(); 
          }

          void mouseReleaseEvent (QMouseEvent* event) 
          {
            if (event->buttons() != Qt::NoButton) return;
            if (mouse_release()) 
              event->accept();
            else
              event->ignore(); 

            buttons_ = Qt::NoButton;
            modifiers_ = Qt::NoModifier;
          }

          void wheelEvent (QWheelEvent* event)
          {
            buttons_ = event->buttons();
            modifiers_ = event->modifiers();
            lastPos = currentPos = event->pos();
            if (mouse_wheel (event->delta()/120.0, event->orientation())) event->accept();
            else event->ignore();
          }

          void get_modelview_projection_viewport () const 
          {
            if (isnan (modelview_matrix[0])) {
              glGetIntegerv (GL_VIEWPORT, viewport_matrix); 
              glGetDoublev (GL_MODELVIEW_MATRIX, modelview_matrix);
              glGetDoublev (GL_PROJECTION_MATRIX, projection_matrix); 
            }
          }

          friend class MR::Viewer::Window;
      };

      Base* create (Window& parent, size_t index);
      const char* name (size_t index);
      const char* tooltip (size_t index);

    }
  }
}

#endif


