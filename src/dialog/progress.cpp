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

#ifndef __dialog_progressbar_h__
#define __dialog_progressbar_h__

#include <QApplication>
#include <QProgressDialog>

#include <cassert>
#include "progressbar.h"
#include "dialog/progress.h"

namespace MR {
  namespace Dialog {
    namespace ProgressBar {

      namespace {
        QProgressDialog* dialog = NULL;
      }

      void init ()
      {
        assert (dialog == NULL);
        dialog = new QProgressDialog (MR::ProgressBar::message.c_str(), "Cancel", 0, 
            isnan (MR::ProgressBar::multiplier) ? 0 : 100);
        dialog->setWindowModality (Qt::WindowModal);
      }

      void display () { dialog->setValue (MR::ProgressBar::percent); }
      void done () { delete dialog; dialog = NULL; }

    }
  }
}

#endif


