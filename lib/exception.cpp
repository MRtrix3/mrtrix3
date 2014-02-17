/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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

