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
              E[E.num()-1].c_str(), QMessageBox::Ok, qApp->activeWindow());
          if (E.num() > 1) {
            QString text;
            for (size_t i = 0; i < E.num(); ++i) {
              text += E[i].c_str();
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


