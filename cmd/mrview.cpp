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

#include <QApplication>

#include "app.h"
#include "progressbar.h"
#include "dialog/progress.h"
#include "dialog/report_exception.h"
#include "dialog/dicom.h"
#include "mrview/window.h"

using namespace MR; 

SET_VERSION_DEFAULT;
SET_AUTHOR (NULL);
SET_COPYRIGHT (NULL);

DESCRIPTION = {
  "the MRtrix image viewer.",
  NULL
};

ARGUMENTS = {

  Argument ("image", "an image to be loaded.")
    .optional()
    .allow_multiple()
    .type_image_in (),

  Argument()
};

OPTIONS = { Option() };




class MyApp : public MR::App { 
  public: 
    MyApp (int argc, char** argv) : App (argc, argv, __command_description, __command_arguments, __command_options, 
        __command_version, __command_author, __command_copyright), qapp (argc, argv) { 
      ProgressBar::display_func = Dialog::ProgressBar::display;
      ProgressBar::done_func = Dialog::ProgressBar::done;
      MR::File::Dicom::select_func = Dialog::select_dicom;
      parse_arguments(); 
    }

    void execute () 
    { 
      Viewer::Window window;
      window.show();

      if (argument.size()) {
        VecPtr<MR::Image::Header> list;

        for (size_t n = 0; n < argument.size(); ++n) {
          try {
            list.push_back (new Image::Header (argument[n]));
          }
          catch (Exception& e) {
            Dialog::report_exception (e, &window);
          }
        }

        if (list.size())
          window.add_images (list);
      }

      if (qapp.exec()) 
        throw Exception ("error running Qt application");
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

