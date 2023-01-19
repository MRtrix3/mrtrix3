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

#ifndef __dwi_tractography_sift_proc_mask_h__
#define __dwi_tractography_sift_proc_mask_h__

#include "cmdline_option.h"
#include "image.h"

#include "algo/iterator.h"
#include "interp/linear.h"

#include "dwi/tractography/ACT/act.h"
#include "dwi/tractography/ACT/tissues.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {


        extern const App::OptionGroup SIFTModelProcMaskOption;


        void initialise_processing_mask (Image<float>&, Image<float>&, Image<float>&);


        // Private functor for performing ACT image regridding
        class ResampleFunctor
        { MEMALIGN(ResampleFunctor)

            using transform_type = Eigen::Transform<float, 3, Eigen::AffineCompact>;

          public:
            ResampleFunctor (Image<float>&, Image<float>&, Image<float>&);
            ResampleFunctor (const ResampleFunctor&);

            void operator() (const Iterator&);

          private:
            Image<float> dwi;
            std::shared_ptr<transform_type> voxel2scanner;
            Interp::Linear<Image<float>> interp_anat;
            Image<float> out;

            // Helper function for doing the regridding
            ACT::Tissues ACT2pve (const Iterator&);
        };



      }
    }
  }
}


#endif
