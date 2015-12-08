/*
   Copyright 2011 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 12/08/11.

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

#ifndef __interp_sinc_h__
#define __interp_sinc_h__

#include "transform.h"
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

    template <class ImageType> class Sinc : public ImageType, public Transform
    {
      public:
        typedef typename ImageType::value_type value_type;

        using Transform::set_to_nearest;
        using ImageType::index;
        using ImageType::size;
        using Transform::scanner2voxel;
        using Transform::operator!;
        using Transform::out_of_bounds;
        using Transform::bounds;

        //! construct an Interp object to obtain interpolated values using the
        // parent DataSet class
        Sinc (const ImageType& parent, value_type value_when_out_of_bounds = Transform::default_out_of_bounds_value<value_type>(), const size_t w = SINC_WINDOW_SIZE) :
            ImageType (parent),
            Transform (parent),
            out_of_bounds_value (value_when_out_of_bounds),
            window_size (w),
            kernel_width ((window_size-1)/2),
            Sinc_x (w),
            Sinc_y (w),
            Sinc_z (w),
            y_values (w, 0.0),
            z_values (w, 0.0)
        {
          assert (w % 2);
          out_of_bounds = false;
        }

        //! Set the current position to <b>voxel space</b> position \a pos
        /*! This will set the position from which the image intensity values will
         * be interpolated, assuming that \a pos provides the position as a
         * (floating-point) voxel coordinate within the dataset. */
        template <class VectorType>
        bool voxel (const VectorType& pos) {
          if ((out_of_bounds = is_out_of_bounds (pos)))
            return false;
          Sinc_x.set (*this, 0, pos[0]);
          Sinc_y.set (*this, 1, pos[1]);
          Sinc_z.set (*this, 2, pos[2]);
          return true;
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

        value_type value () {
          if (out_of_bounds)
            return out_of_bounds_value;
          for (size_t z = 0; z != window_size; ++z) {
            index(2) = Sinc_z.index (z);
            for (size_t y = 0; y != window_size; ++y) {
              index(1) = Sinc_y.index (y);
              y_values[y] = Sinc_x.value (static_cast<ImageType&>(*this), 0);
            }
            z_values[z] = Sinc_y.value (y_values);
          }
          return Sinc_z.value (z_values);
        }

        // Collectively interpolates values along axis >= 3
        Eigen::Matrix<value_type, Eigen::Dynamic, 1> row (size_t axis) {
          // TODO
          throw Exception ("Sinc interpolation of 4D images not yet implemented");
        }

        const value_type out_of_bounds_value;

      protected:
        const size_t window_size;
        const int kernel_width;
        Math::Sinc<value_type> Sinc_x, Sinc_y, Sinc_z;
        std::vector<value_type> y_values, z_values;

    };





  template <class ImageType, typename... Args>
    inline Sinc<ImageType> make_sinc (const ImageType& parent, Args&&... args) {
      return Sinc<ImageType> (parent, std::forward<Args> (args)...);
    }


    //! @}

  }
}

#endif


