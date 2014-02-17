#include "app.h"
#include "exception.h"

namespace MR
{

  void display_exception_cmdline (const Exception& E, int log_level)
  {
    if (App::log_level >= log_level) 
      for (size_t n = 0; n < E.description.size(); ++n) 
        report_to_user_func (E.description[n], log_level);
  }


  namespace {

    inline const char* console_prefix (int type) { 
      switch (type) {
        case 0: return " [ERROR]: ";
        case 1: return " [WARNING]: ";
        case 2: return " [INFO]: ";
        case 3: return " [DEBUG]: ";
        default: return ": ";
      }
    }

  }

  void cmdline_report_to_user_func (const std::string& msg, int type)
  {
    std::cerr << App::NAME << console_prefix (type) << msg << "\n";
  }



  void cmdline_print_func (const std::string& msg)
  {
    std::cout << msg;
  }




  void (*print) (const std::string& msg) = cmdline_print_func;
  void (*report_to_user_func) (const std::string& msg, int type) = cmdline_report_to_user_func;
  void (*Exception::display_func) (const Exception& E, int log_level) = display_exception_cmdline;

}

