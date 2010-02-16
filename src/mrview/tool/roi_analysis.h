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

#ifndef __viewer_tool_roi_analysis_h__
#define __viewer_tool_roi_analysis_h__

#include "mrview/tool/base.h"

namespace MR {
  namespace Viewer {
    namespace Tool {

      class ROI : public Base
      {
        Q_OBJECT

        public:
          ROI (Window& parent) : Base (tr("ROI analysis"), tr("Draw & analyse regions of interest"), parent) { }

        private slots:
          void slot ();

        protected:
          virtual QWidget* create ();
      };

    }
  }
}

#endif



