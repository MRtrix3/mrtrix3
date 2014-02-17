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


#include "gui/mrview/displayable.h"
#include "progressbar.h"
#include "image/stride.h"
#include "gui/mrview/window.h"



namespace MR
{
  namespace GUI
  {
    namespace MRView
    {

      Displayable::Displayable (const std::string& filename) :
        QAction (NULL),
        lessthan (NAN),
        greaterthan (NAN),
        display_midpoint (NAN),
        display_range (NAN),
        transparent_intensity (NAN),
        opaque_intensity (NAN),
        alpha (NAN),
        colourmap (0),
        show (true),
        filename (filename),
        value_min (NAN),
        value_max (NAN),
        flags_ (0x00000000) { }


      Displayable::Displayable (Window& window, const std::string& filename) :
        QAction (shorten (filename, 20, 0).c_str(), &window),
        lessthan (NAN),
        greaterthan (NAN),
        display_midpoint (NAN),
        display_range (NAN),
        transparent_intensity (NAN),
        opaque_intensity (NAN),
        alpha (NAN),
        colourmap (0), 
        show (true),
        filename (filename),
        value_min (NAN),
        value_max (NAN),
        flags_ (0x00000000) {
          connect (this, SIGNAL(scalingChanged()), &window, SLOT(on_scaling_changed()));
      }


      Displayable::~Displayable ()
      {
      }


      bool Displayable::Shader::need_update (const Displayable& object) const { 
        return flags != object.flags() || colourmap != object.colourmap;
      }


      void Displayable::Shader::update (const Displayable& object) {
        flags = object.flags();
        colourmap = object.colourmap;
      }

    }
  }
}


