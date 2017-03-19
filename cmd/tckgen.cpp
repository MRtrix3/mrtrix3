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


#include "command.h"
#include "image.h"

#include "dwi/tractography/properties.h"
#include "dwi/tractography/roi.h"

#include "dwi/tractography/tracking/exec.h"
#include "dwi/tractography/tracking/method.h"
#include "dwi/tractography/tracking/tractography.h"

#include "dwi/tractography/ACT/act.h"

#include "dwi/tractography/algorithms/fact.h"
#include "dwi/tractography/algorithms/iFOD1.h"
#include "dwi/tractography/algorithms/iFOD2.h"
#include "dwi/tractography/algorithms/nulldist.h"
#include "dwi/tractography/algorithms/sd_stream.h"
#include "dwi/tractography/algorithms/seedtest.h"
#include "dwi/tractography/algorithms/tensor_det.h"
#include "dwi/tractography/algorithms/tensor_prob.h"

#include "dwi/tractography/seeding/seeding.h"




using namespace MR;
using namespace App;



const char* algorithms[] = { "fact", "ifod1", "ifod2", "nulldist1", "nulldist2", "sd_stream", "seedtest", "tensor_det", "tensor_prob", nullptr };


void usage ()
{

  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com) and Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Perform streamlines tractography";

  REFERENCES 
   + "References based on streamlines algorithm used:"

   + "* FACT:\n"
   "Mori, S.; Crain, B. J.; Chacko, V. P. & van Zijl, P. C. M. "
   "Three-dimensional tracking of axonal projections in the brain by magnetic resonance imaging. "
   "Annals of Neurology, 1999, 45, 265-269"

   + "* iFOD1 or SD_STREAM:\n"
   "Tournier, J.-D.; Calamante, F. & Connelly, A. " // Internal
   "MRtrix: Diffusion tractography in crossing fiber regions. "
   "Int. J. Imaging Syst. Technol., 2012, 22, 53-66"

   + "* iFOD2:\n"
   "Tournier, J.-D.; Calamante, F. & Connelly, A. " // Internal
   "Improved probabilistic streamlines tractography by 2nd order integration over fibre orientation distributions. "
   "Proceedings of the International Society for Magnetic Resonance in Medicine, 2010, 1670"

   + "* Nulldist1 / Nulldist2:\n"
   "Morris, D. M.; Embleton, K. V. & Parker, G. J. "
   "Probabilistic fibre tracking: Differentiation of connections from chance events. "
   "NeuroImage, 2008, 42, 1329-1339"

   + "* Tensor_Det:\n"
   "Basser, P. J.; Pajevic, S.; Pierpaoli, C.; Duda, J. & Aldroubi, A. "
   "In vivo fiber tractography using DT-MRI data. "
   "Magnetic Resonance in Medicine, 2000, 44, 625-632"

   + "* Tensor_Prob:\n"
   "Jones, D. "
   "Tractography Gone Wild: Probabilistic Fibre Tracking Using the Wild Bootstrap With Diffusion Tensor MRI. "
   "IEEE Transactions on Medical Imaging, 2008, 27, 1268-1274"

   + "References based on command-line options:"

   + "* -rk4:\n"
   "Basser, P. J.; Pajevic, S.; Pierpaoli, C.; Duda, J. & Aldroubi, A. "
   "In vivo fiber tractography using DT-MRI data. "
   "Magnetic Resonance in Medicine, 2000, 44, 625-632"

   + "* -act, -backtrack, -seed_gmwmi:\n"
   "Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. " // Internal
   "Anatomically-constrained tractography: Improved diffusion MRI streamlines tractography through effective use of anatomical information. "
   "NeuroImage, 2012, 62, 1924-1938"

   + "* -seed_dynamic:\n"
   "Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. " // Internal
   "SIFT2: Enabling dense quantitative assessment of brain white matter connectivity using streamlines tractography. "
   "NeuroImage, 2015, 119, 338-351";

  ARGUMENTS
    + Argument ("source",
        "the image containing the source data. The type of data depends on the algorithm used:\n"
        "- FACT: the directions file (each triplet of volumes is the X,Y,Z direction of a fibre population).\n"
        "- iFOD1/2, Nulldist2 & SD_Stream: the SH image resulting from CSD.\n"
        "- Nulldist1 & SeedTest: any image (will not be used).\n"
        "- Tensor_Det / Tensor_Prob: the DWI image.\n"
        ).type_image_in()

    + Argument ("tracks", "the output file containing the tracks generated.").type_tracks_out();



  OPTIONS

  + Option ("algorithm",
            "specify the tractography algorithm to use. Valid choices are: "
              "FACT, iFOD1, iFOD2, Nulldist1, Nulldist2, SD_Stream, Seedtest, Tensor_Det, Tensor_Prob (default: iFOD2).")
    + Argument ("name").type_choice (algorithms)

  + DWI::Tractography::Tracking::TrackOption

  + DWI::Tractography::Seeding::SeedMechanismOption

  + DWI::Tractography::Seeding::SeedParameterOption

  + DWI::Tractography::ROIOption

  + DWI::Tractography::ACT::ACTOption

  + DWI::GradImportOptions();

}



void run ()
{

  using namespace DWI::Tractography;
  using namespace DWI::Tractography::Tracking;
  using namespace DWI::Tractography::Algorithms;

  Properties properties;

  int algorithm = 2; // default = ifod2
  auto opt = get_options ("algorithm");
  if (opt.size()) algorithm = opt[0][0];

  load_rois (properties);

  Tracking::load_streamline_properties (properties);

  ACT::load_act_properties (properties);

  Seeding::load_seed_mechanisms (properties);
  Seeding::load_seed_parameters (properties);

  // Check validity of options -number and -maxnum; these are meaningless if seeds are number-limited
  // By over-riding the values in properties, the progress bar should still be valid
  if (properties.seeds.is_finite()) {

    if (properties["max_num_tracks"].size())
      WARN ("Overriding -select option (desired number of successful streamline selections), as seeds can only provide a finite number");
    properties["max_num_tracks"] = str (properties.seeds.get_total_count());

    if (properties["max_num_seeds"].size())
      WARN ("Overriding -seeds option (maximum number of seeds that will be attempted to track from), as seeds can only provide a finite number");
    properties["max_num_seeds"] = str (properties.seeds.get_total_count());

  }

  switch (algorithm) {
    case 0:
      Exec<FACT>       ::run (argument[0], argument[1], properties);
      break;
    case 1:
      Exec<iFOD1>      ::run (argument[0], argument[1], properties);
      break;
    case 2:
      Exec<iFOD2>      ::run (argument[0], argument[1], properties);
      break;
    case 3:
      Exec<NullDist1>  ::run (argument[0], argument[1], properties);
      break;
    case 4:
      Exec<NullDist2>  ::run (argument[0], argument[1], properties);
      break;
    case 5:
      Exec<SDStream>   ::run (argument[0], argument[1], properties);
      break;
    case 6:
      Exec<Seedtest>   ::run (argument[0], argument[1], properties);
      break;
    case 7:
      Exec<Tensor_Det> ::run (argument[0], argument[1], properties);
      break;
    case 8:
      Exec<Tensor_Prob>::run (argument[0], argument[1], properties);
      break;
    default:
      assert (0);
  }

}

