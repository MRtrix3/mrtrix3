/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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

