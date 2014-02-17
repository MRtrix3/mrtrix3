#include "command.h"
#include "progressbar.h"
#include "gui/init.h"
#include "file/path.h"
#include "math/SH.h"
#include "gui/shview/icons.h"
#include "gui/shview/render_window.h"


using namespace MR;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "view spherical harmonics surface plots.";

  ARGUMENTS
  + Argument ("coefs",
              "a text file containing the even spherical harmonics coefficients to display.")
  .optional()
  .type_file();

  OPTIONS
  + Option ("response",
            "assume SH coefficients file only contains even, m=0 terms. Used to "
            "display the response function as produced by estimate_response");

  REQUIRES_AT_LEAST_ONE_ARGUMENT = false;

  GUI::init();
}





void run ()
{
  GUI::DWI::Window window (get_options ("response").size());

  if (argument.size())
    window.set_values (std::string (argument[0]));

  window.show();

  if (qApp->exec())
    throw Exception ("error running Qt application");
}

