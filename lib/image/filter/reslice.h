#ifndef __image_interp_reslice_h__
#define __image_interp_reslice_h__

#include "image/adapter/reslice.h"
#include "image/threaded_copy.h"
#include "datatype.h"

namespace MR
{
  namespace Image
  {
    namespace Filter
    {

      //! convenience function to regrid one DataSet onto another
      /*! This function resamples (regrids) the Image \a source onto the
       * Image& \a destination, using the templated interpolator class.
       *
       * A linear transformation can be optionally applied (that maps from the destination to the source)
       *
       * For example:
       * \code
       * // source and destination data:
       * Image::Header source_header (...);
       * Image::Voxel<float> source (source_header);
       *
       * Image::Header destination_header (...);
       * Image::Voxel<float> destination (destination_header);
       *
       * // regrid source onto destination using linear interpolation:
       * Image::Filter::reslice<Image::Interp::Linear> (source, destination, operation);
       * DataSet::Interp::reslice<DataSet::Interp::Linear> (destination, source);
       * \endcode
       */
      template <template <class VoxelType> class Interpolator, class VoxelTypeDestination, class VoxelTypeSource>
        void reslice (
            VoxelTypeSource& source,
            VoxelTypeDestination& destination,
            const Math::Matrix<float>& transform = Adapter::NoTransform,
            const std::vector<int>& oversampling = Adapter::AutoOverSample,
            const typename VoxelTypeDestination::value_type value_when_out_of_bounds = DataType::default_out_of_bounds_value<typename VoxelTypeDestination::value_type>())
        {
          Adapter::Reslice<Interpolator,VoxelTypeSource> interp (source, destination, transform, oversampling, value_when_out_of_bounds);
          Image::threaded_copy_with_progress_message ("reslicing \"" + source.name() + "\"...", interp, destination, 2);
        }


      //! @}
    }
  }
}

#endif




