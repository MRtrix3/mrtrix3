#include "gui/mrview/tool/base.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        Base::Base (Window& main_window, Dock* parent) : 
          QFrame (parent),
          window (main_window) { 
            QFont f = font();
            f.setPointSize (MR::File::Config::get_int ("MRViewToolFontSize", f.pointSize()-1));
            setFont (f);

            setFrameShadow (QFrame::Sunken); 
            setFrameShape (QFrame::Panel);
          }



        QSize Base::sizeHint () const
        {
          return minimumSizeHint();
        }


        void Base::draw (const Projection& transform, bool is_3D) { }

        void Base::drawOverlays (const Projection& transform) { }

        bool Base::process_batch_command (const std::string& cmd, const std::string& args) 
        {
          return false;
        }

      }
    }
  }
}



