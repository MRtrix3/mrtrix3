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

#ifndef __dataset_interp_nearest_h__
#define __dataset_interp_nearest_h__

#include "dataset/interp/base.h"

namespace MR {
  namespace DataSet {
    namespace Interp {

      //! \addtogroup interp
      // @{

      //! This class provides access to the voxel intensities of a data set, using nearest-neighbour interpolation.
      /*! Interpolation is only performed along the first 3 (spatial) axes. 
       * The (integer) position along the remaining axes should be set using the
       * template DataSet class.
       * The spatial coordinates can be set using the functions voxel(), image(),
       * and scanner(). 
       * For example:
       * \code
       * Image::Voxel voxel (image);
       *
       * // create an Interp::Nearest object using voxel as the parent data set:
       * Image::Interp::Nearest<Image::Voxel> interp (voxel);
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
       * Math::Transform<float> M = voxel.transform; // a valid 4x4 transformation matrix
       * \endcode
       */

      template <class Set> class Nearest : public Base<Set> 
      {
        private:
          typedef class Base<Set> B;

        public:
          typedef typename Set::value_type value_type;

          //! construct an Nearest object to obtain interpolated values using the
          // parent DataSet class 
          Nearest (Set& parent) : Base<Set> (parent) { }

          //! Set the current position to <b>voxel space</b> position \a pos
          /*! This will set the position from which the image intensity values will
           * be interpolated, assuming that \a pos provides the position as a
           * (floating-point) voxel coordinate within the dataset. */
          bool voxel (const Point& pos)
          {
            Point f = B::set (pos);
            if (B::out_of_bounds) return (true);
            B::data[0] = Math::round<ssize_t> (pos[0]);
            B::data[1] = Math::round<ssize_t> (pos[1]);
            B::data[2] = Math::round<ssize_t> (pos[2]);
            return (false);
          }


          //! Set the current position to <b>image space</b> position \a pos
          /*! This will set the position from which the image intensity values will
           * be interpolated, assuming that \a pos provides the position as a
           * coordinate relative to the axes of the dataset, in units of
           * millimeters. The origin is taken to be the centre of the voxel at [
           * 0 0 0 ]. */
          bool image (const Point& pos) { return (voxel (B::image2voxel (pos))); }
          //! Set the current position to the <b>scanner space</b> position \a pos
          /*! This will set the position from which the image intensity values will
           * be interpolated, assuming that \a pos provides the position as a
           * scanner space coordinate, in units of millimeters. */
          bool scanner (const Point& pos) { return (voxel (B::scanner2voxel (pos))); }

          value_type value () const
          {
            if (B::out_of_bounds) return (NAN);
            return (B::data.value());
          }
      };

      //! @}

    }
  }
}

#endif


