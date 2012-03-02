/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 25/10/09.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/


#include "app.h"
#include "image/voxel.h"
#include "image/interp/linear.h"
#include "dwi/tractography/exec.h"
#include "dwi/tractography/iFOD1.h"
#include "dwi/tractography/iFOD2.h"
#include "dwi/tractography/fact.h"
#include "dwi/tractography/sd_stream.h"
#include "dwi/tractography/tractography.h"
#include "dwi/tractography/vecstream.h"
#include "dwi/tractography/wbfact.h"


MRTRIX_APPLICATION

using namespace MR;
using namespace App;

const char* algorithms[] = { "ifod1", "ifod2", "fact", "wbfact", "vecstream", "sd_stream", NULL };

void usage ()
{
  DESCRIPTION
  + "perform streamlines tracking.";

  ARGUMENTS
  + Argument ("source",
              "the image containing the source data. "
              "For iFOD1/2 and SD_STREAM, this should be the FOD file, expressed in spherical harmonics. "
              "For VecStream, this should be the directions file."
             ).type_image_in()

  + Argument ("tracks",
              "the output file containing the tracks generated."
             ).type_file();


  OPTIONS
  + Option ("algorithm",
            "specify the tractography algorithm to use. Valid choices are: iFOD1, "
            "iFOD2, FACT, WBFACT, VecStream, SD_STREAM (default: iFOD2).")
  + Argument ("name").type_choice (algorithms)

  + DWI::Tractography::TrackOption;

}




void run ()
{
  using namespace DWI::Tractography;

  Properties properties;

  int algorithm = 1;
  Options opt = get_options ("algorithm");
  if (opt.size()) algorithm = opt[0][0];

  load_streamline_properties (properties);

  Image::Header source (argument[0]);

  switch (algorithm) {
    case 0:
      Exec<iFOD1>    ::run (argument[0], argument[1], properties);
      break;
    case 1:
      Exec<iFOD2>    ::run (argument[0], argument[1], properties);
      break;
    case 2:
      Exec<FACT>     ::run (argument[0], argument[1], properties);
      break;
    case 3:
      Exec<WBFACT>   ::run (argument[0], argument[1], properties);
      break;
    case 4:
      Exec<VecStream>::run (argument[0], argument[1], properties);
      break;
    case 5:
      Exec<SDStream> ::run (argument[0], argument[1], properties);
      break;
    default:
      assert (0);
  }
}
