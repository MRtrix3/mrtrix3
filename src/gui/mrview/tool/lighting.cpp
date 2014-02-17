#include "gui/mrview/tool/lighting.h"
#include "gui/dialog/lighting.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        Lighting::Lighting (Window& main_window, Dock* parent) : 
          Base (main_window, parent)
        {
          VBoxLayout* main_box = new VBoxLayout (this);
          main_box->addWidget (new Dialog::LightingSettings (this, window.lighting(), false));
          main_box->addStretch ();
          setMinimumSize (main_box->minimumSize());
        }



      }
    }
  }
}







