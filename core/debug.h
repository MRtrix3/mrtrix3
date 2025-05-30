/* Copyright (c) 2008-2025 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __debug_h__
#define __debug_h__

#include <iostream>
#include <cstring>

namespace MR {
  namespace App {
    extern std::string NAME;
  }
}

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

/** \brief Prints a matrix name and in the following line its formatted value. */
#define MAT(variable) { \
  Eigen::IOFormat fmt(Eigen::FullPrecision, 0, ", ", ",\n        ", "[", "]", "\nnp.array([", "])"); \
  std::cerr << MR::App::NAME << " [" << __FILE__  << ": " << __LINE__ << "]: " << #variable << " = " << (variable.format(fmt)) << "\n"; \
}

#define VEC(variable) { \
  std::cerr << MR::App::NAME << " [" << __FILE__  << ": " << __LINE__ << "]: " << #variable << " = "; \
  for (ssize_t i=0; i<variable.size(); ++i){std::cerr << str(variable(i)) << " "; } \
  std::cerr << std::endl; \
}


/** \brief Stops execution and prints current function, file and line number.
  Remuses on user input (i.e. Return key). */
#define PAUSE { \
  std::cerr << MR::App::NAME << " [" << __FILE__  << ": " << __LINE__ << "]: paused (press any key to resume)\n"; \
  std::string __n__; std::getline (std::cin, __n__); \
}

/** @} */

#endif

