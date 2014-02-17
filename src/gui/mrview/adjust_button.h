/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#ifndef __gui_mrview_log_spin_box_h__
#define __gui_mrview_log_spin_box_h__

#include "mrtrix.h"
#include "gui/opengl/gl.h"

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

