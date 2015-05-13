/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08, David Raffelt, 08/06/2012

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

#ifndef __image_filter_dilate_h__
#define __image_filter_dilate_h__

#include "memory.h"
#include "image/buffer_scratch.h"
#include "image/copy.h"
#include "image/loop.h"
#include "image/filter/base.h"



namespace MR
{
  namespace Image
  {
    namespace Filter
    {

      /** \addtogroup Filters
        @{ */

      //! a filter to dilate a mask
      /*!
       * Typical usage:
       * \code
       * Buffer<bool> input_data (argument[0]);
       * auto input_voxel = input_data.voxel();
       *
       * Filter::Dilate dilate (input_data);
       * Header header (input_data);
       * header.info() = dilate.info();
       *
       * Buffer<bool> output_data (header, argument[1]);
       * auto output_voxel = output_data.voxel();
       * dilate (input_voxel, output_voxel);
       *
       * \endcode
       */
      class Dilate : public Base
      {

        public:
          template <class InfoType>
          Dilate (const InfoType& in) :
              Base (in),
              npass_ (1)
          {
            datatype_ = DataType::Bit;
          }


          template <class InputVoxelType, class OutputVoxelType>
          void operator() (InputVoxelType& input, OutputVoxelType& output)
          {

            std::shared_ptr <BufferScratch<bool> > in_data (new BufferScratch<bool> (input));
            std::shared_ptr <BufferScratch<bool>::voxel_type> in (new BufferScratch<bool>::voxel_type (*in_data));
            Image::copy (input, *in);

            std::shared_ptr <BufferScratch<bool> > out_data;
            std::shared_ptr <BufferScratch<bool>::voxel_type> out;

            std::shared_ptr<ProgressBar> progress (message.size() ? new ProgressBar (message, npass_ + 1) : nullptr);

            for (unsigned int pass = 0; pass < npass_; pass++) {
              out_data.reset (new BufferScratch<bool> (input));
              out.reset (new BufferScratch<bool>::voxel_type (*out_data));
              LoopInOrder loop (*in);
              for (auto l = LoopInOrder(*in) (*in, *out); l; ++l)
                out->value() = dilate (*in);
              if (pass < npass_ - 1) {
                in_data = out_data;
                in = out;
              }
              if (progress)
                ++(*progress);
            }
            Image::copy (*out, output);
          }


          void set_npass (unsigned int npass) {
            npass_ = npass;
          }


        protected:

          bool dilate (BufferScratch<bool>::voxel_type& in)
          {
            if (in.value()) return true;
            bool val;
            if (in[0] > 0) { in[0]--; val = in.value(); in[0]++; if (val) return true; }
            if (in[1] > 0) { in[1]--; val = in.value(); in[1]++; if (val) return true; }
            if (in[2] > 0) { in[2]--; val = in.value(); in[2]++; if (val) return true; }
            if (in[0] < in.dim(0)-1) { in[0]++; val = in.value(); in[0]--; if (val) return true; }
            if (in[1] < in.dim(1)-1) { in[1]++; val = in.value(); in[1]--; if (val) return true; }
            if (in[2] < in.dim(2)-1) { in[2]++; val = in.value(); in[2]--; if (val) return true; }
            return false;
          }

          unsigned int npass_;
      };
      //! @}
    }
  }
}




#endif
