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





    void App::displayProgressBar (QString text, int value, bool bounded)
    {
      assert (progress_info);
      GrabContext context;

      if (!progress_dialog) {
        progress_dialog = new QProgressDialog (text, "Cancel", 0, bounded ? 100 : 0, main_window);
        progress_dialog->setWindowModality (Qt::ApplicationModal);
        progress_dialog->show();
        qApp->processEvents();
      }
      progress_dialog->setValue (value);
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




