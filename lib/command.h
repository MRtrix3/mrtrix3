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

#ifndef __command_h__
#define __command_h__


#include <xmmintrin.h>
#include "project_version.h"
#include "app.h"

#define MRTRIX_UPDATED_API

#ifdef MRTRIX_AS_R_LIBRARY

extern "C" void R_main (int* cmdline_argc, char** cmdline_argv) 
{ 
  ::MR::App::build_date = __DATE__; 
#ifdef MRTRIX_PROJECT_VERSION
  ::MR::App::project_version = MRTRIX_PROJECT_VERSION;
#endif
  SET_MRTRIX_PROJECT_VERSION 
  ::MR::App::AUTHOR = "J-Donald Tournier (d.tournier@brain.org.au)"; 
  ::MR::App::DESCRIPTION.clear(); 
  ::MR::App::ARGUMENTS.clear(); 
  ::MR::App::OPTIONS.clear(); 
  try { 
    usage(); 
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
  ::MR::App::build_date = __DATE__; 
#ifdef MRTRIX_PROJECT_VERSION
  ::MR::App::project_version = MRTRIX_PROJECT_VERSION;
#endif
  try {
#ifdef __gui_app_h__
    ::MR::GUI::App app (cmdline_argc, cmdline_argv);
#else
    ::MR::App::init (cmdline_argc, cmdline_argv); 
#endif
    usage (); 
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
  return 0; 
}

#endif

#endif


