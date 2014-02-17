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

