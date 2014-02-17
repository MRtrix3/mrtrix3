#include <cassert>
#include "gui/dialog/progress.h"

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {
      namespace ProgressBar
      {

        void display (ProgressInfo& p)
        {
          if (!p.data) {
            p.data = new QProgressDialog (p.text.c_str(), "Cancel", 0, p.as_percentage ? 100 : 0);
            reinterpret_cast<QProgressDialog*> (p.data)->setWindowModality (Qt::WindowModal);
          }
          reinterpret_cast<QProgressDialog*> (p.data)->setValue (p.value);
        }


        void done (ProgressInfo& p)
        {
          delete reinterpret_cast<QProgressDialog*> (p.data);
          p.data = NULL;
        }

      }
    }
  }
}



