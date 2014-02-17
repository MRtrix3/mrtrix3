#ifndef __gui_dialog_opengl_h__
#define __gui_dialog_opengl_h__

#include "gui/opengl/gl.h"

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {

      class OpenGL : public QDialog
      {
        public:
          OpenGL (QWidget* parent, const QGLFormat& format);
      };

    }
  }
}

#endif


