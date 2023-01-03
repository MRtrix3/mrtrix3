/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
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


        class CameraInteractor
        { NOMEMALIGN
          public:
            CameraInteractor () : _active (false) { }
            bool active () const { return _active; }
            virtual void deactivate ();
            virtual bool slice_move_event (const ModelViewProjection& projection, float inc);
            virtual bool pan_event (const ModelViewProjection& projection);
            virtual bool panthrough_event (const ModelViewProjection& projection);
            virtual bool tilt_event (const ModelViewProjection& projection);
            virtual bool rotate_event (const ModelViewProjection& projection);
          protected:
            bool _active;
            void set_active (bool onoff) { _active = onoff; }
        };



        class Dock : public QDockWidget
        { NOMEMALIGN
          public:
            Dock (const QString& name, bool floating) :
              QDockWidget (name, Window::main), tool (nullptr) {
                Window::main->addDockWidget (Qt::RightDockWidgetArea, this);
                setFloating (floating);
              }
            ~Dock ();

            void closeEvent (QCloseEvent*) override;

            Base* tool;
        };






        class Base : public QFrame { NOMEMALIGN
          public:
            Base (Dock* parent);
            Window& window () const { return *Window::main; }

            std::string current_folder;

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

            virtual Dock* create (bool floating) = 0;
            Dock* dock;
        };
        //! \endcond


        template <class T>
          Dock* create (const QString& text, bool floating)
          {
            Dock* dock = new Dock (text, floating);
            dock->tool = new T (dock);
            dock->setWidget (dock->tool);
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

            virtual Dock* create (bool floating) {
              dock = Tool::create<T> (this->text(), floating);
              return dock;
            }
        };




      }
    }
  }
}

#endif


