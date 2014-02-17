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


