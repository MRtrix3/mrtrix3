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

#ifndef __image_interp_linear_h__
#define __image_interp_linear_h__

#include "image/transform.h"
#include "datatype.h"

namespace MR
{
  namespace Image
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
       * Image::Voxel<float> voxel (image);
       *
       * // create an Interp::Linear object using voxel as the parent data set:
       * DataSet::Interp::Linear<Image::Voxel<float> > interp (voxel);
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

      template <class VoxelType>
        class Linear : public VoxelType, public Transform
      {
        public:
          typedef typename VoxelType::value_type value_type;

          using Transform::set_to_nearest;
          using VoxelType::dim;
          using Transform::image2voxel;
          using Transform::scanner2voxel;
          using Transform::operator!;
          using typename Transform::out_of_bounds;
          using typename Transform::bounds;
          //! construct an Linear object to obtain interpolated values using the
          // parent DataSet class
          Linear (const VoxelType& parent, value_type value_when_out_of_bounds = DataType::default_out_of_bounds_value<value_type>()) :
            VoxelType (parent),
            Transform (parent),
            out_of_bounds_value (value_when_out_of_bounds) { }

          //! Set the current position to <b>voxel space</b> position \a pos
          /*! This will set the position from which the image intensity values will
           * be interpolated, assuming that \a pos provides the position as a
           * (floating-point) voxel coordinate within the dataset. */
          bool voxel (const Point<float>& pos) {
            Point<float> f = set_to_nearest (pos);
            if (out_of_bounds)
              return true;

            if (pos[0] < 0.0) {
              f[0] = 0.0;
              (*this)[0] = 0;
            }
            else {
              (*this)[0] = Math::floor (pos[0]);
              if (pos[0] > bounds[0]-0.5) f[0] = 0.0;
            }
            if (pos[1] < 0.0) {
              f[1] = 0.0;
              (*this)[1] = 0;
            }
            else {
              (*this)[1] = Math::floor (pos[1]);
              if (pos[1] > bounds[1]-0.5) f[1] = 0.0;
            }

            if (pos[2] < 0.0) {
              f[2] = 0.0;
              (*this)[2] = 0;
            }
            else {
              (*this)[2] = Math::floor (pos[2]);
              if (pos[2] > bounds[2]-0.5) f[2] = 0.0;
            }

            faaa = (1.0-f[0]) * (1.0-f[1]) * (1.0-f[2]);
            if (faaa < 1e-6) faaa = 0.0;
            faab = (1.0-f[0]) * (1.0-f[1]) *      f[2];
            if (faab < 1e-6) faab = 0.0;
            faba = (1.0-f[0]) *      f[1]  * (1.0-f[2]);
            if (faba < 1e-6) faba = 0.0;
            fabb = (1.0-f[0]) *      f[1]  *      f[2];
            if (fabb < 1e-6) fabb = 0.0;
            fbaa =      f[0]  * (1.0-f[1]) * (1.0-f[2]);
            if (fbaa < 1e-6) fbaa = 0.0;
            fbab =      f[0]  * (1.0-f[1])      * f[2];
            if (fbab < 1e-6) fbab = 0.0;
            fbba =      f[0]  *      f[1]  * (1.0-f[2]);
            if (fbba < 1e-6) fbba = 0.0;
            fbbb =      f[0]  *      f[1]  *      f[2];
            if (fbbb < 1e-6) fbbb = 0.0;

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
              return out_of_bounds_value;
            value_type val = 0.0;
            if (faaa) val  = faaa * VoxelType::value();
            (*this)[2]++;
            if (faab) val += faab * VoxelType::value();
            (*this)[1]++;
            if (fabb) val += fabb * VoxelType::value();
            (*this)[2]--;
            if (faba) val += faba * VoxelType::value();
            (*this)[0]++;
            if (fbba) val += fbba * VoxelType::value();
            (*this)[1]--;
            if (fbaa) val += fbaa * VoxelType::value();
            (*this)[2]++;
            if (fbab) val += fbab * VoxelType::value();
            (*this)[1]++;
            if (fbbb) val += fbbb * VoxelType::value();
            (*this)[0]--;
            (*this)[1]--;
            (*this)[2]--;
            return val;
          }

          const value_type out_of_bounds_value;

        protected:
          float  faaa, faab, faba, fabb, fbaa, fbab, fbba, fbbb;
      };

      //! @}

    }
  }
}

#endif

