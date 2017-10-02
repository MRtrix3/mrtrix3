/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "app.h"
#include "progressbar.h"

// MSYS2 supports VT100, and file redirection is handled explicitly so this can be used globally
#define CLEAR_LINE_CODE "\033[0K"

namespace MR
{

  extern bool __need_newline;

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



    void display_func_terminal (ProgressInfo& p)
    {
      __need_newline = true;
      if (p.multiplier)
        __print_stderr (printf ("\r%s: [%3zu%%] %s%s" CLEAR_LINE_CODE,
              App::NAME.c_str(), p.value, p.text.c_str(), p.ellipsis.c_str()));
      else
        __print_stderr (printf ("\r%s: [%s] %s%s" CLEAR_LINE_CODE,
              App::NAME.c_str(), busy[p.value%6], p.text.c_str(), p.ellipsis.c_str()));
    }


    void done_func_terminal (ProgressInfo& p)
    {
      if (p.multiplier)
        __print_stderr (printf ("\r%s: [100%%] %s" CLEAR_LINE_CODE "\n",
              App::NAME.c_str(), p.text.c_str()));
      else
        __print_stderr (printf ("\r%s: [done] %s" CLEAR_LINE_CODE "\n",
              App::NAME.c_str(), p.text.c_str()));
      __need_newline = false;
    }







    void display_func_redirect (ProgressInfo& p)
    {
      static size_t count = 0;
      static size_t next_update_at = 0;
      // need to update whole line since text may have changed:
      if (p.text_has_been_modified) {
        __need_newline = false;
        if (p.value == 0 && p.current_val == 0)
          count = next_update_at = 0;
        if (count++ == next_update_at) {
          if (p.multiplier) {
            __print_stderr (printf ("%s: [%3zu%%] %s%s\n",
                  App::NAME.c_str(), p.value, p.text.c_str(), p.ellipsis.c_str()));;
          }
          else {
            __print_stderr (printf ("%s: [%s] %s%s\n",
                  App::NAME.c_str(), busy[p.value%6], p.text.c_str(), p.ellipsis.c_str()));
          }
          if (next_update_at)
            next_update_at *= 2;
          else
            next_update_at = 1;
        }
      }
      // text is static - can simply append to the current line:
      else {
        __need_newline = true;
        if (p.multiplier) {
          if (p.value == 0) {
            __print_stderr (printf ("%s: %s%s [",
                  App::NAME.c_str(), p.text.c_str(), p.ellipsis.c_str()));;
          }
          else if (p.value%2 == 0) {
            __print_stderr (printf ("="));
          }
        }
        else {
          if (p.value == 0) {
            __print_stderr (printf ("%s: %s%s ",
                  App::NAME.c_str(), p.text.c_str(), p.ellipsis.c_str()));;
          }
          else if (!(p.value & (p.value-1))) {
            __print_stderr (".");
          }
        }
      }
    }


    void done_func_redirect (ProgressInfo& p)
    {
      if (p.text_has_been_modified) {
        if (p.multiplier) {
          __print_stderr (printf ("%s: [100%%] %s\n", App::NAME.c_str(), p.text.c_str()));;
        }
        else {
          __print_stderr (printf ("%s: [done] %s\n", App::NAME.c_str(), p.text.c_str()));
        }
      }
      else {
        if (p.multiplier)
          __print_stderr (printf ("]\n"));
        else
          __print_stderr (printf (" done\n"));
      }
      __need_newline = false;
    }

  }


  void (*ProgressInfo::display_func) (ProgressInfo& p) = display_func_terminal;
  void (*ProgressInfo::done_func) (ProgressInfo& p) = done_func_terminal;






  bool ProgressBar::set_update_method ()
  {
    bool stderr_to_file = false;

    struct stat buf;
    if (fstat (STDERR_FILENO, &buf))
      // unable to determine nature of stderr; assuming socket
      stderr_to_file = false;
    else
      stderr_to_file = S_ISREG (buf.st_mode);



    if (stderr_to_file) {
      ProgressInfo::display_func = display_func_redirect;
      ProgressInfo::done_func = done_func_redirect;
    }
    else {
      ProgressInfo::display_func = display_func_terminal;
      ProgressInfo::done_func = done_func_terminal;
    }

    return stderr_to_file;
  }




}

