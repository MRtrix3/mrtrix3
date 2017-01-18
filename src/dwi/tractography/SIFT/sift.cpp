/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */
#include "dwi/tractography/SIFT/sift.h"
#include "math/math.h"

namespace MR {
namespace DWI {
namespace Tractography {
namespace SIFT {



using namespace App;



const OptionGroup SIFTModelOption = OptionGroup ("Options affecting the SIFT model")

  + Option ("fd_scale_gm", "provide this option (in conjunction with -act) to heuristically downsize the fibre density estimates based on the presence of GM in the voxel. "
                           "This can assist in reducing tissue interface effects when using a single-tissue deconvolution algorithm")

  + Option ("no_dilate_lut", "do NOT dilate FOD lobe lookup tables; only map streamlines to FOD lobes if the precise tangent lies within the angular spread of that lobe")

  + Option ("make_null_lobes", "add an additional FOD lobe to each voxel, with zero integral, that covers all directions with zero / negative FOD amplitudes")

  + Option ("remove_untracked", "remove FOD lobes that do not have any streamline density attributed to them; "
                                "this improves filtering slightly, at the expense of longer computation time "
                                "(and you can no longer do quantitative comparisons between reconstructions if this is enabled)")

  + Option ("fd_thresh", "fibre density threshold; exclude an FOD lobe from filtering processing if its integral is less than this amount "
                         "(streamlines will still be mapped to it, but it will not contribute to the cost function or the filtering)")
    + Argument ("value").type_float (0.0, 2.0 * Math::pi);




const OptionGroup SIFTOutputOption = OptionGroup ("Options to make SIFT provide additional output files")

  + Option ("csv", "output statistics of execution per iteration to a .csv file")
    + Argument ("file").type_file_out()

  + Option ("out_mu", "output the final value of SIFT proportionality coefficient mu to a text file")
    + Argument ("file").type_file_out()

  + Option ("output_debug", "provide various output images for assessing & debugging performace etc.");




const OptionGroup SIFTTermOption = OptionGroup ("Options to control when SIFT terminates filtering")

  + Option ("term_number", "number of streamlines - continue filtering until this number of streamlines remain")
    + Argument ("value").type_integer (1)

  + Option ("term_ratio", "termination ratio - defined as the ratio between reduction in cost function, and reduction in density of streamlines.\n"
                          "Smaller values result in more streamlines being filtered out.")
    + Argument ("value").type_float (1e-6)

  + Option ("term_mu", "terminate filtering once the SIFT proportionality coefficient reaches a given value")
    + Argument ("value").type_float (0.0);





}
}
}
}


