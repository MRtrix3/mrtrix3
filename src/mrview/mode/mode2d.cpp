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

#include "mrtrix.h"
#include "mrview/mode/mode2d.h"

namespace MR {
  namespace Viewer {
    namespace Mode {

      Mode2D::Mode2D (Window& parent) : Base (parent) 
      { 
        axial_action = new QAction(tr("&Axial"), this);
        axial_action->setShortcut (tr("A"));
        axial_action->setStatusTip (tr("Switch to axial projection"));
        connect (axial_action, SIGNAL (triggered()), this, SLOT (axial()));
        add_action (axial_action);

        sagittal_action = new QAction(tr("&Sagittal"), this);
        sagittal_action->setShortcut (tr("S"));
        sagittal_action->setStatusTip (tr("Switch to sagittal projection"));
        connect (sagittal_action, SIGNAL (triggered()), this, SLOT (sagittal()));
        add_action (sagittal_action);

        coronal_action = new QAction(tr("&Coronal"), this);
        coronal_action->setShortcut (tr("C"));
        coronal_action->setStatusTip (tr("Switch to coronal projection"));
        connect (coronal_action, SIGNAL (triggered()), this, SLOT (coronal()));
        add_action (coronal_action);

        QAction* separator = new QAction (this);
        separator->setSeparator (true);
        add_action (separator);

        show_focus_action = new QAction(tr("Show &Focus"), this);
        show_focus_action->setShortcut (tr("F"));
        show_focus_action->setStatusTip (tr("Show focus with the crosshairs"));
        connect (show_focus_action, SIGNAL (triggered()), this, SLOT (show_focus()));
        add_action (show_focus_action);

        reset_action = new QAction(tr("Reset &View"), this);
        reset_action->setShortcut (tr("Crtl+R"));
        reset_action->setStatusTip (tr("Reset image projection & zoom"));
        connect (reset_action, SIGNAL (triggered()), this, SLOT (reset()));
        add_action (reset_action);
      }


      Mode2D::~Mode2D () { }

      void Mode2D::paint () { }

      void Mode2D::mousePressEvent (QMouseEvent* event) { }
      void Mode2D::mouseMoveEvent (QMouseEvent* event) { }
      void Mode2D::mouseDoubleClickEvent (QMouseEvent* event) { }
      void Mode2D::mouseReleaseEvent (QMouseEvent* event) { }
      void Mode2D::wheelEvent (QWheelEvent* event) { }

      void Mode2D::axial () { TEST; }
      void Mode2D::sagittal () { TEST; }
      void Mode2D::coronal () { TEST; }
      void Mode2D::show_focus () { TEST; }
      void Mode2D::reset () { TEST; }
    }
  }
}

