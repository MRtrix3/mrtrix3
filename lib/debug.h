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


#ifndef __debug_h__
#define __debug_h__

#include "mrtrix.h"
#include "app.h"

/** \defgroup debug Debugging 
 * \brief functions and macros provided to ease debugging. */

/** \addtogroup debug
 * @{ */

/** \brief Prints the current function, file and line number. */
#define TRACE \
  std::cerr << MR::App::NAME << ": at " << __FILE__ << ": " << __LINE__ << "\n";


/** \brief Prints a variable name and its value, followed by the function, file and line number. */
#define VAR(variable) \
  std::cerr << MR::App::NAME << " [" << __FILE__  << ": " << __LINE__ << "]: " << #variable << " = " << (variable) << "\n";


/** \brief Stops execution and prints current function, file and line number. 
  Remuses on user input (i.e. Return key). */
#define PAUSE { \
  std::cerr << MR::App::NAME << " [" << __FILE__  << ": " << __LINE__ << "]: paused (press any key to resume)\n"; \
  std::string __n__; std::getline (std::cin, __n__); \
}

/** @} */

#endif

