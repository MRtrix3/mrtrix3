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
#include "image.h"

using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "";

  SYNOPSIS = "Please use the new mtlognorm command instead.";

  DESCRIPTION
   + "WARNING: there were some major issues with this method.  Please start using the new mtlognorm command instead.";

  ARGUMENTS
    + Argument ("input output", "").type_image_in().allow_multiple();

  OPTIONS
    + Option ("mask", "").required ()
    + Argument ("image").type_image_in ()

    + Option ("value", "")
    + Argument ("number").type_float ()

    + Option ("bias", "")
    + Argument ("image").type_image_out ()

    + Option ("independent", "")

    + Option ("maxiter", "")
    + Argument ("number").type_integer()

    + Option ("check", "")
    + Argument ("image").type_image_out ();
}



void run ()
{

  throw Exception ("There were some major issues with this method.  Please start using the new mtlognorm command instead.");

}

