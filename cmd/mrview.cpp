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
  + "the MRtrix image viewer.";

  ARGUMENTS
  + Argument ("image", "an image to be loaded.")
  .optional()
  .allow_multiple()
  .type_image_in ();

  using namespace GUI::MRView::Mode;
#define MODE(classname, specifier, name, description)
#define MODE_OPTION(classname, specifier, name, description) \
  + classname::options

  OPTIONS
    + GUI::MRView::Window::options
#include "gui/mrview/mode/list.h"
    ;

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




