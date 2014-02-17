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

