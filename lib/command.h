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


