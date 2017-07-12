/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#include "command.h"

#include "header.h"
#include "image.h"

#include "dwi/tractography/ACT/act.h"
#include "dwi/tractography/ACT/tissues.h"


using namespace MR;
using namespace App;



#define VALUE_DEFAULT_BG   0.0
#define VALUE_DEFAULT_CGM  0.5
#define VALUE_DEFAULT_SGM  0.75
#define VALUE_DEFAULT_WM   1.0
#define VALUE_DEFAULT_CSF  0.15
#define VALUE_DEFAULT_PATH 2.0



void usage ()
{

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Generate an image for visualisation purposes from an ACT 5TT segmented anatomical image";

  ARGUMENTS
  + Argument ("input",  "the input 4D tissue-segmented image").type_image_in()
  + Argument ("output", "the output 3D image for visualisation").type_image_out();

  OPTIONS

  + Option ("bg",   "image intensity of background (default: " + str(VALUE_DEFAULT_BG, 2) + ")")
    + Argument ("value").type_float (0.0, 1.0)

  + Option ("cgm",  "image intensity of cortical grey matter (default: " + str(VALUE_DEFAULT_CGM, 2) + ")")
    + Argument ("value").type_float (0.0, 1.0)

  + Option ("sgm",  "image intensity of sub-cortical grey matter (default: " + str(VALUE_DEFAULT_SGM, 2) + ")")
    + Argument ("value").type_float (0.0, 1.0)

  + Option ("wm",   "image intensity of white matter (default: " + str(VALUE_DEFAULT_WM, 2) + ")")
    + Argument ("value").type_float (0.0, 1.0)

  + Option ("csf",  "image intensity of CSF (default: " + str(VALUE_DEFAULT_CSF, 2) + ")")
    + Argument ("value").type_float (0.0, 1.0)

  + Option ("path", "image intensity of pathological tissue (default: " + str(VALUE_DEFAULT_PATH, 2) + ")")
    + Argument ("value").type_float (0.0, 10.0);

}







void run ()
{

  auto input = Image<float>::open (argument[0]);
  DWI::Tractography::ACT::verify_5TT_image (input);

  Header H (input);
  H.ndim() = 3;

  const float bg_multiplier   = get_option_value ("bg",   VALUE_DEFAULT_BG);
  const float cgm_multiplier  = get_option_value ("cgm",  VALUE_DEFAULT_CGM);
  const float sgm_multiplier  = get_option_value ("sgm",  VALUE_DEFAULT_SGM);
  const float wm_multiplier   = get_option_value ("wm",   VALUE_DEFAULT_WM);
  const float csf_multiplier  = get_option_value ("csf",  VALUE_DEFAULT_CSF);
  const float path_multiplier = get_option_value ("path", VALUE_DEFAULT_PATH);

  auto output = Image<float>::create (argument[1], H);

  auto f = [&] (decltype(input)& in, decltype(output)& out) {
    const DWI::Tractography::ACT::Tissues t (in);
    const float bg = 1.0 - (t.get_cgm() + t.get_sgm() + t.get_wm() + t.get_csf() + t.get_path());
    out.value() = (bg_multiplier * bg) + (cgm_multiplier * t.get_cgm()) + (sgm_multiplier * t.get_sgm()) 
                + (wm_multiplier * t.get_wm()) + (csf_multiplier * t.get_csf()) + (path_multiplier * t.get_path());
  };
  ThreadedLoop (output).run (f, input, output);

}

