/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "app.h"
#include "progressbar.h"

#ifdef MRTRIX_WINDOWS
# define CLEAR_LINE_CODE "\015\033[0K"
#else
# define CLEAR_LINE_CODE "\033[0K"
#endif

namespace MR
{

  extern off_t __stderr_offset;

  namespace
  {

    const char* busy[] = {
      ".   ",
      " .  ",
      "  . ",
      "   .",
      "  . ",
      " .  "
    };



    inline void __update_progress_cmdline (const std::string& text, bool done) 
    {
      if (App::stderr_to_file) {
        if (__stderr_offset == 0 && !done)
          __stderr_offset = lseek (STDERR_FILENO, 0, SEEK_CUR);
        else
          lseek (STDERR_FILENO, __stderr_offset, SEEK_SET);
        __print_stderr (text.c_str());
        if (done)
          __stderr_offset = 0;
      }
      else {
        __stderr_offset = done ? 0 : 1;
        __print_stderr ("\r");
        __print_stderr (text.c_str());
        __print_stderr (CLEAR_LINE_CODE);
      }
    }


    void display_func_cmdline (ProgressInfo& p)
    {
      if (p.multiplier) 
        __update_progress_cmdline (printf ("%s: [%3zu%%] %s", App::NAME.c_str(), size_t (p.value), p.text.c_str()), false);
      else
        __update_progress_cmdline (printf ("%s: [%s] %s", App::NAME.c_str(), busy[p.value%6], p.text.c_str()), false);
    }


    void done_func_cmdline (ProgressInfo& p)
    {
      if (p.multiplier)
        __update_progress_cmdline (printf ("%s: [%3u%%] %s\n", App::NAME.c_str(), 100, p.text.c_str()), true);
      else
        __update_progress_cmdline (printf ("%s: [done] %s\n", App::NAME.c_str(), p.text.c_str()), true);
    }
  }

  void (*ProgressInfo::display_func) (ProgressInfo& p) = display_func_cmdline;
  void (*ProgressInfo::done_func) (ProgressInfo& p) = done_func_cmdline;


}

