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
#ifdef MRTRIX_SIGNAL_HANDLING
#include <signal.h>
#endif

#include "exception.h"
#include "file/path.h"

namespace MR
{



  std::vector<std::string> SignalHandler::data;
  std::mutex SignalHandler::mutex;



  SignalHandler::SignalHandler()
  {
#ifdef MRTRIX_SIGNAL_HANDLING

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
    std::lock_guard<std::mutex> lock (mutex);
    data.push_back (s);
  }

  void SignalHandler::operator-= (const std::string& s)
  {
    std::lock_guard<std::mutex> lock (mutex);
    auto i = data.begin();
    while (i != data.end()) {
      if (*i == s)
        i = data.erase (i);
      else
        ++i;
    }
  }




  void SignalHandler::handler (int i) noexcept
  {
    // Only show one signal error message if multi-threading
    std::lock_guard<std::mutex> lock (mutex);
    // Don't attempt to use any terminal colouring
    switch (i) {
#ifdef SIGALRM
      case SIGALRM:
        std::cerr << "\n" << App::NAME << ": [SYSTEM FATAL CODE: SIGALRM (" << SIGALRM << ")] Timer expiration\n";
        break;
#endif
#ifdef SIGBUS
      case SIGBUS:
        std::cerr << "\n" << App::NAME << ": [SYSTEM FATAL CODE: SIGBUS ("  << SIGBUS  << ")] Bus error: Accessing invalid address (out of storage space?)\n";
        break;
#endif
#ifdef SIGFPE
      case SIGFPE:
        std::cerr << "\n" << App::NAME << ": [SYSTEM FATAL CODE: SIGFPE ("  << SIGFPE  << ")] Floating-point arithmetic exception\n";
        break;
#endif
#ifdef SIGHUP
      case SIGHUP:
        std::cerr << "\n" << App::NAME << ": [SYSTEM FATAL CODE: SIGHUP ("  << SIGHUP  << ")] Disconnection of terminal\n";
        break;
#endif
#ifdef SIGILL
      case SIGILL:
        std::cerr << "\n" << App::NAME << ": [SYSTEM FATAL CODE: SIGILL ("  << SIGILL  << ")] Illegal instruction (corrupt binary command file?)\n";
        break;
#endif
#ifdef SIGINT
      case SIGINT:
        std::cerr << "\n" << App::NAME << ": [SYSTEM FATAL CODE: SIGINT ("  << SIGINT  << ")] Program manually interrupted by terminal\n";
        break;
#endif
#ifdef SIGPIPE
      case SIGPIPE:
        std::cerr << "\n" << App::NAME << ": [SYSTEM FATAL CODE: SIGPIPE (" << SIGPIPE << ")] Nothing on receiving end of pipe\n";
        break;
#endif
#ifdef SIGPWR
      case SIGPWR:
        std::cerr << "\n" << App::NAME << ": [SYSTEM FATAL CODE: SIGPWR ("  << SIGPWR  << ")] Power failure restart\n";
        break;
#endif
#ifdef SIGQUIT
      case SIGQUIT:
        std::cerr << "\n" << App::NAME << ": [SYSTEM FATAL CODE: SIGQUIT (" << SIGQUIT << ")] Received terminal quit signal\n";
        break;
#endif
#ifdef SIGSEGV
      case SIGSEGV:
        std::cerr << "\n" << App::NAME << ": [SYSTEM FATAL CODE: SIGSEGV (" << SIGSEGV << ")] Segmentation fault: Invalid memory reference\n";
        break;
#endif
#ifdef SIGSYS
      case SIGSYS:
        std::cerr << "\n" << App::NAME << ": [SYSTEM FATAL CODE: SIGSYS ("  << SIGSYS  << ")] Bad system call\n";
        break;
#endif
#ifdef SIGTERM
      case SIGTERM:
        std::cerr << "\n" << App::NAME << ": [SYSTEM FATAL CODE: SIGTERM (" << SIGTERM << ")] Terminated by kill command\n";
        break;
#endif
#ifdef SIGXCPU
      case SIGXCPU:
        std::cerr << "\n" << App::NAME << ": [SYSTEM FATAL CODE: SIGXCPU (" << SIGXCPU << ")] CPU time limit exceeded\n";
        break;
#endif
#ifdef SIGXFSZ
      case SIGXFSZ:
        std::cerr << "\n" << App::NAME << ": [SYSTEM FATAL CODE: SIGXFSZ (" << SIGXFSZ << ")] File size limit exceeded\n";
        break;
#endif
      default:
        std::cerr << "\n" << App::NAME << ": [SYSTEM FATAL CODE: " << i << "] Unknown system signal\n";
    }

    for (auto i : data) {
      if (Path::exists (i))
        // Don't use File::unlink: may throw an exception
        ::unlink (i.c_str());
    }
    std::_Exit (i);
  }



}
