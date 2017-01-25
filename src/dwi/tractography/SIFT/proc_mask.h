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
        {
            typedef Eigen::Transform<float,3,Eigen::AffineCompact> transform_type;
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
