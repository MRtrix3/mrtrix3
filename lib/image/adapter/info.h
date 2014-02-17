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



