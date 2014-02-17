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


#ifndef __image_min_max_h__
#define __image_min_max_h__

#include "image/threaded_loop.h"

namespace MR
{
  namespace Image
  {

    //! \cond skip
    namespace {
      template <typename ValueType> 
      class __MinMax {
        public:
          __MinMax (ValueType& overal_min, ValueType& overal_max) :
            overal_min (overal_min), overal_max (overal_max),
            min (std::numeric_limits<ValueType>::infinity()), 
            max (-std::numeric_limits<ValueType>::infinity()) { 
              overal_min = min;
              overal_max = max;
            }
          ~__MinMax () {
            overal_min = std::min (overal_min, min);
            overal_max = std::max (overal_max, max);
          }
          void operator() (ValueType val) {
            if (std::isfinite (val)) {
              if (val < min) min = val;
              if (val > max) max = val;
            }
          }

          ValueType& overal_min;
          ValueType& overal_max;
          ValueType min, max;
      };
    }
    //! \endcond

    template <class InputVoxelType>
      inline void min_max (
          InputVoxelType& in, 
          typename InputVoxelType::value_type& min, 
          typename InputVoxelType::value_type& max, 
          size_t from_axis = 0, 
          size_t to_axis = std::numeric_limits<size_t>::max())
    {
      Image::ThreadedLoop ("finding min/max of \"" + shorten (in.name()) + "\"...", in)
        .run_foreach (__MinMax<typename InputVoxelType::value_type> (min, max), in, Input());
    }

  }
}

#endif
