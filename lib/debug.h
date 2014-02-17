/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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
  std::cerr << MR::App::NAME << ": " << __PRETTY_FUNCTION__ << " (" << __FILE__ << ": " << __LINE__ << ")\n"


/** \brief Prints a variable name and its value, followed by the function, file and line number. */
#define VAR(variable) \
  std::cerr << MR::App::NAME << ": " << #variable << " = " << (variable) \
  << " (" << __PRETTY_FUNCTION__ << "; " << __FILE__  << ": " << __LINE__ << ")\n"

/** \brief Stops execution and prints current function, file and line number. 
  Remuses on user input (i.e. Return key). */
#define PAUSE { \
  std::cerr << MR::App::NAME << ": paused, " << __PRETTY_FUNCTION__ << " (" << __FILE__ << ": " << __LINE__ << ")...\n"; \
  std::string __n__; std::getline (std::cin, __n__); \
}

/** @} */

#endif

