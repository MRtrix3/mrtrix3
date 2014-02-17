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

