#include <QApplication>

#include "app.h"
#include "progressbar.h"
#include "gui/init.h"
#include "gui/mrview_icons.h"
#include "gui/mrview/window.h"
#include "gui/mrview/mode/list.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (d.tournier@brain.org.au) & Dave Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
  + "the MRtrix image viewer."

  + "Basic scripting can be performed at startup, using the -run or -batch options. "
  "Each command consists of a single line of text, with the first word interpreted "
  "as the command. Valid commands are:"


  // use this command to re-generate the command documentation:
  // grep -rh BATCH_COMMAND src/gui/ | sed -n -e 's/^.*BATCH_COMMAND \(.*\) # \(.*$\)/+ "\1\\n  \2"/p'

  + "view.mode index\n  Switch to view mode specified by the integer index. as per the view menu."
  + "view.size width,height\n  Set the size of the view area, in pixel units."
  + "view.reset\n  Reset the view according to current image. This resets the FOV, projection, and focus."
  + "view.fov num\n  Set the field of view, in mm."
  + "view.plane num\n  Set the viewing plane, according to the mappping 0: sagittal; 1: coronal; 2: axial."
  + "view.lock\n  Set whether view is locked to image axes (0: no, 1: yes)."
  + "view.fullscreen\n  Show fullscreen or windowed (0: windowed, 1: fullscreen)."
  + "image.select index\n  Switch to image number specified, with reference to the list of currently loaded images."
  + "image.load path\n  Load image specified and make it current."
  + "image.reset\n  Reset the image scaling."
  + "image.colourmap index\n  Switch the image colourmap to that specified, as per the colourmap menu."
  + "exit\n  quit MRView."
  ;

 

  ARGUMENTS
  + Argument ("image", "an image to be loaded.")
  .optional()
  .allow_multiple()
  .type_image_in ();

  OPTIONS
    + Option ("run", "run command specified in string at start time").allow_multiple()
    +   Argument("command")

    + Option ("batch", "run commands in batch script at start time").allow_multiple()
    +   Argument("file").type_file();

  REQUIRES_AT_LEAST_ONE_ARGUMENT = false;

  GUI::init ();
}




void run ()
{
  GUI::MRView::Window window;
  window.show();

  if (argument.size()) {
    VecPtr<MR::Image::Header> list;

    for (size_t n = 0; n < argument.size(); ++n) {
      try {
        list.push_back (new Image::Header (argument[n]));
      }
      catch (Exception& e) {
        e.display();
      }
    }

    if (list.size())
      window.add_images (list);
  }

  if (qApp->exec())
    throw Exception ("error running Qt application");
}




