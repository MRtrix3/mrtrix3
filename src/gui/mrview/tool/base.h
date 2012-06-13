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
#include <QFrame>
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

        class Dock : public QDockWidget
        {
          public:
            Dock (Window& parent, const QString& name) : QDockWidget (name, &parent) { }

          protected:
            virtual void showEvent (QShowEvent * event);
            virtual void closeEvent (QCloseEvent * event);
            virtual void hideEvent (QCloseEvent * event);
        };

        class Base : public QFrame {
          public:
            Base (Dock* parent) : QFrame (parent) { 
              setFrameShadow (QFrame::Sunken); 
              setFrameShape (QFrame::Panel);
            }
            Window& window() {
              return *dynamic_cast<Window*> (parentWidget()->parentWidget());
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
              setShortcut (tr (std::string ("Shift+F" + str (index)).c_str()));
              setStatusTip (tr (description));
            }

            virtual Dock* create (Window& parent) = 0;
            Dock* instance;
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

            virtual Dock* create (Window& parent) {
              instance = new Dock (parent, this->text());
              instance->setWidget (new T (instance));
              return instance;
            }
        };


      }
    }
  }
}

#endif


