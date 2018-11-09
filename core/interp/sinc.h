/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#ifndef __interp_sinc_h__
#define __interp_sinc_h__

#include "types.h"
#include "interp/base.h"
#include "math/sinc.h"


#define SINC_WINDOW_SIZE 7


namespace MR
{
  namespace Interp
  {

    //! \addtogroup interp
    // @{

    //! This class provides access to the voxel intensities of an image, using sinc interpolation.
    /*! Interpolation is only performed along the first 3 (spatial) axes.
     * The (integer) position along the remaining axes should be set using the
     * template Image class.
     * The spatial coordinates can be set using the functions voxel(), image(),
     * and scanner().
     * For example:
     * \code
   * auto input = Image<float>::create (Argument[0]);
   *
   * // create an Interp::Sinc object using input as the parent data set:
   * Interp::Sinc<decltype(input) > interp (input);
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

    template <class ImageType> class Sinc : public Base<ImageType>
    { MEMALIGN(Sinc<ImageType>)
      public:
        using typename Base<ImageType>::value_type;
        using Base<ImageType>::out_of_bounds;
        using Base<ImageType>::out_of_bounds_value;

        Sinc (const ImageType& parent, value_type value_when_out_of_bounds = Base<ImageType>::default_out_of_bounds_value(), const size_t w = SINC_WINDOW_SIZE) :
            Base<ImageType> (parent, value_when_out_of_bounds),
            window_size (w),
            kernel_width ((window_size-1)/2),
            Sinc_x (w),
            Sinc_y (w),
            Sinc_z (w),
            y_values (w, 0.0),
            z_values (w, 0.0)
        {
          assert (w % 2);
        }


        //! Set the current position to <b>voxel space</b> position \a pos
        /*! See file interp/base.h for details. */
        template <class VectorType>
        bool voxel (const VectorType& pos) {
          if ((out_of_bounds = Base<ImageType>::is_out_of_bounds (pos)))
            return false;
          Sinc_x.set (*this, 0, pos[0]);
          Sinc_y.set (*this, 1, pos[1]);
          Sinc_z.set (*this, 2, pos[2]);
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
        FORCE_INLINE value_type value () {
          if (out_of_bounds)
            return out_of_bounds_value;
          for (size_t z = 0; z != window_size; ++z) {
            ImageType::index(2) = Sinc_z.index (z);
            for (size_t y = 0; y != window_size; ++y) {
              ImageType::index(1) = Sinc_y.index (y);
              // Cast necessary so that Sinc_x calls the ImageType value() function
              y_values[y] = Sinc_x.value (static_cast<ImageType&>(*this), 0);
            }
            z_values[z] = Sinc_y.value (y_values);
          }
          return Sinc_z.value (z_values);
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

          Eigen::Matrix<value_type, Eigen::Dynamic, 1> row (ImageType::size(axis));
          // Lazy, non-optimized code, since nothing is actually using this yet
          // Just make use of the kernel setup within voxel()
          for (ssize_t volume = 0; volume != ImageType::size(axis); ++volume) {
            ImageType::index (axis) = volume;
            row (volume,0) = value();
          }
          return row;
        }


      protected:
        const size_t window_size;
        const int kernel_width;
        Math::Sinc<value_type> Sinc_x, Sinc_y, Sinc_z;
        vector<value_type> y_values, z_values;

    };





  template <class ImageType, typename... Args>
    inline Sinc<ImageType> make_sinc (const ImageType& parent, Args&&... args) {
      return Sinc<ImageType> (parent, std::forward<Args> (args)...);
    }


    //! @}

  }
}

#endif


