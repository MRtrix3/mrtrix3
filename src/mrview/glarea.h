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

#ifndef __viewer_glarea_h__
#define __viewer_glarea_h__

#include <QGLWidget>

namespace MR {
  namespace Viewer {

    class Window;
    namespace Mode { class Base; }

    class GLArea : public QGLWidget
    {
      Q_OBJECT

      public:
        GLArea (QWidget *parent);
        ~GLArea ();
        QSize minimumSizeHint () const;
        QSize sizeHint () const;

        void set_mode (int index);

      protected:
        void initializeGL ();
        void paintGL ();
        void resizeGL (int width, int height);

        void mousePressEvent (QMouseEvent* event);
        void mouseMoveEvent (QMouseEvent* event);
        void mouseDoubleClickEvent (QMouseEvent* event);
        void mouseReleaseEvent (QMouseEvent* event);
        void wheelEvent (QWheelEvent* event);

      private slots:

      private:
        Mode::Base* mode;

        friend class Window;
    };

  }
}

#endif

