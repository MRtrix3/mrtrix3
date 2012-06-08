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

#ifndef __image_filter_erode_h__
#define __image_filter_erode_h__

#include "image/loop.h"
#include "ptr.h"
#include "progressbar.h"
#include "image/buffer_scratch.h"
#include "image/copy.h"

namespace MR
{
  namespace Image
  {
    namespace Filter
    {

      /** \addtogroup Filters
        @{ */

      //! a filter to erode a mask
      /*!
       * Typical usage:
       * \code
       * Buffer<value_type> input_data (argument[0]);
       * Buffer<value_type>::voxel_type input_voxel (input_data);
       *
       * Filter::Dilate erode (input_data);
       * Header header (input_data);
       * header.info() = erode.info();
       *
       * Buffer<int> output_data (header, argument[1]);
       * Buffer<int>::voxel_type output_voxel (output_data);
       * erode (input_voxel, output_voxel);
       *
       * \endcode
       */
      class Erode : public ConstInfo
      {

        public:
          template <class InputVoxelType>
            Erode (const InputVoxelType & input) : ConstInfo (input) {
              npass_ = 1;
          }


          template <class InputVoxelType, class OutputVoxelType>
          void operator() (InputVoxelType & input, OutputVoxelType & output) {

            RefPtr <BufferScratch<float> > in_data (new BufferScratch<float> (input));
            RefPtr <BufferScratch<float>::voxel_type> in (new BufferScratch<float>::voxel_type (*in_data));
            Image::copy(input, *in);

            RefPtr <BufferScratch<float> > out_data;
            RefPtr <BufferScratch<float>::voxel_type> out;

            for (unsigned int pass = 0; pass < npass_; pass++) {
              out_data = new BufferScratch<float> (input);
              out = new BufferScratch<float>::voxel_type (*out_data);
              LoopInOrder loop (*in, "eroding (pass " + str(pass+1) + ") ...");
              for (loop.start (*in, *out); loop.ok(); loop.next(*in, *out)) {
               out->value() = erode(*in);
              }
              if (pass < npass_ - 1) {
                in_data = out_data;
                in = out;
              }
            }
            Image::copy(*out, output);
          }


          void set_npass (unsigned int npass) {
            npass_ = npass;
          }


        protected:

          float erode (BufferScratch<float>::voxel_type & in)
          {
            if (in.value() < 0.5) return (0.0);
            float val;
            if (in[0] == 0) return (0.0);
            if (in[1] == 0) return (0.0);
            if (in[2] == 0) return (0.0);
            if (in[0] == in.dim(0)-1) return (0.0);
            if (in[1] == in.dim(1)-1) return (0.0);
            if (in[2] == in.dim(2)-1) return (0.0);
            if (in[0] > 0) { in[0]--; val = in.value(); in[0]++; if (val < 0.5) return (0.0); }
            if (in[1] > 0) { in[1]--; val = in.value(); in[1]++; if (val < 0.5) return (0.0); }
            if (in[2] > 0) { in[2]--; val = in.value(); in[2]++; if (val < 0.5) return (0.0); }
            if (in[0] < in.dim(0)-1) { in[0]++; val = in.value(); in[0]--; if (val < 0.5) return (0.0); }
            if (in[1] < in.dim(1)-1) { in[1]++; val = in.value(); in[1]--; if (val < 0.5) return (0.0); }
            if (in[2] < in.dim(2)-1) { in[2]++; val = in.value(); in[2]--; if (val < 0.5) return (0.0); }
            return (1.0);
          }

          unsigned int npass_;
      };
      //! @}
    }
  }
}




#endif
