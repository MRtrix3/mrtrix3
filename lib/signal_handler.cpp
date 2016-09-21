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

#include "signal_handler.h"

#include <cstdlib>
#include <iostream>
#include <signal.h>

#include "exception.h"
#include "file/path.h"

#ifdef MRTRIX_WINDOWS
#define STDERR_FILENO 2
#endif

namespace MR
{



  std::vector<std::string> SignalHandler::data;
  std::atomic_flag SignalHandler::flag = ATOMIC_FLAG_INIT;



  SignalHandler::SignalHandler()
  {
#ifdef MRTRIX_WINDOWS
    // Use signal() rather than sigaction() for Windows, as the latter is not supported
    // Set the handler for a priori known signals only
#ifdef SIGALRM
    signal (SIGALRM, handler);
#endif
#ifdef SIGBUS
    signal (SIGBUS,  handler);
#endif
#ifdef SIGFPE
    signal (SIGFPE,  handler);
#endif
#ifdef SIGHUP
    signal (SIGHUP,  handler);
#endif
#ifdef SIGILL // Note: Not generated under Windows
    signal (SIGILL,  handler);
#endif
#ifdef SIGINT // Note: Not supported for any Win32 application
    signal (SIGINT,  handler);
#endif
#ifdef SIGPIPE
    signal (SIGPIPE, handler);
#endif
#ifdef SIGPWR
    signal (SIGPWR,  handler);
#endif
#ifdef SIGQUIT
    signal (SIGQUIT, handler);
#endif
#ifdef SIGSEGV
    signal (SIGSEGV, handler);
#endif
#ifdef SIGSYS
    signal (SIGSYS,  handler);
#endif
#ifdef SIGTERM // Note: Not generated under Windows
    signal (SIGTERM, handler);
#endif
#ifdef SIGXCPU
    signal (SIGXCPU, handler);
#endif
#ifdef SIGXFSZ
    signal (SIGXFSZ, handler);
#endif

#else

    // Construct the signal structure
    struct sigaction act;
    act.sa_handler = &handler;
    // Since we're _Exit()-ing for any of these signals, block them all
    sigfillset (&act.sa_mask);
    act.sa_flags = 0;

    // Set the handler for a priori known signals only
#ifdef SIGALRM
    sigaction (SIGALRM, &act, nullptr);
#endif
#ifdef SIGBUS
    sigaction (SIGBUS,  &act, nullptr);
#endif
#ifdef SIGFPE
    sigaction (SIGFPE,  &act, nullptr);
#endif
#ifdef SIGHUP
    sigaction (SIGHUP,  &act, nullptr);
#endif
#ifdef SIGILL
    sigaction (SIGILL,  &act, nullptr);
#endif
#ifdef SIGINT
    sigaction (SIGINT,  &act, nullptr);
#endif
#ifdef SIGPIPE
    sigaction (SIGPIPE, &act, nullptr);
#endif
#ifdef SIGPWR
    sigaction (SIGPWR,  &act, nullptr);
#endif
#ifdef SIGQUIT
    sigaction (SIGQUIT, &act, nullptr);
#endif
#ifdef SIGSEGV
    sigaction (SIGSEGV, &act, nullptr);
#endif
#ifdef SIGSYS
    sigaction (SIGSYS,  &act, nullptr);
#endif
#ifdef SIGTERM
    sigaction (SIGTERM, &act, nullptr);
#endif
#ifdef SIGXCPU
    sigaction (SIGXCPU, &act, nullptr);
#endif
#ifdef SIGXFSZ
    sigaction (SIGXFSZ, &act, nullptr);
#endif

#endif
  }




  void SignalHandler::operator+= (const std::string& s)
  {
    while (!flag.test_and_set(std::memory_order_seq_cst));
    data.push_back (s);
    flag.clear (std::memory_order_seq_cst);
  }

  void SignalHandler::operator-= (const std::string& s)
  {
    while (!flag.test_and_set (std::memory_order_seq_cst));
    auto i = data.begin();
    while (i != data.end()) {
      if (*i == s)
        i = data.erase (i);
      else
        ++i;
    }
    flag.clear (std::memory_order_seq_cst);
  }




  void SignalHandler::handler (int i) noexcept
  {
    // Only show one signal error message if multi-threading
    if (atomic_flag_test_and_set_explicit (&flag, std::memory_order_seq_cst)) {

      // Try to do a tempfile cleanup before printing the error, since the latter's not guaranteed to work...
      for (auto i : data) {
        // Don't use File::unlink: may throw an exception
        ::unlink (i.c_str());
      }

      // Don't use std::cerr << here: Use basic C string-handling functions and a write() call to STDERR_FILENO

#define PRINT_SIGNAL(sig,msg) \
        case sig: \
          strcat (s, #sig); \
          strcat (s, " ("); \
          snprintf (i_string, 2, "%d", sig); \
          strcat (s, i_string); \
          strcat (s, ")] "); \
          strcat (s, msg); \
          break;


      char s[128] = "\n";
      strcat (s, App::NAME.c_str());
      strcat (s, ": [SYSTEM FATAL CODE: ");
      char i_string[3]; // Max 2 digits, plus null-terminator

      // Don't attempt to use any terminal colouring
      switch (i) {
#ifdef SIGALRM
        PRINT_SIGNAL(SIGALRM,"Timer expiration");
#endif
#ifdef SIGBUS
        PRINT_SIGNAL(SIGBUS,"Bus error: Accessing invalid address (out of storage space?)");
#endif
#ifdef SIGFPE
        PRINT_SIGNAL(SIGFPE,"Floating-point arithmetic exception");
#endif
#ifdef SIGHUP
        PRINT_SIGNAL(SIGHUP,"Disconnection of terminal");
#endif
#ifdef SIGILL
        PRINT_SIGNAL(SIGILL,"Illegal instruction (corrupt binary command file?)");
#endif
#ifdef SIGINT
        PRINT_SIGNAL(SIGINT,"Program manually interrupted by terminal");
#endif
#ifdef SIGPIPE
        PRINT_SIGNAL(SIGPIPE,"Nothing on receiving end of pipe");
#endif
#ifdef SIGPWR
        PRINT_SIGNAL(SIGPWR,"Power failure restart");
#endif
#ifdef SIGQUIT
        PRINT_SIGNAL(SIGQUIT,"Received terminal quit signal");
#endif
#ifdef SIGSEGV
        PRINT_SIGNAL(SIGSEGV,"Segmentation fault: Invalid memory reference");
#endif
#ifdef SIGSYS
        PRINT_SIGNAL(SIGSYS,"Bad system call");
#endif
#ifdef SIGTERM
        PRINT_SIGNAL(SIGTERM,"Terminated by kill command");
#endif
#ifdef SIGXCPU
        PRINT_SIGNAL(SIGXCPU,"CPU time limit exceeded");
#endif
#ifdef SIGXFSZ
        PRINT_SIGNAL(SIGXFSZ,"File size limit exceeded");
#endif
        default:
          strcat (s, "?] Unknown fatal system signal");
          break;
      }
      strcat (s, "\n");
      write (STDERR_FILENO, s, sizeof(s) - 1);

      std::_Exit (i);
    }
  }



}
