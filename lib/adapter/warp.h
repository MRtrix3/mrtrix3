/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __adapter_warp_h__
#define __adapter_warp_h__

#include "image.h"
#include "transform.h"
#include "interp/cubic.h"

namespace MR
{
  namespace Adapter
  {

    //! \addtogroup interp
    // @{

    //! an Image providing interpolated values from another Image
    /*! the Warp class provides an Image interface to data
     * interpolated after transformation with the input warp (supplied as a deformation field).
     *
     * For example:
     * \code
     * // reference header:
     * auto warp = Header::open (argument[0]);
     * // input data to be resliced:
     * auto input = Image<float>::open (argument[1]);
     * auto output = Image<float>::create (argument[2]);
     * threaded_copy (interp, output);
     *
     * \endcode
     *
     *
     * \sa Interp::warp()
     */
    template <template <class ImageType> class Interpolator, class ImageType, class WarpType>
      class Warp :
        public ImageBase<Warp<Interpolator,ImageType,WarpType>, typename ImageType::value_type>
    {
      public:
        typedef typename ImageType::value_type value_type;

          Warp (const ImageType& original,
                const WarpType& warp,
                const value_type value_when_out_of_bounds = Interpolator<ImageType>::default_out_of_bounds_value()) :
            interp (original, value_when_out_of_bounds),
            warp (warp),
            x { 0, 0, 0 },
            dim { warp.size(0), warp.size(1), warp.size(2) },
            vox { warp.spacing(0), warp.spacing(1), warp.spacing(2) },
            value_when_out_of_bounds (value_when_out_of_bounds) {
              assert (warp.ndim() == 4);
              assert (warp.size(3) == 3);
            }


        size_t ndim () const { return interp.ndim(); }
        bool valid () const { return interp.valid(); }
        int size (size_t axis) const { return axis < 3 ? dim[axis]: interp.size (axis); }
        default_type spacing (size_t axis) const { return axis < 3 ? vox[axis] : interp.spacing (axis); }
        const std::string& name () const { return interp.name(); }

        ssize_t stride (size_t axis) const {
          return interp.stride (axis);
        }

        void reset () {
          x[0] = x[1] = x[2] = 0;
          for (size_t n = 3; n < interp.ndim(); ++n)
            interp.index(n) = 0;
        }


        value_type value () {
          Eigen::Vector3 pos = get_position();
          if (std::isnan(pos[0]) || std::isnan(pos[1]) || std::isnan(pos[2]))
            return value_when_out_of_bounds;
          interp.scanner (pos);
          return interp.value();
        }


        Eigen::Matrix<value_type, Eigen::Dynamic, 1> row (size_t axis) {
          assert (interp.ndim() > 3);
          Eigen::Vector3 pos = get_position();
          if (std::isnan(pos[0]) || std::isnan(pos[1]) || std::isnan(pos[2])) {
            Eigen::Matrix<value_type, Eigen::Dynamic, 1> out_of_bounds_row (interp.size (axis));
            out_of_bounds_row.setOnes();
            out_of_bounds_row *= value_when_out_of_bounds;
            return out_of_bounds_row;
          }
          interp.scanner (pos);
          return interp.row (axis);
        }


        ssize_t get_index (size_t axis) const { return axis < 3 ? x[axis] : interp.index(axis); }
        void move_index (size_t axis, ssize_t increment) {
          if (axis < 3) x[axis] += increment;
          else interp.index(axis) += increment;
        }

      private:

        Eigen::Vector3d get_position (){
          warp.index(0) = x[0];
          warp.index(1) = x[1];
          warp.index(2) = x[2];

          return warp.row(3).template cast<double>();
        }

        Interpolator<ImageType> interp;
        WarpType warp;
        ssize_t x[3];
        const ssize_t dim[3];
        const default_type vox[3];
        value_type value_when_out_of_bounds;
    };

    //! @}

  }
}

#endif




