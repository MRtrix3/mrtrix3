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

#include "opengl/gl.h"
#include "mrview/mode/base.h"
#include "mrview/mode/list.h"

namespace MR {
  namespace Viewer {
    namespace Mode {

      Base::Base (Window& parent) : 
        window (parent) { 
          font_.setPointSize (0.9*font_.pointSize());

          QAction* separator = new QAction (this);
          separator->setSeparator (true);
          add_action_common (separator);

          show_image_info_action = new QAction(tr("Show &image info"), this);
          show_image_info_action->setCheckable (true);
          show_image_info_action->setShortcut (tr("H"));
          show_image_info_action->setStatusTip (tr("Show image header information"));
          show_image_info_action->setChecked (true);
          connect (show_image_info_action, SIGNAL (triggered()), this, SLOT (toggle_show_xyz()));
          add_action_common (show_image_info_action);

          show_orientation_action = new QAction(tr("Show &orientation"), this);
          show_orientation_action->setCheckable (true);
          show_orientation_action->setShortcut (tr("O"));
          show_orientation_action->setStatusTip (tr("Show image orientation labels"));
          show_orientation_action->setChecked (true);
          connect (show_orientation_action, SIGNAL (triggered()), this, SLOT (toggle_show_xyz()));
          add_action_common (show_orientation_action);

          show_position_action = new QAction(tr("Show &voxel"), this);
          show_position_action->setCheckable (true);
          show_position_action->setShortcut (tr("V"));
          show_position_action->setStatusTip (tr("Show image voxel position and value"));
          show_position_action->setChecked (true);
          connect (show_position_action, SIGNAL (triggered()), this, SLOT (toggle_show_xyz()));
          add_action_common (show_position_action);

          separator = new QAction (this);
          separator->setSeparator (true);
          add_action_common (separator);

          show_focus_action = new QAction(tr("Show &focus"), this);
          show_focus_action->setCheckable (true);
          show_focus_action->setShortcut (tr("F"));
          show_focus_action->setStatusTip (tr("Show focus with the crosshairs"));
          show_focus_action->setChecked (true);
          connect (show_focus_action, SIGNAL (triggered()), this, SLOT (toggle_show_xyz()));
          add_action_common (show_focus_action);

          reset_action = new QAction(tr("Reset &view"), this);
          reset_action->setShortcut (tr("Crtl+R"));
          reset_action->setStatusTip (tr("Reset image projection & zoom"));
          connect (reset_action, SIGNAL (triggered()), this, SLOT (reset()));
          add_action_common (reset_action);
        }


      Base::~Base () { }

      void Base::paint () { }
      void Base::updateGL () { reinterpret_cast<QGLWidget*> (window.glarea)->updateGL(); }
      void Base::reset () { }
      void Base::toggle_show_xyz () { updateGL(); }

      bool Base::mouse_click () { return (false); }
      bool Base::mouse_move () { return (false); }
      bool Base::mouse_doubleclick () { return (false); }
      bool Base::mouse_release () { return (false); }
      bool Base::mouse_wheel (float delta, Qt::Orientation orientation) { return (false); }

      Base* create (Window& parent, size_t index) 
      {
        switch (index) {
#include "mrview/mode/list.h"
          default: assert (0);
        };
        return (NULL);
      }

      const char* name (size_t index)
      {
        switch (index) {
#include "mrview/mode/list.h"
          default: return (NULL);
        };
        return (NULL);
      }

      const char* tooltip (size_t index)
      {
        switch (index) {
#include "mrview/mode/list.h"
          default: return (NULL);
        };
        return (NULL);
      }

    }
  }
}

#undef MODE


