/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */


#include "gui/dialog/dialog.h"

#include "app.h"
#include "exception.h"
#include "progressbar.h"
#include "file/dicom/tree.h"

#include "gui/dialog/dicom.h"
#include "gui/dialog/file.h"
#include "gui/dialog/progress.h"
#include "gui/dialog/report_exception.h"

namespace MR
{
  namespace GUI
  {
    namespace Dialog
    {



      void init()
      {
        ::MR::ProgressInfo::display_func = ::MR::GUI::Dialog::ProgressBar::display;
        ::MR::ProgressInfo::done_func = ::MR::GUI::Dialog::ProgressBar::done;
        ::MR::File::Dicom::select_func = ::MR::GUI::Dialog::select_dicom;
        ::MR::Exception::display_func = ::MR::GUI::Dialog::display_exception;

        ::MR::App::check_overwrite_files_func = ::MR::GUI::Dialog::File::check_overwrite_files_func;
      }



    }
  }
}

