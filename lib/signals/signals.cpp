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

#include "signals/signals.h"

#include <cstdlib>
#include <iostream>
#include <signal.h>
#include <unistd.h>

#include "exception.h"
#include "signals/table.h"

#ifndef MRTRIX_NO_SIGNAL_HANDLING

namespace MR
{
  namespace Signals
  {



    void init()
    {
      // Construct the signal structure
      struct sigaction act;
      act.sa_handler = &_handler;
      sigemptyset (&act.sa_mask);
#ifdef SIGINT
      sigaddset (&act.sa_mask, SIGINT);
#endif
#ifdef SIGQUIT
      sigaddset (&act.sa_mask, SIGQUIT);
#endif
      act.sa_flags = 0;

      // Set the handler for a priori known signals only
      for (size_t i = 0; i != NSIG; ++i) {
        if (table[i][0])
          sigaction (i, &act, nullptr);
      }

      // Set function to delete temporary files if the signal handler is invoked
      std::at_quick_exit (_at_quick_exit);
    }



    void _handler (int i)
    {
      // Don't attempt to use any terminal colouring
      std::cerr << App::NAME << ": [SYSTEM FATAL CODE: " << i << "] " << table[i] << "\n";
      std::quick_exit (i);
    }



    void _at_quick_exit()
    {
      if (pipe_in.size())
        unlink (pipe_in.c_str());
      if (pipe_out.size())
        unlink (pipe_out.c_str());
    }



  }
}

#endif
