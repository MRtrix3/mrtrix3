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
#include "progressbar.h"

#define PROGRESS_PRINT fprintf (stderr, 

namespace MR
{

  namespace
  {

    const char* busy[] = {
      ".    ",
      " .   ",
      "  .  ",
      "   . ",
      "    .",
      "   . ",
      "  .  ",
      " .   "
    };



    void display_func_cmdline (ProgressInfo& p)
    {
      if (p.as_percentage)
        PROGRESS_PRINT "\r%s: %s %3zu%%", App::NAME.c_str(), p.text.c_str(), size_t (p.value));
      else
        PROGRESS_PRINT "\r%s: %s %s", App::NAME.c_str(), p.text.c_str(), busy[p.value%8]);
    }


    void done_func_cmdline (ProgressInfo& p)
    {
      if (p.as_percentage)
        PROGRESS_PRINT "\r%s: %s %3u%%\n", App::NAME.c_str(), p.text.c_str(), 100);
      else
        PROGRESS_PRINT "\r%s: %s  - done\n", App::NAME.c_str(), p.text.c_str());
    }
  }

  void (*ProgressBar::display_func) (ProgressInfo& p) = display_func_cmdline;
  void (*ProgressBar::done_func) (ProgressInfo& p) = done_func_cmdline;


}

