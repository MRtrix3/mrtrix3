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

#ifndef __interp_linear_h__
#define __interp_linear_h__

#include <complex>
#include <type_traits>

#include "datatype.h"
#include "types.h"
#include "interp/base.h"


namespace MR
{
  namespace Interp
  {

    //! \addtogroup interp
    // @{

    //! This class provides access to the voxel intensities of a data set, using tri-linear interpolation.
    /*! Interpolation is only performed along the first 3 (spatial) axes.
     * The (integer) position along the remaining axes should be set using the
     * template DataSet class.
     * The spatial coordinates can be set using the functions voxel(), image(),
     * and scanner().
     * For example:
     * \code
     * auto voxel = image_buffer.voxel();
     *
     * auto input = Image<float>::create (Argument[0]);
     *
     * // create an Interp::Linear object using input as the parent data set:
     * Interp::Linear<decltype(input) > interp (input);
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
    template <class C>
    struct value_type_of
    {
      typedef C type;
    };

    template <class X>
    struct value_type_of<std::complex<X>>
    {
      typedef X type;
    };

    template <class ImageType>
      class Linear : public Base<ImageType>
    {
      public:
        using typename Base<ImageType>::value_type;

        using Base<ImageType>::bounds;
        using Base<ImageType>::index;
        using Base<ImageType>::out_of_bounds;
        using Base<ImageType>::out_of_bounds_value;

        typedef typename value_type_of<value_type>::type coef_type;

        Linear (const ImageType& parent, value_type value_when_out_of_bounds = Base<ImageType>::default_out_of_bounds_value()) :
            Base<ImageType> (parent, value_when_out_of_bounds),
            zero (0.0),
            eps (1.0e-6) { }

        //! Set the current position to <b>voxel space</b> position \a pos
        /*! See file interp/base.h for details. */
        template <class VectorType>
        bool voxel (const VectorType& pos) {
          Eigen::Vector3 f = Base<ImageType>::intravoxel_offset (pos.template cast<default_type>());
          if (out_of_bounds)
            return false;

          if (pos[0] < 0.0) {
            f[0] = 0.0;
            index(0) = 0;
          }
          else {
            index(0) = std::floor (pos[0]);
            if (pos[0] > bounds[0]-0.5) f[0] = 0.0;
          }
          if (pos[1] < 0.0) {
            f[1] = 0.0;
            index(1) = 0;
          }
          else {
            index(1) = std::floor (pos[1]);
            if (pos[1] > bounds[1]-0.5) f[1] = 0.0;
          }

          if (pos[2] < 0.0) {
            f[2] = 0.0;
            index(2) = 0;
          }
          else {
            index(2) = std::floor (pos[2]);
            if (pos[2] > bounds[2]-0.5) f[2] = 0.0;
          }

          faaa = (1.0-f[0]) * (1.0-f[1]) * (1.0-f[2]);
          if (faaa < eps) faaa = 0.0;
          faab = (1.0-f[0]) * (1.0-f[1]) *      f[2];
          if (faab < eps) faab = 0.0;
          faba = (1.0-f[0]) *      f[1]  * (1.0-f[2]);
          if (faba < eps) faba = 0.0;
          fabb = (1.0-f[0]) *      f[1]  *      f[2];
          if (fabb < eps) fabb = 0.0;
          fbaa =      f[0]  * (1.0-f[1]) * (1.0-f[2]);
          if (fbaa < eps) fbaa = 0.0;
          fbab =      f[0]  * (1.0-f[1])      * f[2];
          if (fbab < eps) fbab = 0.0;
          fbba =      f[0]  *      f[1]  * (1.0-f[2]);
          if (fbba < eps) fbba = 0.0;
          fbbb =      f[0]  *      f[1]  *      f[2];
          if (fbbb < eps) fbbb = 0.0;

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
          value_type val = 0.0;
          if (faaa != zero) val  = faaa * value_type (ImageType::value());
          index(2)++;
          if (faab != zero) val += faab * value_type (ImageType::value());
          index(1)++;
          if (fabb != zero) val += fabb * value_type (ImageType::value());
          index(2)--;
          if (faba != zero) val += faba * value_type (ImageType::value());
          index(0)++;
          if (fbba != zero) val += fbba * value_type (ImageType::value());
          index(1)--;
          if (fbaa != zero) val += fbaa * value_type (ImageType::value());
          index(2)++;
          if (fbab != zero) val += fbab * value_type (ImageType::value());
          index(1)++;
          if (fbbb != zero) val += fbbb * value_type (ImageType::value());
          index(0)--;
          index(1)--;
          index(2)--;
          return val;
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
          row.setZero();
          if (faaa != zero) row  = faaa * ImageType::row(axis);
          index(2)++;
          if (faab != zero) row += faab * ImageType::row(axis);
          index(1)++;
          if (fabb != zero) row += fabb * ImageType::row(axis);
          index(2)--;
          if (faba != zero) row += faba * ImageType::row(axis);
          index(0)++;
          if (fbba != zero) row += fbba * ImageType::row(axis);
          index(1)--;
          if (fbaa != zero) row += fbaa * ImageType::row(axis);
          index(2)++;
          if (fbab != zero) row += fbab * ImageType::row(axis);
          index(1)++;
          if (fbbb != zero) row += fbbb * ImageType::row(axis);
          index(0)--;
          index(1)--;
          index(2)--;
          return row;
        }

      protected:
        coef_type faaa, faab, faba, fabb, fbaa, fbab, fbba, fbbb;

      private:
        const coef_type zero, eps;


    };



    template <class ImageType, typename... Args>
      inline Linear<ImageType> make_linear (const ImageType& parent, Args&&... args) {
        return Linear<ImageType> (parent, std::forward<Args> (args)...);
      }

    //! @}

  }
}

#endif

