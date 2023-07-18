/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#ifndef __command_h__
#define __command_h__


#ifdef FLUSH_TO_ZERO
# include <xmmintrin.h>
#endif

#include "app.h"
#include "exec_version.h"
#ifdef MRTRIX_PROJECT
namespace MR {
  namespace App {
    void set_project_version ();
  }
}
#endif

#define MRTRIX_UPDATED_API

#ifdef MRTRIX_AS_R_LIBRARY

extern "C" void R_main (int* cmdline_argc, char** cmdline_argv)
{
  ::MR::App::set_executable_uses_mrtrix_version();
#ifdef MRTRIX_PROJECT
  ::MR::App::set_project_version();
#endif
  ::MR::App::DESCRIPTION.clear();
  ::MR::App::ARGUMENTS.clear();
  ::MR::App::OPTIONS.clear();
  try {
    usage();
    ::MR::App::verify_usage();
    ::MR::App::init (*cmdline_argc, cmdline_argv);
    ::MR::App::parse ();
    run ();
  }
  catch (MR::Exception& E) {
    E.display();
    return;
  }
  catch (int retval) {
    return;
  }
}

extern "C" void R_usage (char** output)
{
  ::MR::App::DESCRIPTION.clear();
  ::MR::App::ARGUMENTS.clear();
  ::MR::App::OPTIONS.clear();
  usage();
  std::string s = MR::App::full_usage();
  *output = new char [s.size()+1];
  strncpy(*output, s.c_str(), s.size()+1);
}

#else

int main (int cmdline_argc, char** cmdline_argv)
{
#ifdef FLUSH_TO_ZERO
  // use gcc switches: -msse -mfpmath=sse -ffast-math
  int mxcsr = _mm_getcsr ();
  // Sets denormal results from floating-point calculations to zero:
  mxcsr |= (1<<15) | (1<<11); // flush-to-zero
  // Treats denormal values used as input to floating-point instructions as zero:
  mxcsr |= (1<<6); // denormals-are-zero
  _mm_setcsr (mxcsr);
#endif
  ::MR::App::set_executable_uses_mrtrix_version();
#ifdef MRTRIX_PROJECT
  ::MR::App::set_project_version();
#endif
  try {
    ::MR::App::init (cmdline_argc, cmdline_argv);
    usage ();
    ::MR::App::verify_usage();
    ::MR::App::parse_special_options();
#ifdef __gui_app_h__
    ::MR::GUI::App app (cmdline_argc, cmdline_argv);
#endif
    ::MR::App::parse ();
    run ();
  }
  catch (::MR::Exception& E) {
    E.display();
    return 1;
  }
  catch (int retval) {
    return retval;
  }
  return ::MR::App::exit_error_code;
}

#endif

#endif


