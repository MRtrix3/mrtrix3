/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 22/01/09.

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

#include "app.h"
#include "progressbar.h"
#include "file/config.h"
#include "gui/app.h"
#include "gui/dialog/progress.h"
#include "gui/dialog/report_exception.h"
#include "gui/dialog/dicom.h"

namespace MR
{
  namespace GUI
  {

    App::App ()
    {
      new QApplication (MR::App::argc, MR::App::argv);

      MR::ProgressInfo::display_func = Dialog::ProgressBar::display;
      MR::ProgressInfo::done_func = Dialog::ProgressBar::done;
      MR::File::Dicom::select_func = Dialog::select_dicom;
      MR::Exception::display_func = Dialog::display_exception;
    }

    App::~App () 
    {
      delete qApp;
    }


  }
}

