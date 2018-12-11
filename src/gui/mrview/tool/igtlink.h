/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __gui_mrview_tool_igtlink_h__
#define __gui_mrview_tool_igtlink_h__

#include "gui/mrview/tool/base.h"
#include "gui/mrview/mode/base.h"
#include "gui/mrview/spin_box.h"
#include "gui/opengl/transformation.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {

        class IGTWorker : public QObject
        { NOMEMALIGN
          Q_OBJECT
          public:
            IGTWorker ();
            ~IGTWorker ();

            std::atomic<bool> listening;

            public slots:
              void connect();

            signals:
              void positionChanged (const Eigen::Vector3f x);

          private:
            void listen();
        };





        class IGTLink : public Base, public Mode::ModeGuiVisitor
        { MEMALIGN(IGTLink)
          Q_OBJECT
          public:
            IGTLink (Dock* parent);
            ~IGTLink ();

            void draw (const Projection& transform, bool is_3D, int axis, int slice) override;
            Eigen::Vector3f position;

          public slots:
            void onStart (bool on_off);
            void onPositionChanged (const Eigen::Vector3f x);

          private:
            QPushButton *start_button;
            QThread thread;
            IGTWorker  worker;
        };

      }
    }
  }
}

#endif





