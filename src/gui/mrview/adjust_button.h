/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __gui_mrview_adjust_button_h__
#define __gui_mrview_adjust_button_h__

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
      { NOMEMALIGN
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

          bool isMin() const { return is_min; }
          bool isMax() const { return is_max; }

          void setValue (float val) {
            if (std::isfinite (val)) {
              if (val >= max) {
                setText (str(max).c_str());
                is_min = false; is_max = true;
              } else if (val <= min) {
                setText (str(min).c_str());
                is_min = true; is_max = false;
              } else {
                setText (str(val).c_str());
                is_min = is_max = false;
              }
            }
            else  {
              clear();
              is_min = is_max = false;
            }
          }


          void setRate (float new_rate) {
            rate = new_rate;
          }

          void setMin (float val) {
            min = val;
            if (value() <= val) {
              setValue (val);
              is_min = true;
              emit valueChanged();
              emit valueChanged (val);
            }
          }

          void setMax (float val) {
            max = val;
            if (value() >= val) {
              setValue (val);
              is_max = true;
              emit valueChanged();
              emit valueChanged (val);
            }
          }

          float getMin() const { return min; }
          float getMax() const { return max; }

        signals:
          void valueChanged ();
          void valueChanged (float val);

        protected slots:
          void onSetValue ();

        protected:
          float rate, min, max;
          bool is_min, is_max;
          int previous_y;

          int deadzone_y;
          float deadzone_value;

          bool eventFilter (QObject *obj, QEvent *event);
      };


    }
  }
}

#endif

