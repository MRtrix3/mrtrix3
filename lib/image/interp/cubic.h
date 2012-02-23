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

#ifndef __image_interp_cubic_h__
#define __image_interp_cubic_h__

#include "image/interp/base.h"
#include "math/hermite.h"

namespace MR
{
  namespace Image
  {
    namespace Interp
    {

      //! \addtogroup interp
      // @{

      //! This class provides access to the voxel intensities of a data set, using cubic spline interpolation.
      /*! Interpolation is only performed along the first 3 (spatial) axes.
       * The (integer) position along the remaining axes should be set using the
       * template DataSet class.
       * The spatial coordinates can be set using the functions voxel(), image(),
       * and scanner().
       * For example:
       * \code
       * Image::Voxel<float> voxel (image);
       *
       * // create an Interp::Cubic object using voxel as the parent data set:
       * DataSet::Interp::Cubic<Image::Voxel<float> > interp (voxel);
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

      template <class VoxelType> class Cubic : public Base<VoxelType>
      {
        public:
          typedef typename VoxelType::value_type value_type;

          using Base<VoxelType>::set;
          using Base<VoxelType>::dim;
          using Base<VoxelType>::image2voxel;
          using Base<VoxelType>::scanner2voxel;
          using typename Base<VoxelType>::out_of_bounds;
          using typename Base<VoxelType>::bounds;

          //! construct an Nearest object to obtain interpolated values using the
          // parent DataSet class
          Cubic (const VoxelType& parent) : Base<VoxelType> (parent) { }

          //! Set the current position to <b>voxel space</b> position \a pos
          /*! This will set the position from which the image intensity values will
           * be interpolated, assuming that \a pos provides the position as a
           * (floating-point) voxel coordinate within the dataset. */
          bool voxel (const Point<float>& pos) {
            Point<float> f = set (pos);
            if (out_of_bounds)
              return true;
            P = pos;
            Hx.set (f[0]);
            Hy.set (f[1]);
            Hz.set (f[2]);
            return false;
          }
          //! Set the current position to <b>image space</b> position \a pos
          /*! This will set the position from which the image intensity values will
           * be interpolated, assuming that \a pos provides the position as a
           * coordinate relative to the axes of the dataset, in units of
           * millimeters. The origin is taken to be the centre of the voxel at [
           * 0 0 0 ]. */
          bool image (const Point<float>& pos) {
            return voxel (image2voxel (pos));
          }
          //! Set the current position to the <b>scanner space</b> position \a pos
          /*! This will set the position from which the image intensity values will
           * be interpolated, assuming that \a pos provides the position as a
           * scanner space coordinate, in units of millimeters. */
          bool scanner (const Point<float>& pos) {
            return voxel (scanner2voxel (pos));
          }

          value_type value () {
            if (out_of_bounds)
              return NAN;

            ssize_t c[] = { Math::floor (P[0])-1, Math::floor (P[1])-1, Math::floor (P[2])-1 };
            value_type r[4];
            for (ssize_t z = 0; z < 4; ++z) {
              (*this)[2] = check (c[2] + z, dim (2)-1);
              value_type q[4];
              for (ssize_t y = 0; y < 4; ++y) {
                (*this)[1] = check (c[1] + y, dim (1)-1);
                value_type p[4];
                for (ssize_t x = 0; x < 4; ++x) {
                  (*this)[0] = check (c[0] + x, dim (0)-1);
                  p[x] = VoxelType::value();
                }
                q[y] = Hx.value (p);
              }
              r[z] = Hy.value (q);
            }
            return Hz.value (r);
          }


        protected:
          Math::Hermite<value_type> Hx, Hy, Hz;
          Point<float> P;

          ssize_t check (ssize_t x, ssize_t dim) const {
            if (x < 0) return 0;
            if (x > dim) return dim;
            return x;
          }
      };

      //! @}

    }
  }
}

#endif


