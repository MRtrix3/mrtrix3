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

#ifndef __interp_nearest_h__
#define __interp_nearest_h__

#include "datatype.h"
#include "image_helpers.h"
#include "types.h"
#include "interp/base.h"

namespace MR
{
  namespace Interp
  {

    //! \addtogroup interp
    // @{

    //! This class provides access to the voxel intensities of an Image, using nearest-neighbour interpolation.
    /*! Interpolation is only performed along the first 3 (spatial) axes.
     * The (integer) position along the remaining axes should be set using the
     * template Image class.
     * The spatial coordinates can be set using the functions voxel(), image(),
     * and scanner().
     * For example:
     * \code
     * auto input = Image<float>::create (Argument[0]);
     *
     * // create an Interp::Nearest object using input as the parent data set:
     * Interp::Nearest<decltype(input) > interp (input);
     *
     * // set the scanner-space position to [ 10.2 3.59 54.1 ]:
     * interp.scanner (10.2, 3.59, 54.1);
     *
     * // get the value at this position:
     * float value = interp.value();
     * \endcode
     *
     * The template \a input class must be usable with this type of syntax:
     * \code
     * int xsize = input.size(0);    // return the dimension
     * int ysize = input.size(1);    // along the x, y & z dimensions
     * int zsize = input.size(2);
     * float v[] = { input.spacing(0), input.spacing(1), input.spacing(2) };  // return voxel dimensions
     * input.index(0) = 0;               // these lines are used to
     * input.index(1)--;                 // set the current position
     * input.index(2)++;                 // within the data set
     * float f = input.value();
     * transform_type M = input.transform(); // a valid 4x4 transformation matrix
     * \endcode
     */

    template <class ImageType>
      class Nearest : public Base<ImageType> { MEMALIGN(Nearest<ImageType>)
      public:
        using typename Base<ImageType>::value_type;

        using Base<ImageType>::index;
        using Base<ImageType>::out_of_bounds;
        using Base<ImageType>::out_of_bounds_value;

        Nearest (const ImageType& parent, value_type value_when_out_of_bounds = Base<ImageType>::default_out_of_bounds_value()) :
          Base<ImageType> (parent, value_when_out_of_bounds) { }

        //! Set the current position to <b>voxel space</b> position \a pos
        /*! See file interp/base.h for details. */
        template <class VectorType>
          bool voxel (const VectorType& pos) {
            Base<ImageType>::intravoxel_offset (pos);
            if (out_of_bounds)
              return false;
            index(0) = std::round (pos[0]);
            index(1) = std::round (pos[1]);
            index(2) = std::round (pos[2]);
            return true;
          }

        //! Set the current position to <b>image space</b> position \a pos
        /*! See file interp/base.h for details. */
        template <class VectorType>
          FORCE_INLINE bool image (const VectorType& pos) {
            return voxel (Transform::voxelsize.inverse() * pos.template cast<default_type>());
          }

        //! Set the current position to the <b>scanner space</b> position \a pos
        /*! See file interp/base.h for details. */
        template <class VectorType>
          FORCE_INLINE bool scanner (const VectorType& pos) {
            return voxel (Transform::scanner2voxel * pos.template cast<default_type>());
          }

        //! Read an interpolated image value from the current position.
        /*! See file interp/base.h for details. */
        FORCE_INLINE value_type value () const {
          if (out_of_bounds)
            return out_of_bounds_value;
          return ImageType::value();
        }

        //! Read interpolated values from volumes along axis >= 3
        /*! See file interp/base.h for details. */
        Eigen::Matrix<value_type, Eigen::Dynamic, 1> row (size_t axis) {
          assert (axis > 2);
          assert (axis < ImageType::ndim());
          if (out_of_bounds) {
            Eigen::Matrix<value_type, Eigen::Dynamic, 1> out_of_bounds_row (ImageType::size(axis));
            out_of_bounds_row.setOnes();
            out_of_bounds_row *= out_of_bounds_value;
            return out_of_bounds_row;
          }
          return ImageType::row(axis);
        }

    };



    template <class ImageType, typename... Args>
      inline Nearest<ImageType> make_nearest (const ImageType& parent, Args&&... args) {
        return Nearest<ImageType> (parent, std::forward<Args> (args)...);
      }



    //! @}

  }
}

#endif


