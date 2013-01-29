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
#include "image/filter/optimal_threshold.h"
#include "image/filter/median3D.h"
#include "image/filter/connected_components.h"
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
       * Filter::DWIBrainMask filter (input_data);
       * Header mask_header (input_data);
       * mask_header.info() = filter.info();
       *
       * Buffer<int> mask_data (mask_header, argument[1]);
       * Buffer<int>::voxel_type mask_voxel (mask_data);
       *
       * filter(input_voxel, mask_voxel);
       *
       * \endcode
       */
      class DWIBrainMask : public ConstInfo
      {

        public:

          template <class InputVoxelType>
            DWIBrainMask (const InputVoxelType & input) :
              ConstInfo (input) {
            axes_.resize(3);
            datatype_ = DataType::Bit;
          }


          template <class InputVoxelType, class OutputVoxelType>
            void operator() (InputVoxelType& input, Math::Matrix<float>& grad, OutputVoxelType& output) {
              typedef typename InputVoxelType::value_type value_type;

              std::vector<int> bzeros, dwis;
              DWI::guess_DW_directions (dwis, bzeros, grad);


              Info info (input);
              info.set_ndim (3);
              ProgressBar progress("computing dwi brain mask...");
              // Compute the mean b=0 and mean DWI image
              BufferScratch<value_type> b0_mean_data (info, "mean b0");
              typename BufferScratch<value_type>::voxel_type b0_mean_voxel (b0_mean_data);

              BufferScratch<value_type> dwi_mean_data (info, "mean DWI");
              typename BufferScratch<value_type>::voxel_type dwi_mean_voxel (dwi_mean_data);
              Loop loop(0, 3);
              for (loop.start (input, b0_mean_voxel, dwi_mean_voxel); loop.ok(); loop.next (input, b0_mean_voxel, dwi_mean_voxel)) {
                value_type mean = 0;
                for (size_t i = 0; i < dwis.size(); i++) {
                  input[3] = dwis[i];
                  mean += input.value();
                }
                dwi_mean_voxel.value() = mean / dwis.size();
                mean = 0;
                for (size_t i = 0; i < bzeros.size(); i++) {
                  input[3] = bzeros[i];
                  mean += input.value();
                }
                b0_mean_voxel.value() = mean / bzeros.size();
              }
              // Here we independently threshold the mean b=0 and dwi images
              OptimalThreshold threshold_filter (b0_mean_data);
              BufferScratch<int> b0_mean_mask_data (threshold_filter);
              BufferScratch<int>::voxel_type b0_mean_mask_voxel (b0_mean_mask_data);
              threshold_filter (b0_mean_voxel, b0_mean_mask_voxel);

              BufferScratch<value_type> dwi_mean_mask_data (threshold_filter);
              typename BufferScratch<value_type>::voxel_type dwi_mean_mask_voxel (dwi_mean_mask_data);
              threshold_filter (dwi_mean_voxel, dwi_mean_mask_voxel);

              for (loop.start (b0_mean_voxel, b0_mean_mask_voxel, dwi_mean_mask_voxel); 
                  loop.ok(); 
                  loop.next (b0_mean_voxel, b0_mean_mask_voxel, dwi_mean_mask_voxel)) {
                if (b0_mean_mask_voxel.value() > 0.0)
                  dwi_mean_mask_voxel.value() = 1.0;
              }
              Median3D median_filter (dwi_mean_mask_voxel);

              BufferScratch<value_type> temp_data (info, "temp image");
              typename BufferScratch<value_type>::voxel_type temp_voxel (temp_data);
              median_filter (dwi_mean_mask_voxel, temp_voxel);

              ConnectedComponents connected_filter (temp_voxel);
              connected_filter.set_largest_only (true);
              connected_filter (temp_voxel, temp_voxel);

              Loop loop_mask(0,3);
              for (loop_mask.start (temp_voxel); loop_mask.ok(); loop_mask.next (temp_voxel))
                temp_voxel.value() = !temp_voxel.value();

              connected_filter (temp_voxel, temp_voxel);

              for (loop_mask.start (temp_voxel, output); loop_mask.ok(); loop_mask.next (temp_voxel, output))
                output.value() = !temp_voxel.value();
          }
      };
      //! @}
    }
  }
}




#endif
