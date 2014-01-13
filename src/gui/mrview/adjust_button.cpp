/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 13/11/09.

   This file is part of MRtrix.

   MRtrix is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MRtrix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

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
        adjusting (false) {
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


      void AdjustButton::onSetValue () { emit valueChanged(); }



      bool AdjustButton::eventFilter (QObject* obj, QEvent* event)
      {
        if (this->isEnabled()) {
          if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent* mevent = static_cast<QMouseEvent*> (event);
            if (mevent->button() == mevent->buttons())
              previous_y = mevent->y();
          }
          else if (event->type() == QEvent::MouseButtonRelease) {
            if (static_cast<QMouseEvent*> (event)->buttons() == Qt::NoButton)
              adjusting = false;
          }
          else if (event->type() == QEvent::MouseMove) {
            QMouseEvent* mevent = static_cast<QMouseEvent*> (event);
            if (mevent->buttons() != Qt::NoButton) {
              if (!adjusting && Math::abs (mevent->y() - previous_y) > 4) 
                adjusting = true;
              if (adjusting) {
                setValue (value() - rate * (mevent->y() - previous_y));
                previous_y = mevent->y();
                emit valueChanged();
                return true;
              }
            }
          }
        }
        return QObject::eventFilter (obj, event);
      }

    }
  }
}

