/*
    Copyright 2010 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 28/03/2010.

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

#include <QApplication>
#include <QMessageBox>

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
        report (E);
      }

    }
  }
}


