#include "command.h"
#include "progressbar.h"
#include "gui/app.h"
#include "gui/mrview/icons.h"
#include "gui/mrview/window.h"
#include "gui/mrview/mode/list.h"


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
  // grep -rh BATCH_COMMAND src/gui/mrview/window.cpp src/gui/mrview/tool/ | sed -n -e 's/^.*BATCH_COMMAND \(.*\) # \(.*$\)/+ "\1\\n  \2"/p'

  + "view.mode index\n  Switch to view mode specified by the integer index. as per the view menu."
  + "view.size width,height\n  Set the size of the view area, in pixel units."
  + "view.reset\n  Reset the view according to current image. This resets the FOV, projection, and focus."
  + "view.fov num\n  Set the field of view, in mm."
  + "view.focus x,y,z\n  Set the position of the crosshairs in scanner coordinates, with the new position supplied as a comma-separated list of floating-point values. "
  + "view.voxel x,y,z\n  Set the position of the crosshairs in voxel coordinates, relative the image currently displayed. The new position should be supplied as a comma-separated list of floating-point values. "
  + "view.fov num\n  Set the field of view, in mm."
  + "view.plane num\n  Set the viewing plane, according to the mappping 0: sagittal; 1: coronal; 2: axial."
  + "view.lock\n  Set whether view is locked to image axes (0: no, 1: yes)."
  + "image.select index\n  Switch to image number specified, with reference to the list of currently loaded images."
  + "image.load path\n  Load image specified and make it current."
  + "image.reset\n  Reset the image scaling."
  + "image.colourmap index\n  Switch the image colourmap to that specified, as per the colourmap menu."
  + "image.range min max\n  Set the image intensity range to that specified"
  + "tool.open index\n  Start the tool specified, indexed as per the tool menu"
  + "window.position x,y\n  Set the position of the main window, in pixel units."
  + "window.fullscreen\n  Show fullscreen or windowed (0: windowed, 1: fullscreen)."
  + "exit\n  quit MRView."
  + "overlay.load path\n  Loads the specified image on the overlay tool."
  + "overlay.opacity value\n  Sets the overlay opacity to floating value [0-1]."
  + "tractography.load path\n  Load the specified tracks file into the tractography tool"
  + "capture.folder path\n  Set the output folder for the screen capture tool"
  + "capture.prefix path\n  Set the output file prefix for the screen capture tool"
  + "capture.grab\n  Start the screen capture process"
  + "fixel.load path\n  Load the specified MRtrix sparse image file (.msf) into the fixel tool"
  ;

  ARGUMENTS
    + Argument ("image", "an image to be loaded.")
    .optional()
    .allow_multiple()
    .type_image_in ();

  OPTIONS
    + Option ("run", "run command specified in string at start time").allow_multiple()
    +   Argument("command").type_text()

    + Option ("batch", "run commands in batch script at start time").allow_multiple()
    +   Argument("file").type_file_in();

  REQUIRES_AT_LEAST_ONE_ARGUMENT = false;

}




void run ()
{
  MR::App::build_date = __DATE__; 
  GUI::App app;

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




