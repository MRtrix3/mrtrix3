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

#ifndef __signal_handler_h__
#define __signal_handler_h__

#include <mutex>
#include <string>
#include <vector>

namespace MR
{



  class SignalHandler
  {
    public:
      SignalHandler();
      SignalHandler (const SignalHandler&) = delete;

      void operator+= (const std::string&);
      void operator-= (const std::string&);

    private:
      static std::vector<std::string> data;
      static std::mutex mutex;

      static void on_exit() noexcept;
      static void handler (int) noexcept;

  };



}

#endif
