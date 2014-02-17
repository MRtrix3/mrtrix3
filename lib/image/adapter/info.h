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


#ifndef __image_adapter_info_h__
#define __image_adapter_info_h__

#include "debug.h"
#include "types.h"
#include "datatype.h"
#include "math/matrix.h"

namespace MR
{
  namespace Image
  {
    namespace Adapter {

      template <class InfoType> class Info 
      {
        public:

          Info (InfoType& parent_image) :
            parent (parent_image) { }

          const std::string& name () const {
            return parent.name();
          }

          DataType datatype () const {
            return parent.datatype();
          }

          size_t ndim () const {
            return parent.ndim();
          }

          int dim (size_t axis) const {
            return parent.dim (axis);
          }

          float vox (size_t axis) const {
            return parent.vox (axis);
          }

          ssize_t stride (size_t axis) const {
            return parent.stride (axis);
          }

          const Math::Matrix<float>& transform () const {
            return parent.transform();
          }

        protected:
          InfoType& parent;
      };

    }

  }
  //! @}
}

#endif



