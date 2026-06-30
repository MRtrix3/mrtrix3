/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "dwi/tractography/weights.h"

#include "app.h"
#include "exception.h"
#include "math/math.h"

namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {

      using namespace App;

      const Option TrackWeightsInOption
      = Option ("tck_weights_in", "specify a text scalar file containing the streamline weights")
          + Argument ("path").type_file_in();

      const Option TrackWeightsOutOption
      = Option ("tck_weights_out", "specify the path for an output text scalar file containing streamline weights")
          + Argument ("path").type_file_out();

      void check_weights_in_nonnegative (const std::string& computation)
      {
        auto opt = get_options ("tck_weights_in");
        if (!opt.size())
          return;
        const std::string path = opt[0][0];
        const Eigen::Matrix<float, Eigen::Dynamic, 1> weights = MR::load_vector<float> (path);
        if ((weights.array() < 0.0f).any())
          throw Exception ("Streamline weights file \"" + path + "\" contains negative value(s), "
                           "which are incompatible with " + computation + "; "
                           "such a computation cannot be performed automatically in the presence of "
                           "SIFT2 differential weights, and must instead be defined explicitly by the researcher "
                           "(e.g. by separating positively- and negatively-weighted streamlines using tckedit)");
      }

    }
  }
}


