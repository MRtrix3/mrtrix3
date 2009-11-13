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

#include <QAction>

#include "mrview/tool/base.h"

namespace MR {
  namespace Viewer {
    namespace Tool {

      Base::Base (const QString& name, const QString& description, QWidget *parent) : 
        QDockWidget (name, parent), widget (NULL) { 
          toggleViewAction()->setStatusTip (description);
          setAllowedAreas (Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
          setVisible (false);
        }

      void Base::showEvent (QShowEvent* event) { 
        if (!widget) {
          widget = create ();
          widget->setMinimumWidth (128);
          setWidget (widget);
        }
        QDockWidget::showEvent (event);
      }


    }
  }
}



