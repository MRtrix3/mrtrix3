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

#include <QMouseEvent>
#include <QHBoxLayout>
#include <QDoubleValidator>

#include "debug.h"
#include "gui/mrview/adjust_button.h"
#include "math/math.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {

      AdjustButton::AdjustButton (QWidget* parent, float change_rate) :
        QFrame (parent),
        text (this),
        button (this),
        rate (change_rate),
        min (-std::numeric_limits<float>::max()),
        max (std::numeric_limits<float>::max()) {
          text.setValidator (new QDoubleValidator (this));

          button.setToolTip (tr ("Click & drag to adjust"));
          button.setIcon (QIcon (":/adjust.svg"));
          button.setAutoRaise (true);

          QHBoxLayout* layout = new QHBoxLayout (this);
          layout->setContentsMargins (0, 0, 0, 0);
          layout->setSpacing (5);
          layout->addWidget (&text, 1);
          layout->addWidget (&button, 0);

          connect (&text, SIGNAL (editingFinished()), SLOT (onSetValue()));
          button.installEventFilter (this);
        }


      void AdjustButton::onSetValue () { emit valueChanged(); }



      bool AdjustButton::eventFilter (QObject* obj, QEvent* event)
      {
        if (this->isEnabled()) {
          if (event->type() == QEvent::MouseButtonPress)
            previous_y = ((QMouseEvent*) event)->y();
          else if (event->type() == QEvent::MouseMove) {
            QMouseEvent* mouse_event = (QMouseEvent*) event;
            setValue (value() - rate * (mouse_event->y() - previous_y));
            previous_y = mouse_event->y();
            emit valueChanged();
            return true;

          }
        }
        return QObject::eventFilter(obj, event);
      }

    }
  }
}

