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
        if (log_level < 2)
          report (E);
      }

    }
  }
}


