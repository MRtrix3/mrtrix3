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

#ifndef __gui_mrview_tool_base_h__
#define __gui_mrview_tool_base_h__

#include <QAction>
#include <QDockWidget>

#include "gui/mrview/window.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        class Base : public QDockWidget
        {
            Q_OBJECT

          public:
            Base (Window& parent);

            Window& window() {
              return *dynamic_cast<Window*> (parentWidget());
            }
        };






        //! \cond skip
        class __Action__ : public QAction
        {
          public:
            __Action__ (QActionGroup* parent,
                        const char* const name,
                        const char* const description,
                        int index) :
              QAction (name, parent),
              instance (NULL) {
              setCheckable (true);
              setShortcut (tr (std::string ("F"+str (index)).c_str()));
              setStatusTip (tr (description));
            }

            virtual Base* create (Window& parent) = 0;
            Base* instance;
        };
        //! \endcond



        template <class T> class Action : public __Action__
        {
          public:
            Action (QActionGroup* parent,
                    const char* const name,
                    const char* const description,
                    int index) :
              __Action__ (parent, name, description, index) { }

            virtual Base* create (Window& parent) {
              instance = new T (parent);
              return instance;
            }
        };


      }
    }
  }
}

#endif


