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
#include "gui/projection.h"

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
            Dock (QWidget* parent, const QString& name) :
              QDockWidget (name, parent), tool (NULL) { }

            Base* tool;
        };


        class Base : public QFrame {
          public:
            Base (Window& main_window, Dock* parent) : 
              QFrame (parent),
              window (main_window) { 
              setFrameShadow (QFrame::Plain); 
              setFrameShape (QFrame::NoFrame);
            }
            Window& window;

            virtual void draw (const Projection& transform, bool is_3D);
            virtual void drawOverlays (const Projection& transform);
            virtual bool process_batch_command (const std::string& cmd, const std::string& args);
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
              dock (NULL) {
              setCheckable (true);
              setShortcut (tr (std::string ("Ctrl+F" + str (index)).c_str()));
              setStatusTip (tr (description));
            }

            virtual Dock* create (Window& main_window) = 0;
            Dock* dock;
        };
        //! \endcond


        template <class T> 
          Dock* create (const QString& text, Window& main_window)
          {
            Dock* dock = new Dock (&main_window, text);
            main_window.addDockWidget (Qt::RightDockWidgetArea, dock);
            dock->tool = new T (main_window, dock);
            dock->setWidget (dock->tool);
            dock->setFloating (true);
            dock->show();
            return dock;
          }


        template <class T> 
          class Action : public __Action__
        {
          public:
            Action (QActionGroup* parent,
                const char* const name,
                const char* const description,
                int index) :
              __Action__ (parent, name, description, index) { }

            virtual Dock* create (Window& parent) {
              dock = Tool::create<T> (this->text(), parent);
              return dock;
            }
        };




      }
  }
}
}

#endif


