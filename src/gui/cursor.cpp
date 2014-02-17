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


#include "gui/cursor.h"

namespace MR
{
  namespace GUI
  {

    QCursor Cursor::pan_crosshair;
    QCursor Cursor::forward_backward;
    QCursor Cursor::window;
    QCursor Cursor::crosshair;
    QCursor Cursor::inplane_rotate;
    QCursor Cursor::throughplane_rotate;

    Cursor::Cursor ()
    {
      pan_crosshair = QCursor (QPixmap (":/cursor_pan.svg"), 8, 8);
      forward_backward = QCursor (QPixmap (":/cursor_pan_through_plane.svg"), 8, 8);
      window = QCursor (QPixmap (":/cursor_brightness_contrast.svg"), 8, 8);
      crosshair = QCursor (QPixmap (":/cursor_crosshairs.svg"), 8, 8);
      inplane_rotate = QCursor (QPixmap (":/cursor_rotate_inplane.svg"), 8, 8);
      throughplane_rotate = QCursor (QPixmap (":/cursor_rotate_throughplane.svg"), 8, 8);
    }

  }
}

