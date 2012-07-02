#ifndef __gui_mrview_log_spin_box_h__
#define __gui_mrview_log_spin_box_h__

#include <QFrame>
#include <QLineEdit>
#include <QToolButton>

#include "mrtrix.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {


      class AdjustButton : public QFrame
      {
        Q_OBJECT

        public:
          AdjustButton (QWidget* parent, float change_rate = 1.0);

          float value () const { 
            if (text.text().isEmpty()) 
              return NAN;
            try {
              return to<float> (text.text().toStdString());
            }
            catch (Exception) {
              return NAN;
            }
          }

          void setValue (float val) {
            text.setText (str(val).c_str());
          }

          void setRate (float new_rate) {
            rate = new_rate;
          }

        signals:
          void valueChanged ();

        protected slots:
          void onSetValue ();

        protected:
          QLineEdit text;
          QToolButton button;
          float rate;
          int previous_y;

          bool eventFilter (QObject *obj, QEvent *event);
      };


    }
  }
}

#endif

