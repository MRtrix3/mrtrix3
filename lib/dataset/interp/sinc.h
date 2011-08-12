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

#ifndef __dataset_interp_sinc_h__
#define __dataset_interp_sinc_h__

#include "dataset/interp/base.h"
#include "math/sinc.h"


#define SINC_WINDOW_SIZE 7


namespace MR
{
  namespace DataSet
  {
    namespace Interp
    {

      //! \addtogroup interp
      // @{

      //! This class provides access to the voxel intensities of a data set, using sinc interpolation.
      /*! Interpolation is only performed along the first 3 (spatial) axes.
       * The (integer) position along the remaining axes should be set using the
       * template DataSet class.
       * The spatial coordinates can be set using the functions voxel(), image(),
       * and scanner().
       * For example:
       * \code
       * Image::Voxel<float> voxel (image);
       *
       * // create an Interp::Sinc object using voxel as the parent data set:
       * DataSet::Interp::Sinc< Image::Voxel<float> > interp (voxel);
       *
       * // set the scanner-space position to [ 10.2 3.59 54.1 ]:
       * interp.scanner (10.2, 3.59, 54.1);
       *
       * // get the value at this position:
       * float value = interp.value();
       * \endcode
       *
       * The template \a voxel class must be usable with this type of syntax:
       * \code
       * int xdim = voxel.dim(0);    // return the dimension
       * int ydim = voxel.dim(1);    // along the x, y & z dimensions
       * int zdim = voxel.dim(2);
       * float v[] = { voxel.vox(0), voxel.vox(1), voxel.vox(2) };  // return voxel dimensions
       * voxel[0] = 0;               // these lines are used to
       * voxel[1]--;                 // set the current position
       * voxel[2]++;                 // within the data set
       * float f = voxel.value();
       * Math::Transform<float> M = voxel.transform(); // a valid 4x4 transformation matrix
       * \endcode
       */

      template <class Set, typename T = float> class Sinc : public Base<Set,T>
      {
        private:
          typedef class Base<Set> B;

        public:
          typedef typename Set::value_type value_type;
          typedef typename Set::value_type pos_type;

          //! construct an Interp object to obtain interpolated values using the
          // parent DataSet class
          Sinc (Set& parent, const size_t w = SINC_WINDOW_SIZE) :
            Base<Set> (parent),
            window_size (w),
            Sinc_x (w),
            Sinc_y (w),
            Sinc_z (w),
            y_values (w, 0.0),
            z_values (w, 0.0)
            {
              assert (w % 2);
              B::out_of_bounds = false;
            }

          //! Set the current position to <b>voxel space</b> position \a pos
          /*! This will set the position from which the image intensity values will
           * be interpolated, assuming that \a pos provides the position as a
           * (floating-point) voxel coordinate within the dataset. */
          bool voxel (const Point<pos_type>& pos) {
            if (!within_bounds (pos)) {
              B::out_of_bounds = true;
              return true;
            }
            Sinc_x.set (B::data, 0, pos[0]);
            Sinc_y.set (B::data, 1, pos[1]);
            Sinc_z.set (B::data, 2, pos[2]);
            return (false);
          }
          //! Set the current position to <b>image space</b> position \a pos
          /*! This will set the position from which the image intensity values will
           * be interpolated, assuming that \a pos provides the position as a
           * coordinate relative to the axes of the dataset, in units of
           * millimeters. The origin is taken to be the centre of the voxel at [
           * 0 0 0 ]. */
          bool image (const Point<pos_type>& pos) {
            return (voxel (B::image2voxel (pos)));
          }
          //! Set the current position to the <b>scanner space</b> position \a pos
          /*! This will set the position from which the image intensity values will
           * be interpolated, assuming that \a pos provides the position as a
           * scanner space coordinate, in units of millimeters. */
          bool scanner (const Point<pos_type>& pos) {
            return (voxel (B::scanner2voxel (pos)));
          }

          value_type value () {
            for (size_t z = 0; z != window_size; ++z) {
              B::data[2] = Sinc_z.index (z);
              for (size_t y = 0; y != window_size; ++y) {
                B::data[1] = Sinc_y.index (y);
                y_values[y] = Sinc_x.value (B::data, 0);
              }
              z_values[z] = Sinc_y.value (y_values);
            }
            return (Sinc_z.value (z_values));
          }


        protected:
          const size_t window_size;
          Math::Sinc<value_type> Sinc_x, Sinc_y, Sinc_z;
          std::vector<value_type> y_values, z_values;

          bool within_bounds (const Point<pos_type>& p) {
            // Bounds testing is different for sinc interpolation than others
            // Not only due to the width of the kernel, but also the mirroring of the image data beyond the FoV
            static const int kernel_width = (window_size - 1) / 2;
            return ((round (p[0]) > -B::data.dim(0) + kernel_width) && (round (p[0]) < (2 * B::data.dim(0)) - kernel_width)
                 && (round (p[1]) > -B::data.dim(1) + kernel_width) && (round (p[1]) < (2 * B::data.dim(1)) - kernel_width)
                 && (round (p[2]) > -B::data.dim(2) + kernel_width) && (round (p[2]) < (2 * B::data.dim(2)) - kernel_width));
          }

      };

      //! @}

    }
  }
}

#endif


