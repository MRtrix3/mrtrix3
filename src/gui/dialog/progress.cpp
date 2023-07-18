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



        void display (const ::MR::ProgressBar& p)
        {
          if (!p.data) {
            INFO (MR::App::NAME + ": " + p.text());
            assert (GUI::App::main_window);
            GUI::App::main_window->setUpdatesEnabled (false);
            p.data = new Timer;
          }
          else if (reinterpret_cast<Timer*>(p.data)->elapsed() > 1.0) {
            GL::Context::Grab context;
            if (!progress_dialog) {
              progress_dialog = new QProgressDialog (qstr (p.text() + p.ellipsis()), QString(),
                  0, p.show_percent() ? 100 : 0, GUI::App::main_window);
              progress_dialog->setWindowModality (Qt::ApplicationModal);
              progress_dialog->show();
              qApp->processEvents();
            }
            progress_dialog->setValue (p.value());
            qApp->processEvents();
          }
        }


        void done (const ::MR::ProgressBar& p)
        {
          INFO (MR::App::NAME + ": " + p.text() + " [done]");
          if (p.data) {
            assert (GUI::App::main_window);
            if (progress_dialog) {
              GL::Context::Grab context;
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

