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
#include "image/voxel.h"

#include "dwi/tractography/properties.h"
#include "dwi/tractography/roi.h"

#include "dwi/tractography/tracking/exec.h"
#include "dwi/tractography/tracking/method.h"
#include "dwi/tractography/tracking/tractography.h"

#include "dwi/tractography/algorithms/fact.h"
#include "dwi/tractography/algorithms/iFOD1.h"
#include "dwi/tractography/algorithms/nulldist.h"
#include "dwi/tractography/algorithms/sd_stream.h"
#include "dwi/tractography/algorithms/seedtest.h"
#include "dwi/tractography/algorithms/vecstream.h"
#include "dwi/tractography/algorithms/wbfact.h"

#include "dwi/tractography/seeding/seeding.h"




using namespace MR;
using namespace App;



const char* algorithms[] = { "fact", "ifod1", "nulldist", "sd_stream", "seedtest", "vecstream", "wbfact", NULL };


void usage ()
{

  AUTHOR = "Robert E. Smith (r.smith@brain.org.au) & J-Donald Tournier (jdtournier@gmail.com)";

  DESCRIPTION
  + "perform streamlines tractography.";

  ARGUMENTS
  + Argument ("source",
              "the image containing the source data. The type of data depends on the algorithm used:\n"
              "- FACT / WBFACT: the DWI image.\n"
              "- iFOD1 & SD_Stream: the SH image resulting from CSD.\n"
              "- Nulldist & SeedTest: any image (will not be used).\n"
              "- VecStream (& variants): the directions file."
             ).type_image_in()

  + Argument ("tracks", "the output file containing the tracks generated.").type_file();



  OPTIONS

  + Option ("algorithm",
            "specify the tractography algorithm to use. Valid choices are: "
              "FACT, iFOD1, Nulldist, SD_Stream, Seedtest, VecStream, WBFACT (default: iFOD1).")
    + Argument ("name").type_choice (algorithms, 2)

  + DWI::Tractography::ROIOption

  + DWI::Tractography::Tracking::TrackOption

  + DWI::Tractography::Seeding::SeedOption;

};



void run ()
{

  using namespace DWI::Tractography;
  using namespace DWI::Tractography::Tracking;
  using namespace DWI::Tractography::Algorithms;

  Properties properties;

  int algorithm = 1; // default = ifod1
  Options opt = get_options ("algorithm");
  if (opt.size()) algorithm = opt[0][0];

  load_rois (properties);

  Tracking::load_streamline_properties (properties);

  Seeding::load_tracking_seeds (properties);

  // Check validity of options -number and -maxnum; these are meaningless if seeds are number-limited
  // By over-riding the values in properties, the progress bar should still be valid
  if (properties.seeds.is_finite()) {

    if (properties["max_num_tracks"].size())
      WARN ("Overriding -number option (desired number of successful streamlines) as seeds can only provide a finite number");
    properties["max_num_tracks"] = str (properties.seeds.get_total_count());

    if (properties["max_num_attempts"].size())
      WARN ("Overriding -maxnum option (maximum number of streamline attempts) as seeds can only provide a finite number");
    properties["max_num_attempts"] = str (properties.seeds.get_total_count());

  }

  switch (algorithm) {
    case 0:
      Exec<FACT>     ::run (argument[0], argument[1], properties);
      break;
    case 1:
      Exec<iFOD1>    ::run (argument[0], argument[1], properties);
      break;
    case 2:
      Exec<NullDist> ::run (argument[0], argument[1], properties);
      break;
    case 3:
      Exec<SDStream> ::run (argument[0], argument[1], properties);
      break;
    case 4:
      Exec<Seedtest> ::run (argument[0], argument[1], properties);
      break;
    case 5:
      Exec<VecStream>::run (argument[0], argument[1], properties);
      break;
    case 6:
      Exec<WBFACT>   ::run (argument[0], argument[1], properties);
      break;
    default:
      assert (0);
  }

}

