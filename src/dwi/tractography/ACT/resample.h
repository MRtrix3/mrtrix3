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

#ifndef __dwi_tractography_act_resample_h__
#define __dwi_tractography_act_resample_h__


#include "image.h"
#include "types.h"
#include "interp/linear.h"

#include "dwi/tractography/ACT/tissues.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace ACT
      {



        class ResampleFunctor
        {

            using transform_type = Eigen::Transform<float, 3, Eigen::AffineCompact>;

          public:
            ResampleFunctor (Image<float>&, Image<float>&);
            ResampleFunctor (const ResampleFunctor&);

            void operator() (const Iterator&);

          private:
            std::shared_ptr<transform_type> voxel2scanner;
            Interp::Linear<Image<float>> interp_anat;
            Image<float> out;

            // Helper function for doing the regridding
            Tissues ACT2pve (const Iterator&);
        };



      }
    }
  }
}


#endif
