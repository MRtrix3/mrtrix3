#ifndef __gui_spin_box_h__
#define __gui_spin_box_h__

#include "mrtrix.h"
#include "gui/opengl/gl.h"

namespace MR
{
  namespace GUI
  {

    class SpinBox : public QSpinBox
    {
        Q_OBJECT

      public:
        using QSpinBox::QSpinBox;

      private:
        void timerEvent (QTimerEvent* event)
        {
          // Process all events, which may include a mouse release event
          // Only allow the timer to trigger additional value changes if the user
          //   has in fact held the mouse button, rather than the timer expiry
          //   simply appearing before the mouse release in the event queue
          qApp->processEvents();
          if (QApplication::mouseButtons() & Qt::LeftButton)
            QSpinBox::timerEvent (event);
        }

    };


  }
}

#endif

