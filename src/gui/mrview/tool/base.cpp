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
#include <cassert>

#include "gui/mrview/window.h"
#include "gui/mrview/tool/base.h"
#include "gui/mrview/tool/list.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        Base::Base (const QString& name, const QString& description, Window& parent) :
          QDockWidget (name, &parent), window (parent), widget (NULL)
        {
          toggleViewAction()->setStatusTip (description);
          setAllowedAreas (Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
          setVisible (false);
        }

        void Base::showEvent (QShowEvent* event)
        {
          if (!widget) {
            widget = create ();
            widget->setMinimumWidth (128);
            setWidget (widget);
          }
          QDockWidget::showEvent (event);
        }

        Base* create (Window& parent, size_t index)
        {
          switch (index) {
#include "gui/mrview/tool/list.h"
            default:
              assert (0);
          };
          return (NULL);
        }

        namespace
        {
          bool present (size_t index)
          {
            switch (index) {
#include "gui/mrview/tool/list.h"
              default:
                return (false);
            };
            return (false);
          }
        }

        size_t count ()
        {
          size_t n = 0;
          while (present (n)) ++n;
          return (n);
        }
      }
    }
  }
}



