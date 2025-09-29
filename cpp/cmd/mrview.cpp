/* Copyright (c) 2008-2025 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

// clang-format off
#include "gui.h"
#include "command.h"
// clang-format on

#include "memory.h"
#include "mrview/icons.h"
#include "mrview/mode/list.h"
#include "mrview/sync/syncmanager.h"
#include "mrview/tool/list.h"
#include "mrview/window.h"
#include "progressbar.h"

using namespace MR;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)"
           " and Dave Raffelt (david.raffelt@florey.edu.au)"
           " and Robert E. Smith (robert.smith@florey.edu.au)"
           " and Rami Tabbara (rami.tabbara@florey.edu.au)"
           " and Max Pietsch (maximilian.pietsch@kcl.ac.uk)"
           " and Thijs Dhollander (thijs.dhollander@gmail.com)";

  SYNOPSIS = "The MRtrix image viewer";

  DESCRIPTION
  + "Any images listed as arguments will be loaded and available through the image menu,"
    " with the first listed displayed initially."
    " Any subsequent command-line options will be processed"
    " as if the corresponding action had been performed through the GUI."

  + "Note that because images loaded as arguments"
    " (i.e. simply listed on the command-line)"
    " are opened before the GUI is shown,"
    " subsequent actions to be performed via the various command-line options"
    " must appear after the last argument."
    " This is to avoid confusion about which option will apply to which image."
    " If you need fine control over this,"
    " please use the -load or -select_image options."
    " For example:"
    "\"mrview -load image1.mif -interpolation 0 -load image2.mif -interpolation 0\""
    " or "
    "\"mrview image1.mif image2.mif -interpolation 0 -select_image 2 -interpolation 0\"";

  REFERENCES
    + "Tournier, J.-D.; Calamante, F. & Connelly, A. " // Internal
    "MRtrix: Diffusion tractography in crossing fiber regions. "
    "Int. J. Imaging Syst. Technol., 2012, 22, 53-66";

  ARGUMENTS
    + Argument ("image", "An image to be loaded.").optional().allow_multiple().type_image_in();

    GUI::MRView::Window::add_commandline_options(OPTIONS);

#define TOOL(classname, name, description) \
  MR::GUI::MRView::Tool::classname::add_commandline_options(OPTIONS);
  {
    using namespace MR::GUI::MRView::Tool;
#include "mrview/tool/list.h"
  }

  REQUIRES_AT_LEAST_ONE_ARGUMENT = false;

}
// clang-format on

void run() {
  GUI::App::setEventHandler([](QEvent *event) {
    if (event->type() == QEvent::FileOpen) {
      QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
      std::vector<std::unique_ptr<MR::Header>> list;
      try {
        list.push_back(std::make_unique<MR::Header>(MR::Header::open(openEvent->file().toUtf8().data())));
      } catch (Exception &E) {
        E.display();
      }
      reinterpret_cast<MR::GUI::MRView::Window *>(MR::GUI::App::main_window)->add_images(list);
    }
    return false;
  });
  Q_INIT_RESOURCE(icons);
  GUI::MRView::Window window;
  MR::GUI::MRView::Sync::SyncManager sync; // sync allows syncing between mrview windows in different processes
  window.show();
  try {
    window.parse_arguments();
  } catch (Exception &e) {
    e.display();
    return;
  }

  if (qApp->exec())
    throw Exception("error running Qt application");
}
