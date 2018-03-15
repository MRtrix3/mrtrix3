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


#include "command.h"
#include "image.h"

#include "dwi/tractography/properties.h"
#include "dwi/tractography/roi.h"

#include "dwi/tractography/tracking/exec.h"
#include "dwi/tractography/tracking/method.h"
#include "dwi/tractography/tracking/tractography.h"

#include "dwi/tractography/ACT/act.h"
#include "dwi/tractography/MACT/mact.h"

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

  DESCRIPTION 
    + "By default, tckgen produces a fixed number of streamlines, by attempting "
      "to seed from new random locations until the target number of "
      "streamlines have been selected (in other words, after all inclusion & "
      "exclusion criteria have been applied), or the maximum number of seeds "
      "has been exceeded (by default, this is 1000× the desired number of selected "
      "streamlines). Use the -select and/or -seeds options to modify as "
      "required. See also the Seeding options section for alternative seeding "
      "strategies."

    + "Below is a list of available tracking algorithms, the input image data "
      "that they require, and a brief description of their behaviour:"

    + "- FACT: Fiber Assigned by Continuous Tracking. A deterministic algorithm that "
      "takes as input a 4D image, with 3xN volumes, where N is the maximum number "
      "of fiber orientations in a voxel. Each triplet of volumes represents a 3D "
      "vector corresponding to a fiber orientation; the length of the vector "
      "additionally indicates some measure of density or anisotropy. As streamlines "
      "move from one voxel to another, the fiber orientation most collinear with the "
      "streamline orientation is selected (i.e. there is no intra-voxel interpolation)."

    + "- iFOD1: First-order Integration over Fiber Orientation Distributions. A "
      "probabilistic algorithm that takes as input a Fiber Orientation Distribution "
      "(FOD) image represented in the Spherical Harmonic (SH) basis. At each "
      "streamline step, random samples from the local (trilinear interpolated) FOD "
      "are taken. A streamline is more probable to follow orientations where the FOD "
      "amplitude is large; but it may also rarely traverse orientations with small "
      "FOD amplitude."

    + "- iFOD2 (default): Second-order Integration over Fiber Orientation "
      "Distributions. A probabilistic algorithm that takes as input a Fiber "
      "Orientation Distribution (FOD) image represented in the Spherical Harmonic "
      "(SH) basis. Candidate streamline paths (based on short curved \"arcs\") are "
      "drawn, and the underlying (trilinear-interpolated) FOD amplitudes along those "
      "arcs are sampled. A streamline is more probable to follow a path where the FOD "
      "amplitudes along that path are large; but it may also rarely traverse "
      "orientations where the FOD amplitudes are small, as long as the amplitude "
      "remains above the FOD amplitude threshold along the entire path."

    + "- NullDist1 / NullDist2: Null Distribution tracking algorithms. These "
      "probabilistic algorithms expect as input the same image that was used when "
      "invoking the corresponding algorithm for which the null distribution is "
      "sought. These algorithms generate streamlines based on random orientation "
      "samples; that is, no image information relating to fiber orientations is used, "
      "and streamlines trajectories are determined entirely from random sampling. "
      "The NullDist2 algorithm is designed to be used in conjunction with iFOD2; "
      "NullDist1 should be used in conjunction with any first-order algorithm."

    + "- SD_STREAM: Streamlines tractography based on Spherical Deconvolution (SD). "
      "A deterministic algorithm that takes as input a Fiber Orientation Distribution "
      "(FOD) image represented in the Spherical Harmonic (SH) basis. At each "
      "streamline step, the local (trilinear-interpolated) FOD is sampled, and from "
      "the current streamline tangent orientation, a Newton optimisation on the "
      "sphere is performed in order to locate the orientation of the nearest FOD "
      "amplitude peak."

    + "- SeedTest: A dummy streamlines algorithm used for testing streamline seeding "
      "mechanisms. Any image can be used as input; the image will not be used in any "
      "way. For each seed point generated by the seeding mechanism(s), a streamline "
      "containing a single point corresponding to that seed location will be written "
      "to the output track file."

    + "- Tensor_Det: A deterministic algorithm that takes as input a 4D "
      "diffusion-weighted image (DWI) series. At each streamline step, the diffusion "
      "tensor is fitted to the local (trilinear-interpolated) diffusion data, and "
      "the streamline trajectory is determined as the principal eigenvector of that "
      "tensor."

    + "- Tensor_Prob: A probabilistic algorithm that takes as input a 4D "
      "diffusion-weighted image (DWI) series. Within each image voxel, a residual "
      "bootstrap is performed to obtain a unique realisation of the DWI data in that "
      "voxel for each streamline. These data are then sampled via trilinear "
      "interpolation at each streamline step, the diffusion tensor model is fitted, "
      "and the streamline follows the orientation of the principal eigenvector of "
      "that tensor.";

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
        "The image containing the source data. "
        "The type of image data required depends on the algorithm used (see Description section).").type_image_in()

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

  + DWI::Tractography::MACT::MACTOption

  + DWI::Tractography::Algorithms::iFOD2Option

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
  MACT::load_mact_properties (properties);

  Seeding::load_seed_mechanisms (properties);
  Seeding::load_seed_parameters (properties);

  if (algorithm == 2)
    Algorithms::load_iFOD2_options (properties);

  // Check validity of options -select and -seeds; these are meaningless if seeds are number-limited
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
