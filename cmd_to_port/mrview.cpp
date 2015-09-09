#include "gui/gui.h"
#include "command.h"
#include "progressbar.h"
#include "memory.h"
#include "gui/mrview/icons.h"
#include "gui/mrview/window.h"
#include "gui/mrview/mode/list.h"
#include "gui/mrview/tool/list.h"


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (d.tournier@brain.org.au), Dave Raffelt (d.raffelt@brain.org.au) and Robert E. Smith (r.smith@brain.org.au)";

  DESCRIPTION
  + "the MRtrix image viewer.";

  REFERENCES 
    + "Tournier, J.-D.; Calamante, F. & Connelly, A. "
    "MRtrix: Diffusion tractography in crossing fiber regions. "
    "Int. J. Imaging Syst. Technol., 2012, 22, 53-66";

  ARGUMENTS
    + Argument ("image", "an image to be loaded.")
    .optional()
    .allow_multiple()
    .type_image_in ();

  GUI::MRView::Window::add_commandline_options (OPTIONS);

#define TOOL(classname, name, description) \
  MR::GUI::MRView::Tool::classname::add_commandline_options (OPTIONS);
  {
    using namespace MR::GUI::MRView::Tool;
#include "gui/mrview/tool/list.h"
  }

  REQUIRES_AT_LEAST_ONE_ARGUMENT = false;

}




void run ()
{
  GUI::MRView::Window window;
  window.show();

  if (argument.size()) {
    std::vector<std::unique_ptr<MR::Image::Header>> list;

    for (size_t n = 0; n < argument.size(); ++n) {
      try {
        list.push_back (std::unique_ptr<MR::Image::Header> (new Image::Header (argument[n])));
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




