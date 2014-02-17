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
