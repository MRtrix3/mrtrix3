#ifndef __gui_mrview_tool_roi_analysis_h__
#define __gui_mrview_tool_roi_analysis_h__

#include "gui/mrview/tool/base.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        class ROI : public Base
        {
            Q_OBJECT

          public:
            ROI (Window& parent, const QString& name);

          private slots:
            void slot ();

        };

      }
    }
  }
}

#endif



