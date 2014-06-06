/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith and J-Donald Tournier, 2011.

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


#include "command.h"
#include "image/voxel.h"

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



const char* algorithms[] = { "fact", "ifod1", "ifod2", "nulldist", "sd_stream", "seedtest", "tensor_det", "tensor_prob", NULL };


void usage ()
{

  AUTHOR = "Robert E. Smith (r.smith@brain.org.au) & J-Donald Tournier (jdtournier@gmail.com)";

  DESCRIPTION
  + "perform streamlines tractography.";

  REFERENCES = "References based on streamlines algorithm used:\n\n"
               "FACT:\n"
               "Mori, S.; Crain, B. J.; Chacko, V. P. & van Zijl, P. C. M. "
               "Three-dimensional tracking of axonal projections in the brain by magnetic resonance imaging. "
               "Annals of Neurology, 1999, 45, 265-269\n\n"
               "iFOD1 or SD_STREAM:\n"
               "Tournier, J.-D.; Calamante, F. & Connelly, A. "
               "MRtrix: Diffusion tractography in crossing fiber regions. "
               "Int. J. Imaging Syst. Technol., 2012, 22, 53-66\n\n"
               "iFOD2:\n"
               "Tournier, J.-D.; Calamante, F. & Connelly, A. "
               "Improved probabilistic streamlines tractography by 2nd order integration over fibre orientation distributions. "
               "Proceedings of the International Society for Magnetic Resonance in Medicine, 2010, 1670\n\n"
               "Nulldist:\n"
               "Morris, D. M.; Embleton, K. V. & Parker, G. J. "
               "Probabilistic fibre tracking: Differentiation of connections from chance events. "
               "NeuroImage, 2008, 42, 1329-1339\n\n"
               "Tensor_Det:\n"
               "Basser, P. J.; Pajevic, S.; Pierpaoli, C.; Duda, J. & Aldroubi, A. "
               "In vivo fiber tractography using DT-MRI data. "
               "Magnetic Resonance in Medicine, 2000, 44, 625-632\n\n"
               "Tensor_Prob:\n"
               "Jones, D. "
               "Tractography Gone Wild: Probabilistic Fibre Tracking Using the Wild Bootstrap With Diffusion Tensor MRI. "
               "IEEE Transactions on Medical Imaging, 2008, 27, 1268-1274\n\n"

               "References based on command-line options:\n\n"
               "-rk4:\n"
               "Basser, P. J.; Pajevic, S.; Pierpaoli, C.; Duda, J. & Aldroubi, A. "
               "In vivo fiber tractography using DT-MRI data. "
               "Magnetic Resonance in Medicine, 2000, 44, 625-632\n\n"
               "-act, -backtrack, -seed_gmwmi:\n"
               "Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. "
               "Anatomically-constrained tractography: Improved diffusion MRI streamlines tractography through effective use of anatomical information. "
               "NeuroImage, 2012, 62, 1924-1938\n\n"
               "-seed_dynamic:\n"
               "Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. "
               "SIFT: Spherical-deconvolution informed filtering of tractograms. "
               "NeuroImage, 2013, 67, 298-312\n\n";

  ARGUMENTS
  + Argument ("source",
              "the image containing the source data. The type of data depends on the algorithm used:\n"
              "- FACT: the directions file (each triplet of volumes is the X,Y,Z direction of a fibre population).\n"
              "- iFOD1/2 & SD_Stream: the SH image resulting from CSD.\n"
              "- Nulldist & SeedTest: any image (will not be used).\n"
              "- TensorDet / TensorProb: the DWI image.\n"
             ).type_image_in()

  + Argument ("tracks", "the output file containing the tracks generated.").type_file();



  OPTIONS

  + Option ("algorithm",
            "specify the tractography algorithm to use. Valid choices are: "
              "FACT, iFOD1, iFOD2, Nulldist, SD_Stream, Seedtest, Tensor_Det, Tensor_Prob (default: iFOD2).")
    + Argument ("name").type_choice (algorithms, 2)

  + DWI::Tractography::ROIOption

  + DWI::Tractography::Tracking::TrackOption

  + DWI::Tractography::ACT::ACTOption

  + DWI::Tractography::Seeding::SeedOption;

};



void run ()
{

  using namespace DWI::Tractography;
  using namespace DWI::Tractography::Tracking;
  using namespace DWI::Tractography::Algorithms;

  Properties properties;

  int algorithm = 2; // default = ifod2
  Options opt = get_options ("algorithm");
  if (opt.size()) algorithm = opt[0][0];

  load_rois (properties);

  Tracking::load_streamline_properties (properties);

  ACT::load_act_properties (properties);

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
      Exec<FACT>       ::run (argument[0], argument[1], properties);
      break;
    case 1:
      Exec<iFOD1>      ::run (argument[0], argument[1], properties);
      break;
    case 2:
      Exec<iFOD2>      ::run (argument[0], argument[1], properties);
      break;
    case 3:
      Exec<NullDist>   ::run (argument[0], argument[1], properties);
      break;
    case 4:
      Exec<SDStream>   ::run (argument[0], argument[1], properties);
      break;
    case 5:
      Exec<Seedtest>   ::run (argument[0], argument[1], properties);
      break;
    case 6:
      Exec<Tensor_Det> ::run (argument[0], argument[1], properties);
      break;
    case 7:
      Exec<Tensor_Prob>::run (argument[0], argument[1], properties);
      break;
    default:
      assert (0);
  }

}

