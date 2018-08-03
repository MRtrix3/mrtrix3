/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "signal_handler.h"

#include <cstdlib>
#include <iostream>
#include <signal.h>
#include <atomic>
#include <vector>

#include "app.h"
#include "file/path.h"

#ifdef MRTRIX_WINDOWS
# define STDERR_FILENO 2
#endif


namespace MR
{
  namespace SignalHandler {

    namespace {
      std::vector<std::string> marked_files;
      std::atomic_flag flag = ATOMIC_FLAG_INIT;




      void handler (int i) noexcept
      {
        // Only process this once if using multi-threading:
        if (!flag.test_and_set()) {

          // Try to do a tempfile cleanup before printing the error, since the latter's not guaranteed to work...
          // Don't use File::remove: may throw an exception
          for (const auto& i : marked_files)
            std::remove (i.c_str());


          const char* sig = nullptr;
          const char* msg = nullptr;
          switch (i) {

#define __SIGNAL(SIG,MSG) case SIG: sig = #SIG; msg = MSG; break;
#include "signals.h"
#undef __SIGNAL

            default:
              sig = "UNKNOWN";
              msg = "Unknown fatal system signal";
              break;
          }

          // Don't use std::cerr << here: Use basic C string-handling functions and a write() call to STDERR_FILENO
          // Don't attempt to use any terminal colouring
          char str[256];
          str[255] = '\0';
          snprintf (str, 255, "\n%s: [SYSTEM FATAL CODE: %s (%d)] %s\n", App::NAME.c_str(), sig, i, msg);
          if (write (STDERR_FILENO, str, strnlen(str,256)) == 0)
            std::_Exit (i);
          else
            std::_Exit (i);
        }
      }

    }









    void init()
    {
#ifdef MRTRIX_WINDOWS
      // Use signal() rather than sigaction() for Windows, as the latter is not supported
# define __SIGNAL(SIG,MSG) signal (SIG, handler)
#else
      // Construct the signal structure
      struct sigaction act;
      act.sa_handler = &handler;
      // Since we're _Exit()-ing for any of these signals, block them all
      sigfillset (&act.sa_mask);
      act.sa_flags = 0;
# define __SIGNAL(SIG,MSG) sigaction (SIG, &act, nullptr)
#endif

#include "signals.h"
    }




    void mark_file_for_deletion (const std::string& s)
    {
      while (!flag.test_and_set());
      marked_files.push_back (s);
      flag.clear();
    }

    void unmark_file_for_deletion (const std::string& s)
    {
      while (!flag.test_and_set());
      auto i = marked_files.begin();
      while (i != marked_files.end()) {
        if (*i == s)
          i = marked_files.erase (i);
        else
          ++i;
      }
      flag.clear();
    }

  }
}

