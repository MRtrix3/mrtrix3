/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see www.mrtrix.org
 *
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
        class Base;




        class Dock : public QDockWidget
        { NOMEMALIGN
          public:
            Dock (const QString& name) :
              QDockWidget (name, Window::main), tool (nullptr) { }
            ~Dock ();

            void closeEvent (QCloseEvent*) override;

            Base* tool;
        };






        class Base : public QFrame { NOMEMALIGN
          public:
            Base (Dock* parent);
            Window& window () const { return *Window::main; }

            static void add_commandline_options (MR::App::OptionList& options);
            virtual bool process_commandline_option (const MR::App::ParsedOption& opt);

            virtual QSize sizeHint () const override;

            void grab_focus () {
              window().tool_has_focus = this;
              window().set_cursor();
            }
            void release_focus () {
              if (window().tool_has_focus == this) {
                window().tool_has_focus = nullptr;
                window().set_cursor();
              }
            }

            class HBoxLayout : public QHBoxLayout { NOMEMALIGN
              public:
                HBoxLayout () : QHBoxLayout () { init(); }
                HBoxLayout (QWidget* parent) : QHBoxLayout (parent) { init(); }
              protected:
                void init () {
                  setSpacing (LAYOUT_SPACING);
                  setContentsMargins(LAYOUT_SPACING,LAYOUT_SPACING,LAYOUT_SPACING,LAYOUT_SPACING);
                }
            };

            class VBoxLayout : public QVBoxLayout { NOMEMALIGN
              public:
                VBoxLayout () : QVBoxLayout () { init(); }
                VBoxLayout (QWidget* parent) : QVBoxLayout (parent) { init(); }
              protected:
                void init () {
                  setSpacing (LAYOUT_SPACING);
                  setContentsMargins(LAYOUT_SPACING,LAYOUT_SPACING,LAYOUT_SPACING,LAYOUT_SPACING);
                }
            };

            class GridLayout : public QGridLayout { NOMEMALIGN
              public:
                GridLayout () : QGridLayout () { init(); }
                GridLayout (QWidget* parent) : QGridLayout (parent) { init(); }
              protected:
                void init () {
                  setSpacing (LAYOUT_SPACING);
                  setContentsMargins(LAYOUT_SPACING,LAYOUT_SPACING,LAYOUT_SPACING,LAYOUT_SPACING);
                }
            };


            class FormLayout : public QFormLayout { NOMEMALIGN
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
            void update_cursor() { window().set_cursor(); }

            void dragEnterEvent (QDragEnterEvent* event) override {
              event->acceptProposedAction();
            }
            void dragMoveEvent (QDragMoveEvent* event) override {
              event->acceptProposedAction();
            }
            void dragLeaveEvent (QDragLeaveEvent* event) override {
              event->accept();
            }
        };







        //! \cond skip

        inline Dock::~Dock () { delete tool; }



        class __Action__ : public QAction
        { NOMEMALIGN
          public:
            __Action__ (QActionGroup* parent,
                        const char* const name,
                        const char* const description,
                        int index) :
              QAction (name, parent),
              dock (nullptr) {
              setCheckable (true);
              setShortcut (tr (std::string ("Ctrl+F" + str (index)).c_str()));
              setStatusTip (tr (description));
            }

            virtual ~__Action__ () { delete dock; }

            virtual Dock* create () = 0;
            Dock* dock;
        };
        //! \endcond


        template <class T>
          Dock* create (const QString& text)
          {
            Dock* dock = new Dock (text);
            Window::main->addDockWidget (Qt::RightDockWidgetArea, dock);
            dock->tool = new T (dock);
            dock->tool->adjustSize();
            dock->setWidget (dock->tool);
            dock->setFloating (true);
            dock->show();
            return dock;
          }


        template <class T>
          class Action : public __Action__
        { NOMEMALIGN
          public:
            Action (QActionGroup* parent,
                const char* const name,
                const char* const description,
                int index) :
              __Action__ (parent, name, description, index) { }

            virtual Dock* create () {
              dock = Tool::create<T> (this->text());
              return dock;
            }
        };




      }
    }
  }
}

#endif


