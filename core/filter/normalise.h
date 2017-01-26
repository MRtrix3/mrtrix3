/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __image_filter_normalise3D_h__
#define __image_filter_normalise3D_h__

#include "image.h"
#include "algo/threaded_copy.h"
#include "adapter/normalise3D.h"
#include "filter/base.h"

namespace MR
{
  namespace Filter
  {
    /** \addtogroup Filters
    @{ */

    /*! Smooth images using normalise filtering.
     *
     * Typical usage:
     * \code
     * auto input = Image<float>::open (argument[0]);
     * Filter::Normalise normalise_filter (input);
     * auto output = Image<float>::create (argument[1], normalise_filter);
     * normalise_filter (input, output);
     *
     * \endcode
     */
    class Normalise : public Base { MEMALIGN(Normalise)

      public:
        template <class HeaderType>
        Normalise (const HeaderType& in) :
            Base (in),
            extent (1,3) {
          datatype() = DataType::Float32;
        }

        template <class HeaderType>
          Normalise (const HeaderType& in, const std::string& message) :
            Base (in, message),
            extent (1,3) { 
              datatype() = DataType::Float32;
            }

        template <class HeaderType>
        Normalise (const HeaderType& in, const vector<int>& extent) :
            Base (in),
            extent (extent) {
          datatype() = DataType::Float32;
        }

        template <class HeaderType>
          Normalise (const HeaderType& in, const std::string& message, const vector<int>& extent) :
            Base (in, message),
            extent (extent) { 
              datatype() = DataType::Float32;
            }

        //! Set the extent of normalise filtering neighbourhood in voxels.
        //! This must be set as a single value for all three dimensions
        //! or three values, one for each dimension. Default 3x3x3.
        void set_extent (const vector<int>& ext) {
          for (size_t i = 0; i < ext.size(); ++i) {
            if (!(ext[i] & int (1)))
              throw Exception ("expected odd number for extent");
            if (ext[i] < 0)
              throw Exception ("the kernel extent must be positive");
          }
          extent = ext;
        }

        template <class InputImageType, class OutputImageType>
        void operator() (InputImageType& in, OutputImageType& out) {
          Adapter::Normalise3D<InputImageType> normalise (in, extent);
          if (message.size())
            threaded_copy_with_progress_message (message, normalise, out);
          else
            threaded_copy (normalise, out);
        }

    protected:
        vector<int> extent;
    };
    //! @}
  }
}


#endif
