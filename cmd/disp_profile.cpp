/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 22/01/09.

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

#include "app.h"
#include "file/path.h"
#include "math/SH.h"
#include "dwi/render_window.h"

using namespace std; 
using namespace MR; 

SET_VERSION_DEFAULT;
SET_AUTHOR (NULL);
SET_COPYRIGHT (NULL);

DESCRIPTION = {
  "view spherical harmonics surface plots.",
  NULL
};

ARGUMENTS = {
  Argument ("coefs", "SH coefficients", "a text file containing the even spherical harmonics coefficients to display.", Optional).type_file (),
  Argument::End
};

OPTIONS = { 
  Option ("response", "display response function", "assume SH coefficients file only contains even, m=0 terms. Used to display the response function as produced by estimate_response"),

  Option::End 
};




class MyApp : public MR::App { 
  public: 
    MyApp (int argc, char** argv) : App (argc, argv, __command_description, __command_arguments, __command_options, 
        __command_version, __command_author, __command_copyright), qapp (argc, argv) { parse_arguments(); }

    void execute () { 
      DWI::Window window (get_options("response").size());
      if (argument.size()) window.set_values (argument[0].get_string());
      window.show();
      if (qapp.exec()) throw Exception ("error running Qt application");
    }


  protected:
    QApplication qapp;
}; 




int main (int argc, char* argv[]) 
{ 
  try { 
    MyApp app (argc, argv);  
    app.execute();
  }
  catch (Exception& E) { E.display(); return (1); }
  catch (int ret) { return (ret); } 
  return (0); 
}


