#include "gui/gui.h"

namespace MR
{
  namespace GUI
  {

    namespace {
      QProgressDialog* progress_dialog = nullptr;
    }

    QWidget* App::main_window = nullptr;
    App* App::application = nullptr;




    void App::set_main_window (QWidget* window) {
      main_window = window;
    }





    void App::startProgressBar ()
    {
      assert (main_window);
      main_window->setUpdatesEnabled (false);
    }





    void App::displayProgressBar (void* progress_info)
    {
      assert (progress_info);
      ProgressInfo& p (*reinterpret_cast<ProgressInfo*> (progress_info));
      assert (p.data);
      GrabContext context;

      if (!progress_dialog) {
        progress_dialog = new QProgressDialog (p.text.c_str(), "Cancel", 0, p.multiplier ? 100 : 0, main_window);
        progress_dialog->setWindowModality (Qt::ApplicationModal);
        progress_dialog->show();
      }
      progress_dialog->setValue (p.value);
      qApp->processEvents();
    }






    void App::doneProgressBar ()
    {
      assert (main_window);
      if (progress_dialog) {
        GrabContext context;
        delete progress_dialog;
        progress_dialog = nullptr;
      }

      main_window->setUpdatesEnabled (true);
    }


  }
}




