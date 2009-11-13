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

#include <gtkmm/main.h>
#include <gtk/gtkgl.h>

#include "app.h"
#include "mrview/window.h"

using namespace std; 
using namespace MR; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "the MRtrix image viewer.",
  NULL
};

ARGUMENTS = {
  Argument ("image", "an image specifier", "an image to be loaded.", false, true).type_image_in (),
  Argument::End
};

OPTIONS = {
  Option::End
};

class MyApp : public MR::App { 
  public: 
    MyApp (int argc, char** argv) : 
      App (argc, argv, __command_description, __command_arguments, __command_options, __command_version, __command_author, __command_copyright) { } 
    void execute () { }
    void init () { parse_arguments(); }
    std::vector<ArgBase>& get_args ()  { return (argument); }
}; 


int main (int argc, char* argv[]) 
{ 
  try { 
    MyApp app (argc, argv);  

    Gtk::Main kit (&argc, &argv);
    gtk_gl_init (&argc, &argv);
    app.init();

    Viewer::Window window (app.get_args());
    Gtk::Main::run (window);
  }
  catch (Exception) { return (1); } 
  catch (int ret) { return (ret); } 
  return (0); 
}





