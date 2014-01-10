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

      class AdjustButton : public QLineEdit
      {
        Q_OBJECT

        public:
          AdjustButton (QWidget* parent, float change_rate = 1.0);

          float value () const { 
            if (text().isEmpty()) 
              return NAN;
            try {
              return to<float> (text().toStdString());
            }
            catch (Exception) {
              return NAN;
            }
          }

          void setValue (float val) {
            if (isfinite (val)) {
              if (val > max)
                setText (str(max).c_str());
              else if (val < min)
                setText (str(min).c_str());
              else
                setText (str(val).c_str());
            }
            else 
              clear();
          }


          void setRate (float new_rate) {
            rate = new_rate;
          }

          void setMin (float val) {
            min = val;
          }

          void setMax (float val) {
            max = val;
          }

        signals:
          void valueChanged ();

        protected slots:
          void onSetValue ();

        protected:
          float rate, min, max;
          int previous_y;
          bool adjusting;

          bool eventFilter (QObject *obj, QEvent *event);
      };


    }
  }
}

#endif

