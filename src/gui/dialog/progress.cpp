/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include <cassert>
#include "gui/gui.h"
#include "gui/dialog/progress.h"

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {
      namespace ProgressBar
      {

        namespace {
          QProgressDialog* progress_dialog = nullptr;
        }



        void display (ProgressInfo& p)
        {
          if (!p.data) {
            INFO (MR::App::NAME + ": " + p.text);
            assert (GUI::App::main_window);
            GUI::App::main_window->setUpdatesEnabled (false);
            p.data = new Timer;
          }
          else if (reinterpret_cast<Timer*>(p.data)->elapsed() > 1.0) {
            Context::Grab context;
            if (!progress_dialog) {
              progress_dialog = new QProgressDialog ((p.text + p.ellipsis).c_str(), QString(), 0, p.multiplier ? 100 : 0, GUI::App::main_window);
              progress_dialog->setWindowModality (Qt::ApplicationModal);
              progress_dialog->show();
              qApp->processEvents();
            }
            progress_dialog->setValue (p.value);
            qApp->processEvents();
          }
        }


        void done (ProgressInfo& p)
        {
          INFO (MR::App::NAME + ": " + p.text + " [done]");
          if (p.data) {
            assert (GUI::App::main_window);
            if (progress_dialog) {
              Context::Grab context;
              delete progress_dialog;
              progress_dialog = nullptr;
            }

            GUI::App::main_window->setUpdatesEnabled (true);
          }
          p.data = nullptr;
        }

      }
    }
  }
}

