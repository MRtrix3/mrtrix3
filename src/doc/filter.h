#ifndef __filter_base_h__
#define __filter_base_h__

#include <cassert>
#include <vector>
#include "ptr.h"
#include "math/matrix.h"
#include "image/info.h"

namespace MR
{
  namespace Image
  {
    namespace Filter
    {

      //! \addtogroup Filters
      // @{

      /*! A base class for all image filters.
       * This class is designed to provide a consistent interface for
       * image-to-image filters.
       *
       * Typical usage of a filter:
       * \code
       * Image::Data<value_type> input_data (argument[0]);
       * Image::Voxel<value_type> input_voxel(input_data);
       *
       * // Construct a filter object and define the expected input image properties
       * MyFilter filter (input_data);
       *
       * // Set any required filter parameters
       * filter.set_parameter(parameter);
       *
       * // Create an output header
       * Image::Header<value_type> output_header (input_data);
       *
       * // Given the filter parameters and expected input image properties, all filters must
       * // define the output image properties using the attributes inherited from ConstInfo.
       * // These attributes can then be set to the output image using:
       * output_header.set_info(filter);
       *
       * Image::Data<value_type> output_data (output_header, argument[1]);
       *
       * // Filter an image
       * filter (input_voxel, output_voxel);
       * \endcode
       */
      class Base : public ConstInfo
      {
        public:

          template <class InputSet>
            Base (const InputSet& D) :
              ConstInfo (D) { }

          virtual ~Base () { }

          template <class InputSet, class OutputSet>
            void operator() (const InputSet& input, OutputSet& output) {
            assert (0);
          }
      };

    //! @}
    }
  }
}

#endif

