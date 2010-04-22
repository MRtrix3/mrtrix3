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

#include <QCursor>
#include <QMouseEvent>
#include <QMenu>

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

          virtual void mousePressEvent (QMouseEvent* event);
          virtual void mouseMoveEvent (QMouseEvent* event);
          virtual void mouseDoubleClickEvent (QMouseEvent* event);
          virtual void mouseReleaseEvent (QMouseEvent* event);
          virtual void wheelEvent (QWheelEvent* event);

          void paintGL () 
          { 
            modelview_matrix[0] = NAN; 
            paint(); 
            get_modelview_projection_viewport();
          }

        public slots:
          void updateGL ();

        protected:
          Window& window;
          QPoint lastPos;
          Qt::MouseButtons lastButtons;
          Qt::KeyboardModifiers lastModifiers;

          void grab_event (QMouseEvent* event) {
            lastButtons = event->buttons();
            lastModifiers = event->modifiers();
            lastPos = event->pos();
          }

          Point distance_moved (const QMouseEvent* event) { 
            Point d (event->pos().x()-lastPos.x(), event->pos().y()-lastPos.y(), 0.0); 
            lastPos = event->pos();
            return (d);
          }

          Point distance_moved_motionless (const QMouseEvent* event) {
            Point d (event->pos().x()-lastPos.x(), event->pos().y()-lastPos.y(), 0.0); 
            QCursor::setPos (reinterpret_cast<QWidget*> (window.glarea)->mapToGlobal (lastPos));
            return (d);
          }

          void add_action (QAction* action) { window.view_menu->insertAction (window.view_menu_mode_area, action); }

          Point model_to_screen (const Point& pos) {
            double wx, wy, wz;
            get_modelview_projection_viewport();
            gluProject (pos[0], pos[1], pos[2], modelview_matrix, 
                projection_matrix, viewport_matrix, &wx, &wy, &wz);
            return (Point (wx, wy, wz));
          }

          Point screen_to_model (const Point& pos) {
            double wx, wy, wz;
            get_modelview_projection_viewport();
            gluUnProject (pos[0], height()-pos[1], pos[2], modelview_matrix, 
                projection_matrix, viewport_matrix, &wx, &wy, &wz);
            return (Point (wx, wy, wz));
          }

          Point screen_to_model (const QPoint& pos) {
            Point f (model_to_screen (focus()));
            f[0] = pos.x();
            f[1] = pos.y();
            return (screen_to_model (f));
          }

          Point screen_to_model (const QMouseEvent* event) { return (screen_to_model (event->pos())); } 
          Point screen_to_model (const QWheelEvent* event) { return (screen_to_model (event->pos())); } 

          Point screen_to_model_direction (const Point& pos) { return (screen_to_model (pos) - screen_to_model (Point (0.0, 0.0, 0.0))); } 
          Point screen_to_model_direction (const QPoint& pos) { return (screen_to_model (pos) - screen_to_model (Point (0.0, 0.0, 0.0))); }

          Image* image () { return (window.current_image()); }

          const Math::Quaternion& orientation () const { return (window.orient); }
          float FOV () const { return (window.field_of_view); }
          const Point& focus () const { return (window.focal_point); }
          const Point& target () const { return (window.camera_target); }
          int projection () const { return (window.proj); }

          void set_focus (const Point& p) { window.focal_point = p; }
          void set_target (const Point& p) { window.camera_target = p; }
          void set_projection (int p) { window.proj = p; }
          void set_FOV (float value) { window.field_of_view = value; }
          void change_FOV_fine (float factor) { window.field_of_view *= Math::exp (0.01*factor); }
          void change_FOV_scroll (float factor) { change_FOV_fine (10.0 * factor); }

          int width () const { get_modelview_projection_viewport(); return (viewport_matrix[2]); }
          int height () const { get_modelview_projection_viewport(); return (viewport_matrix[3]); }
          QWidget* glarea () const { return (reinterpret_cast <QWidget*>(window.glarea)); }

          bool cursor_left (const QMouseEvent* event) const { return (10*event->x() < width()); }
          bool cursor_right (const QMouseEvent* event) const { return (10*(width()-event->x()) < width()); }
          bool cursor_top (const QMouseEvent* event) const { return (10*event->y() < height()); }
          bool cursor_bottom (const QMouseEvent* event) const { return (10*(height()-event->y()) < height()); }

        private:
          mutable GLdouble modelview_matrix[16], projection_matrix[16];
          mutable GLint viewport_matrix[4];

          void get_modelview_projection_viewport () const {
            if (isnan (modelview_matrix[0])) {
              glGetIntegerv (GL_VIEWPORT, viewport_matrix); 
              glGetDoublev (GL_MODELVIEW_MATRIX, modelview_matrix);
              glGetDoublev (GL_PROJECTION_MATRIX, projection_matrix); 
            }
          }
      };

      Base* create (Window& parent, size_t index);
      const char* name (size_t index);
      const char* tooltip (size_t index);

    }
  }
}

#endif


