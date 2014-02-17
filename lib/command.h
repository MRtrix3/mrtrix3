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


#ifndef __command_h__
#define __command_h__


#include "project_version.h"
#include "app.h"


int main (int cmdline_argc, char** cmdline_argv) 
{ 
  MR::App::build_date = __DATE__; 
#ifdef MRTRIX_PROJECT_VERSION
  MR::App::project_version = MRTRIX_PROJECT_VERSION;
#endif
  try {
    MR::App::init (cmdline_argc, cmdline_argv); 
    usage (); 
    MR::App::parse (); 
    run (); 
  } 
  catch (MR::Exception& E) { 
    E.display(); 
    return 1;
  } 
  catch (int retval) { 
    return retval; 
  } 
  return 0; 
}

#endif


