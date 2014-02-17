/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


#ifndef __image_filter_median3D_h__
#define __image_filter_median3D_h__

#include "image/info.h"
#include "image/threaded_copy.h"
#include "image/adapter/median3D.h"

namespace MR
{
  namespace Image
  {
    namespace Filter
    {
      /** \addtogroup Filters
      @{ */

      //! Smooth images using median filtering.
      class Median3D : public ConstInfo
      {

      public:
          template <class InputVoxelType>
            Median3D (const InputVoxelType& in) :
              ConstInfo (in),
              extent_ (1,3) { }

          template <class InputVoxelType>
            Median3D (const InputVoxelType& in, const std::vector<int>& extent) :
              ConstInfo (in),
              extent_ (extent) { }

          //! Set the extent of median filtering neighbourhood in voxels.
          //! This must be set as a single value for all three dimensions
          //! or three values, one for each dimension. Default 3x3x3.
          void set_extent (const std::vector<int>& extent) {
            extent_ = extent;
          }

          template <class InputVoxelType, class OutputVoxelType>
            void operator() (InputVoxelType& in, OutputVoxelType& out, const std::string& message = "median filtering...") {
              Adapter::Median3D<InputVoxelType> median (in, extent_);
              threaded_copy_with_progress_message (message, median, out);
            }

      protected:
          std::vector<int> extent_;
      };
      //! @}
    }
  }
}


#endif
