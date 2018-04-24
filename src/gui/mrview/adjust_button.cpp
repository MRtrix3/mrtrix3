/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "gui/mrview/adjust_button.h"
#include "math/math.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {

      AdjustButton::AdjustButton (QWidget* parent, float change_rate) :
        QLineEdit (parent),
        rate (change_rate),
        min (-std::numeric_limits<float>::max()),
        max (std::numeric_limits<float>::max()),
        is_min (false),
        is_max (false),
        deadzone_y (-1),
        deadzone_value (NAN)
      {
          setValidator (new QDoubleValidator (this));

          setToolTip (tr ("Click & drag to adjust"));
          setAlignment (Qt::AlignRight);

          connect (this, SIGNAL (editingFinished()), SLOT (onSetValue()));
          installEventFilter (this);
          setStyleSheet ((
              "QLineEdit { "
              "padding: 0.1em 20px 0.2em 0.3ex; "
              "background: qlineargradient(x1:0, y1:0, x2:0, y2:0.2, stop:0 gray, stop:1 white) url(:/adjustbutton.svg); "
              "background-position: right; "
              "background-repeat: no-repeat; "
              "font-size: " + str(font().pointSize()) + "pt; "
              "border: 1px solid grey; "
              "border-color: black lightgray white gray; "
              "border-radius: 0.3em }").c_str());

        }


      void AdjustButton::onSetValue () { emit valueChanged(); emit valueChanged(value()); }



      bool AdjustButton::eventFilter (QObject* obj, QEvent* event)
      {
        if (this->isEnabled()) {
          if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* mevent = static_cast<QMouseEvent*> (event);
            if (mevent->button() == mevent->buttons()) {
              previous_y = deadzone_y = mevent->y();
              deadzone_value = value();
            }
          }
          else if (event->type() == QEvent::MouseButtonRelease) {
            if (static_cast<QMouseEvent*> (event)->buttons() == Qt::NoButton) {
              deadzone_y = -1;
              deadzone_value = NAN;
            }
          }
          else if (event->type() == QEvent::MouseMove) {
            QMouseEvent* mevent = static_cast<QMouseEvent*> (event);
            if (mevent->buttons() != Qt::NoButton) {
              if (abs (mevent->y() - deadzone_y) < ADJUST_BUTTON_DEADZONE_SIZE) {
                if (value() != deadzone_value) {
                  setValue (deadzone_value);
                  emit valueChanged();
                  emit valueChanged(value());
                }
              } else if (mevent->y() != previous_y) {
                setValue (value() - rate * (mevent->y() - previous_y));
                emit valueChanged();
                emit valueChanged(value());
              }
              previous_y = mevent->y();
              return true;
            }
          }
        }
        return QObject::eventFilter (obj, event);
      }

    }
  }
}

