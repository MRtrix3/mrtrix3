#include "app.h"
#include "progressbar.h"
#include "file/config.h"
#include "gui/init.h"
#include "gui/dialog/progress.h"
#include "gui/dialog/report_exception.h"
#include "gui/dialog/dicom.h"

namespace MR
{
  namespace GUI
  {

    void init ()
    {
      new QApplication (App::argc, App::argv);

      MR::ProgressBar::display_func = Dialog::ProgressBar::display;
      MR::ProgressBar::done_func = Dialog::ProgressBar::done;
      MR::File::Dicom::select_func = Dialog::select_dicom;
      MR::Exception::display_func = Dialog::display_exception;
    }

  }
}

