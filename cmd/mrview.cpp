/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
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
  AUTHOR =
    "J-Donald Tournier (jdtournier@gmail.com), "
    "Dave Raffelt (david.raffelt@florey.edu.au), "
    "Robert E. Smith (robert.smith@florey.edu.au), "
    "Rami Tabbara (rami.tabbara@florey.edu.au), "
    "Max Pietsch (maximilian.pietsch@kcl.ac.uk), "
    "Thijs Dhollander (thijs.dhollander@gmail.com)";

  SYNOPSIS = "The MRtrix image viewer.";

  DESCRIPTION
  + "Any images listed as arguments will be loaded and available through the "
    "image menu, with the first listed displayed initially. Any subsequent "
    "command-line options will be processed as if the corresponding action had "
    "been performed through the GUI."

  + "Note that because images loaded as arguments (i.e. simply listed on the "
    "command-line) are opened before the GUI is shown, subsequent actions to be "
    "performed via the various command-line options must appear after the last "
    "argument. This is to avoid confusion about which option will apply to which "
    "image. If you need fine control over this, please use the -load or -select_image "
    "options. For example:"

  + "$ mrview -load image1.mif -interpolation 0 -load image2.mif -interpolation 0"

  + "or"

  + "$ mrview image1.mif image2.mif -interpolation 0 -select_image 2 -interpolation 0";

  REFERENCES
    + "Tournier, J.-D.; Calamante, F. & Connelly, A. " // Internal
    "MRtrix: Diffusion tractography in crossing fiber regions. "
    "Int. J. Imaging Syst. Technol., 2012, 22, 53-66";

  ARGUMENTS
    + Argument ("image", "An image to be loaded.")
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
  try {
    window.parse_arguments();
  }
  catch (Exception& e) {
    e.display();
    return;
  }

  if (qApp->exec())
    throw Exception ("error running Qt application");
}




