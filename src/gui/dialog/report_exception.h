#ifndef __gui_dialog_exception_h__
#define __gui_dialog_exception_h__

#include "gui/opengl/gl.h"
#include "exception.h"

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {

      extern void display_exception (const Exception& E, int log_level);

    }
  }
}

#endif

