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

#include "file/config.h"

#include "gui/mrview/window.h"
#include "gui/projection.h"

#define LAYOUT_SPACING 3

#define __STR__(x) #x
#define __STR(x) __STR__(x)

namespace MR
{
  namespace App { 
    class OptionList; 
    class Options;
  }

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

            void closeEvent (QCloseEvent*) override;

            Base* tool;
        };


        class Base : public QFrame {
          public:
            Base (Window& main_window, Dock* parent);
            Window& window;

            static void add_commandline_options (MR::App::OptionList& options);
            virtual bool process_commandline_option (const MR::App::ParsedOption& opt);

            virtual QSize sizeHint () const;

            void grab_focus () {
              window.tool_has_focus = this;
              window.set_cursor();
            }
            void release_focus () {
              if (window.tool_has_focus == this) {
                window.tool_has_focus = nullptr;
                window.set_cursor();
              }
            }

            class HBoxLayout : public QHBoxLayout {
              public:
                HBoxLayout () : QHBoxLayout () { init(); }
                HBoxLayout (QWidget* parent) : QHBoxLayout (parent) { init(); }
              protected:
                void init () {
                  setSpacing (LAYOUT_SPACING);
                  setContentsMargins(LAYOUT_SPACING,LAYOUT_SPACING,LAYOUT_SPACING,LAYOUT_SPACING);
                }
            };

            class VBoxLayout : public QVBoxLayout {
              public:
                VBoxLayout () : QVBoxLayout () { init(); }
                VBoxLayout (QWidget* parent) : QVBoxLayout (parent) { init(); }
              protected:
                void init () {
                  setSpacing (LAYOUT_SPACING);
                  setContentsMargins(LAYOUT_SPACING,LAYOUT_SPACING,LAYOUT_SPACING,LAYOUT_SPACING);
                }
            };

            class GridLayout : public QGridLayout {
              public:
                GridLayout () : QGridLayout () { init(); }
                GridLayout (QWidget* parent) : QGridLayout (parent) { init(); }
              protected:
                void init () {
                  setSpacing (LAYOUT_SPACING);
                  setContentsMargins(LAYOUT_SPACING,LAYOUT_SPACING,LAYOUT_SPACING,LAYOUT_SPACING);
                }
            };


            class FormLayout : public QFormLayout {
              public:
                FormLayout () : QFormLayout () { init(); }
                FormLayout (QWidget* parent) : QFormLayout (parent) { init(); }
              protected:
                void init () {
                  setSpacing (LAYOUT_SPACING);
                  setContentsMargins(LAYOUT_SPACING,LAYOUT_SPACING,LAYOUT_SPACING,LAYOUT_SPACING);
                }
            };

            void adjustSize();
            virtual void draw (const Projection& transform, bool is_3D, int axis, int slice);
            virtual void draw_colourbars ();
            virtual size_t visible_number_colourbars () { return 0; }
            virtual int draw_tool_labels (int, int, const Projection&) const { return 0; }
            virtual bool mouse_press_event ();
            virtual bool mouse_move_event ();
            virtual bool mouse_release_event ();
            virtual void close_event() { }
            virtual void reset_event () { }
            virtual QCursor* get_cursor ();
            void update_cursor() { window.set_cursor(); }
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
            dock->tool->adjustSize();
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


