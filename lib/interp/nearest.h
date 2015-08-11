/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 27/06/08.

   This file is part of MRtrix.

   MRtrix is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   MRtrix is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

 */

#ifndef __interp_nearest_h__
#define __interp_nearest_h__

#include "transform.h"
#include "datatype.h"

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
      class Nearest : public ImageType, public Transform
    {
      public:
        typedef typename ImageType::value_type value_type;

        using Transform::set_to_nearest;
        using ImageType::index;
        using Transform::scanner2voxel;
        using Transform::operator!;
        using Transform::out_of_bounds;
        using Transform::bounds;

        //! construct an Nearest object to obtain interpolated values using the
        // parent DataSet class
        Nearest (const ImageType& parent, value_type value_when_out_of_bounds = Transform::default_out_of_bounds_value<value_type>()) :
          ImageType (parent),
          Transform (parent),
          out_of_bounds_value (value_when_out_of_bounds) { }

        //! Set the current position to <b>voxel space</b> position \a pos
        /*! This will set the position from which the image intensity values will
         * be interpolated, assuming that \a pos provides the position as a
         * (floating-point) voxel coordinate within the dataset. */
        template <class VectorType>
        bool voxel (const VectorType& pos) {
          set_to_nearest (pos);
          if (out_of_bounds)
            return true;

          index(0) = std::round (pos[0]);
          index(1) = std::round (pos[1]);
          index(2) = std::round (pos[2]);
          return false;
        }


        //! Set the current position to <b>image space</b> position \a pos
        /*! This will set the position from which the image intensity values will
         * be interpolated, assuming that \a pos provides the position as a
         * coordinate relative to the axes of the dataset, in units of
         * millimeters. The origin is taken to be the centre of the voxel at [
         * 0 0 0 ]. */
        template <class VectorType>
        bool image (const VectorType& pos) {
          return voxel (voxelsize.inverse() * pos.template cast<double>());
        }
        //! Set the current position to the <b>scanner space</b> position \a pos
        /*! This will set the position from which the image intensity values will
         * be interpolated, assuming that \a pos provides the position as a
         * scanner space coordinate, in units of millimeters. */
        template <class VectorType>
        bool scanner (const VectorType& pos) {
          return voxel (Transform::scanner2voxel * pos.template cast<double>());
        }

        value_type value () const {
          if (out_of_bounds) {
            return out_of_bounds_value;
          }

          return ImageType::value();
        }

        // Collectively interpolates values along axis >= 3
        Eigen::Matrix<value_type, Eigen::Dynamic, 1> row (size_t axis) {
          if (out_of_bounds) {
            Eigen::Matrix<value_type, Eigen::Dynamic, 1> out_of_bounds_row (ImageType::size(axis));
            out_of_bounds_row.setOnes();
            out_of_bounds_row *= out_of_bounds_value;
            return out_of_bounds_row;
          }

          return ImageType::row(axis);
        }

        const value_type out_of_bounds_value;
    };



  template <class ImageType, typename... Args>
    inline Nearest<ImageType> make_nearest (const ImageType& parent, Args&&... args) {
      return Nearest<ImageType> (parent, std::forward<Args> (args)...);
    }



    //! @}

  }
}

#endif


