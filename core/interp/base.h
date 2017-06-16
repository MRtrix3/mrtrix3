/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#ifndef __interp_base_h__
#define __interp_base_h__

#include "image_helpers.h"
#include "transform.h"


namespace MR
{

  class Header;

  namespace Interp
  {

    //! \addtogroup interp
    // @{

    //! This class defines the interface for classes that perform image interpolation
    /*! Interpolation is generally performed along the first 3 (spatial) axes;
     * the (integer) position along the remaining axes should be set using the
     * template ImageType class.
     * The spatial coordinates can be set using the functions voxel(), image(),
     * and scanner().
     * For example:
     * \code
     * auto input = Image<float>::create (argument[0]);
     *
     * // create an Interp::Cubic object using input as the parent data set:
     * Interp::Cubic<decltype(input)> interp (input);
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

    template <class ImageType> class Base : public ImageType, public Transform
    { MEMALIGN(Base<ImageType>)
      public:
        using value_type = typename ImageType::value_type;

        //! construct an Interp object to obtain interpolated values using the
        //! parent ImageType class
        Base (const ImageType& parent, value_type value_when_out_of_bounds = default_out_of_bounds_value()) :
            ImageType (parent),
            Transform (parent),
            out_of_bounds_value (value_when_out_of_bounds),
            bounds { parent.size(0) - 0.5, parent.size(1) - 0.5, parent.size(2) - 0.5 },
            out_of_bounds (true)
        {
          check_3D_nonunity (parent);
        }


        //! Functions that must be defined by interpolation classes
        /*! The follwing functions must be defined by any derived
         *  interpolation class. They are NOT defined as virtual functions
         *  here in order to prevent use of vtables in performance-critical
         *  code; nevertheless, the interface should be consistent for all
         *  derived interpolation classes. */

        //! Set the current position to <b>voxel space</b> position \a pos
        /*! This will set the position from which the image intensity values will
         * be interpolated, assuming that \a pos provides the position as a
         * (floating-point) voxel coordinate within the dataset.
         * Interpolation classes need to provide a specialization of this method.
         * Note that unlike some previous code versions, a TRUE return value
         * indicates that the point is WITHIN the image volume.
         *
         * \code
         * template <class VectorType> bool voxel (const VectorType& pos);
         * \endcode
         * */

        //! Set the current position to <b>image space</b> position \a pos
        /*! This will set the position from which the image intensity values will
         * be interpolated, assuming that \a pos provides the position as a
         * coordinate relative to the axes of the dataset, in units of
         * millimeters. The origin is taken to be the centre of the voxel at [
         * 0 0 0 ].
         *
         * Derived classes should reproduce the code exactly as it appears below:
         *
         * \code
         * template <class VectorType>
         * FORCE_INLINE bool image (const VectorType& pos) {
         *   return voxel (Transform::voxelsize.inverse() * pos.template cast<default_type>());
         * }
         * \endcode
         */

        //! Set the current position to the <b>scanner space</b> position \a pos
        /*! This will set the position from which the image intensity values will
         * be interpolated, assuming that \a pos provides the position as a
         * scanner space coordinate, in units of millimeters.
         *
         * Derived classes should reproduce the code exactly as it appears below:
         *
         * \code
         * template <class VectorType>
         * FORCE_INLINE bool scanner (const VectorType& pos) {
         *   return voxel (Transform::scanner2voxel * pos.template cast<default_type>());
         * }
         * \endcode
         */

        //! Read an interpolated value from the current position
        /*! This function must be preceded by a call to voxel(), image() or
         *  scanner(), and perform the core functionality of the derived
         *  interpolator class - that is, generating an intensity value
         *  based on the image data. The precise implementation of this
         *  function will therefore depend on the nature of the interpolator.
         *
         * \code
         * FORCE_INLINE value_type value ()
         * {
             if (out_of_bounds)
               return out_of_bounds_value;
             { ... }
         * }
         * \endcode
         * */

        //! Read interpolated values from volumes along axis >= 3
        /*! This function uses the initialisation of interpolation position
         *  from the voxel(), image() or scanner() functions to produce
         *  interpolated values across a set of volumes, making the
         *  process of interpolation much faster. The specific implementation
         *  of this function, and indeed whether or not the initialisation
         *  from the voxel() function can be exploited across multiple
         *  volumes, will depend on the specific details of the interpolator.
         *
         * \code
         * Eigen::Matrix<value_type, Eigen::Dynamic, 1> row (size_t axis)
         * {
         *   assert (axis > 2);
         *   assert (axis < ImageType::ndim());
         *   if (out_of_bounds) {
         *     Eigen::Matrix<value_type, Eigen::Dynamic, 1> out_of_bounds_row (ImageType::size(axis));
         *     out_of_bounds_row.setOnes();
         *     out_of_bounds_row *= out_of_bounds_value;
         *     return out_of_bounds_row;
         *   }
         *   { ... }
         * }
         * \endcode
         * */


        // Value to return when the position is outside the bounds of the image volume
        const value_type out_of_bounds_value;

        static inline value_type default_out_of_bounds_value () {
          return std::numeric_limits<value_type>::quiet_NaN();
        }

        //! test whether current position is within bounds.
        /*! \return true if the current position is out of bounds, false otherwise */
        bool operator! () const { return out_of_bounds; }



      protected:
        default_type bounds[3];
        bool out_of_bounds;


        // Some helper functions
        template <class VectorType>
        bool is_out_of_bounds (const VectorType& pos) const {
          if (pos[0] <= -0.5 || pos[0] >= bounds[0] ||
              pos[1] <= -0.5 || pos[1] >= bounds[1] ||
              pos[2] <= -0.5 || pos[2] >= bounds[2]) {
            return true;
          }
          return false;
        }

        template <class VectorType>
        Eigen::Vector3 intravoxel_offset (const VectorType& pos) {
          out_of_bounds = (*this).is_out_of_bounds (pos); // Don't want the equally-named function in image_helpers.h!
          if (out_of_bounds)
            return Eigen::Vector3 (NaN, NaN, NaN);
          else
            return Eigen::Vector3 (pos[0]-std::floor (pos[0]), pos[1]-std::floor (pos[1]), pos[2]-std::floor (pos[2]));
        }

    };



    //! @}

  }
}

#endif


