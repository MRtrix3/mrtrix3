/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


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
  AUTHOR = "J-Donald Tournier (d.tournier@brain.org.au), Dave Raffelt (david.raffelt@florey.edu.au) and Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "the MRtrix image viewer.";

  REFERENCES 
    + "Tournier, J.-D.; Calamante, F. & Connelly, A. " // Internal
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
  qApp->processEvents();

  if (MR::App::get_options ("norealign").size())
    Header::do_not_realign_transform = true;

  if (argument.size()) {
    std::vector<std::unique_ptr<MR::Header>> list;

    for (size_t n = 0; n < argument.size(); ++n) {
      try {
        list.push_back (std::unique_ptr<MR::Header> (new MR::Header (MR::Header::open (argument[n]))));
      }
      catch (Exception& e) {
        e.display();
      }
    }

    if (list.size())
      window.add_images (list);
  }

  window.process_commandline_options();

  if (qApp->exec())
    throw Exception ("error running Qt application");
}




