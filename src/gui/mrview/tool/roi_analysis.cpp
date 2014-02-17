#include "mrtrix.h"
#include "gui/mrview/window.h"
#include "gui/mrview/tool/roi_analysis.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {
        ROI::ROI (Window& parent, const QString& name) :
          Base (parent, name) { 
            setWidget (new QLabel ("ROI analysis", this));
          }

        void ROI::slot ()
        {
          TEST;
        }


      }
    }
  }
}




