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
        ::MR::ProgressBar::display_func = ::MR::GUI::Dialog::ProgressBar::display;
        ::MR::ProgressBar::done_func = ::MR::GUI::Dialog::ProgressBar::done;
        ::MR::File::Dicom::select_func = ::MR::GUI::Dialog::select_dicom;
        ::MR::Exception::display_func = ::MR::GUI::Dialog::display_exception;

        ::MR::App::check_overwrite_files_func = ::MR::GUI::Dialog::File::check_overwrite_files_func;
      }



    }
  }
}

