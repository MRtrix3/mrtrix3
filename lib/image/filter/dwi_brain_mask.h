/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 8/06/2012

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

#ifndef __image_filter_dwi_brain_mask_h__
#define __image_filter_dwi_brain_mask_h__

#include "ptr.h"
#include "image/buffer.h"
#include "image/buffer_scratch.h"
#include "image/voxel.h"
#include "image/filter/base.h"
#include "image/filter/connected_components.h"
#include "image/filter/median.h"
#include "image/filter/optimal_threshold.h"
#include "image/histogram.h"
#include "image/copy.h"
#include "image/loop.h"
#include "dwi/gradient.h"
#include "progressbar.h"

namespace MR
{
  namespace Image
  {
    namespace Filter
    {


      /** \addtogroup Filters
        @{ */

      //! a filter to compute a whole brain mask from a DWI image.
      /*! Both diffusion weighted and b=0 volumes are required to
       *  obtain a mask that includes both brain tissue and CSF.
       *
       * Typical usage:
       * \code
       * Buffer<value_type> input_data (argument[0]);
       * Buffer<value_type>::voxel_type input_voxel (input_data);
       *
       * Math::Matrix<float> grad = DWI::get_valid_DW_scheme<float> (input_data);
       *
       * Filter::DWIBrainMask filter (input_data, grad);
       * Header mask_header (input_data);
       * mask_header.info() = filter.info();
       *
       * Buffer<bool> mask_data (mask_header, argument[1]);
       * Buffer<bool>::voxel_type mask_voxel (mask_data);
       *
       * filter(input_voxel, mask_voxel);
       *
       * \endcode
       */
      class DWIBrainMask : public Base
      {

        public:

          template <class InfoType>
          DWIBrainMask (const InfoType& input, const Math::Matrix<float>& grad) :
              Base (input),
              grad (grad)
          {
            axes_.resize(3);
            datatype_ = DataType::Bit;
          }


          template <class InputVoxelType, class OutputVoxelType>
          void operator() (InputVoxelType& input, OutputVoxelType& output) {
              typedef typename InputVoxelType::value_type value_type;

              Info info (input);
              info.set_ndim (3);
              LoopInOrder loop (info, 0, 3);

              // Generate a 'master' scratch buffer mask, to which all shells will contribute
              BufferScratch<bool> mask_data (info, "DWI mask");
              BufferScratch<bool>::voxel_type mask_voxel (mask_data);

              Ptr<ProgressBar> progress;
              if (message.size())
                progress = new ProgressBar (message);

              // Loop over each shell, including b=0, in turn
              DWI::Shells shells (grad);
              for (size_t s = 0; s != shells.count(); ++s) {
                const DWI::Shell shell (shells[s]);

                BufferScratch<value_type> shell_data (info, "mean b=" + str(size_t(std::round(shell.get_mean()))) + " image");
                typename BufferScratch<value_type>::voxel_type shell_voxel (shell_data);

                for (auto l = loop (input, shell_voxel); l; ++l) {
                  value_type mean = 0;
                  for (std::vector<size_t>::const_iterator v = shell.get_volumes().begin(); v != shell.get_volumes().end(); ++v) {
                    input[3] = *v;
                    mean += input.value();
                  }
                  shell_voxel.value() = mean / value_type(shell.count());
                }
                if (progress)
                  ++(*progress);

                // Threshold the mean intensity image for this shell
                OptimalThreshold threshold_filter (shell_data);
                BufferScratch<bool> shell_mask_data (threshold_filter);
                BufferScratch<bool>::voxel_type shell_mask_voxel (shell_mask_data);
                threshold_filter (shell_voxel, shell_mask_voxel);
                if (progress)
                  ++(*progress);

                // Add this mask to the master
                for (auto l = loop (mask_voxel, shell_mask_voxel); l; ++l) {
                  if (shell_mask_voxel.value())
                    mask_voxel.value() = true;
                }
                if (progress)
                  ++(*progress);

              }

              // The following operations apply to the mask as combined from all shells

              BufferScratch<bool> temp_data (info, "temporary mask");
              BufferScratch<bool>::voxel_type temp_voxel (temp_data);
              Median median_filter (mask_voxel);
              median_filter (mask_voxel, temp_voxel);
              if (progress)
                ++(*progress);

              ConnectedComponents connected_filter (temp_voxel);
              connected_filter.set_largest_only (true);
              connected_filter (temp_voxel, temp_voxel);
              if (progress)
                ++(*progress);

              for (auto l = loop (temp_voxel); l; ++l) 
                temp_voxel.value() = !temp_voxel.value();
              if (progress)
                ++(*progress);

              connected_filter (temp_voxel, temp_voxel);
              if (progress)
                ++(*progress);

              for (auto l = loop (temp_voxel, output); l; ++l) 
                output.value() = !temp_voxel.value();
          }

        protected:
          const Math::Matrix<float>& grad;

      };
      //! @}
    }
  }
}




#endif
