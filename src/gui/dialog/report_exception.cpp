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

#include "gui/gui.h"
#include "gui/dialog/report_exception.h"

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {

      namespace
      {

        inline void report (const Exception& E)
        {
          QMessageBox dialog (QMessageBox::Critical, "MRtrix error",
              qstr (E[E.num()-1]), QMessageBox::Ok, qApp->activeWindow());
          if (E.num() > 1) {
            QString text;
            for (size_t i = 0; i < E.num(); ++i) {
              text += qstr (E[i]);
              text += "\n";
            }
            dialog.setDetailedText (text);
          }
          dialog.setEscapeButton (QMessageBox::Ok);
          dialog.setDefaultButton (QMessageBox::Ok);
          dialog.exec();
        }
      }


      void display_exception (const Exception& E, int log_level)
      {
        MR::display_exception_cmdline (E, log_level);
        if (log_level < 2)
          report (E);
      }

    }
  }
}


