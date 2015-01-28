#ifndef __gui_mrview_log_spin_box_h__
#define __gui_mrview_log_spin_box_h__

#include "mrtrix.h"
#include "gui/opengl/gl.h"

#define ADJUST_BUTTON_DEADZONE_SIZE 8

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
            if (std::isfinite (val)) {
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

          float getMin() const { return min; }
          float getMax() const { return max; }

        signals:
          void valueChanged ();

        protected slots:
          void onSetValue ();

        protected:
          float rate, min, max;
          int previous_y;

          int deadzone_y;
          float deadzone_value;

          bool eventFilter (QObject *obj, QEvent *event);
      };


    }
  }
}

#endif

