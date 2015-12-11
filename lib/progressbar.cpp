/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#include "app.h"
#include "progressbar.h"

#ifdef MRTRIX_WINDOWS
# define CLEAR_LINE_CODE 
#else
# define CLEAR_LINE_CODE +"\033[0K"
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
        if (!done && __stderr_offset == 0)
            __stderr_offset = ftello (stderr);
        else if (__stderr_offset)
          fseeko (stderr, __stderr_offset, SEEK_SET);
        __print_stderr ((text + (done ? "\n" : "")).c_str());
        if (done)
          __stderr_offset = 0;
      }
      else {
        __stderr_offset = done ? 0 : 1;
        __print_stderr (("\r" + text CLEAR_LINE_CODE + (done ? "\n" : "")).c_str());
      }
    }


    void display_func_cmdline (ProgressInfo& p)
    {
      if (p.multiplier) 
        __update_progress_cmdline (printf ("%s: [%3zu%%] %s%s", App::NAME.c_str(), size_t (p.value), p.text.c_str(), p.ellipsis.c_str()), false);
      else
        __update_progress_cmdline (printf ("%s: [%s] %s%s", App::NAME.c_str(), busy[p.value%6], p.text.c_str(), p.ellipsis.c_str()), false);
    }


    void done_func_cmdline (ProgressInfo& p)
    {
      if (p.multiplier)
        __update_progress_cmdline (printf ("%s: [%3u%%] %s", App::NAME.c_str(), 100, p.text.c_str()), true);
      else
        __update_progress_cmdline (printf ("%s: [done] %s", App::NAME.c_str(), p.text.c_str()), true);
    }
  }

  void (*ProgressInfo::display_func) (ProgressInfo& p) = display_func_cmdline;
  void (*ProgressInfo::done_func) (ProgressInfo& p) = done_func_cmdline;


}

