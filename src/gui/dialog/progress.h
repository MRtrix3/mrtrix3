#ifndef __gui_dialog_progressbar_h__
#define __gui_dialog_progressbar_h__

#include "gui/opengl/gl.h"
#include "progressbar.h"

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {
      namespace ProgressBar
      {

        void display (ProgressInfo& p);
        void done (ProgressInfo& p);

      }
    }
  }
}

#endif

