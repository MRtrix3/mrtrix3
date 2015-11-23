/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2012.

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
