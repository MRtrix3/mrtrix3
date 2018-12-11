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


#include "gui/mrview/tool/igtlink.h"
#include "gui/mrview/window.h"


namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {



        IGTLink::IGTLink (Dock* parent) :
          Base (parent),
          position (0.0, 0.0, 0.0)
        {
          VBoxLayout* main_box = new VBoxLayout (this);

          start_button = new QPushButton ("Listen",this);
          start_button->setToolTip (tr ("Hide all main images"));
          start_button->setIcon (QIcon (":/hide.svg"));
          start_button->setCheckable (true);
          start_button->setChecked (false);
          connect (start_button, SIGNAL (clicked(bool)), this, SLOT (onStart (bool)));
          main_box->addWidget (start_button, 1);
          main_box->addStretch ();

          // call connect() & listen();
        }


        IGTLink::~IGTLink ()
        {
          worker.listening.store (false);
          thread.wait();
        }


        void IGTLink::onStart (bool on_off)
        {
          VAR (on_off);

          if (on_off) {
          }
          else {
          }

          window().updateGL();
        }



        void IGTLink::draw (const Projection& transform, bool, int, int)
        {
          if (start_button->isChecked())
            transform.render_crosshairs (position, { 1.0, 0.0, 0.0, 1.0 });
        }


        void IGTLink::onPositionChanged (const Eigen::Vector3f x)
        {
          position = x;
          window().updateGL();
        }






        IGTWorker::IGTWorker() :
          listening (true) {
            TRACE;
          }

        IGTWorker::~IGTWorker() {
          TRACE;
        }

        void IGTWorker::connect()
        {
          TRACE;
        }

        void IGTWorker::listen ()
        {
        }


      }
    }
  }
}






