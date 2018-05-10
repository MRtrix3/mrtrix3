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


#ifndef __command_h__
#define __command_h__


#include <xmmintrin.h>
#include "command_version.h"
#include "project_version.h"
#include "app.h"

#define MRTRIX_UPDATED_API

#ifdef MRTRIX_AS_R_LIBRARY

extern "C" void R_main (int* cmdline_argc, char** cmdline_argv)
{
#ifdef MRTRIX_PROJECT_VERSION
  ::MR::App::project_version = MRTRIX_PROJECT_VERSION;
#endif
  SET_MRTRIX_PROJECT_VERSION
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
#ifdef MRTRIX_PROJECT_VERSION
  ::MR::App::project_version = MRTRIX_PROJECT_VERSION;
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


