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

#ifndef __dataset_interp_h__
#define __dataset_interp_h__

#include "dataset/interp_base.h"

namespace MR {
  namespace DataSet {

    //! \addtogroup DataSet
    // @{

    //! This class provides access to the voxel intensities of a data set, using tri-linear interpolation.
    /*! Interpolation is only performed along the first 3 (spatial) axes. 
     * The (integer) position along the remaining axes should be set using the
     * template DataSet class.
     * The spatial coordinates can be set using the functions voxel(), image(),
     * and scanner(). 
     * For example:
     * \code
     * Image::Voxel voxel (image);
     * Image::Interp<Image::Voxel> interp (voxel);  // create an Interp object using voxel as the parent data set
     * interp.scanner (10.2, 3.59, 54.1);   // set the scanner-space position to [ 10.2 3.59 54.1 ]
     * float value = interp.value();  // get the value at this position
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

    template <class Set> class Interp : public InterpBase<Set> 
    {
      private:
        typedef class InterpBase<Set> Base;

      public:
        typedef typename Set::value_type value_type;

        //! construct an Interp object to obtain interpolated values using the
        // parent DataSet class 
        Interp (Set& parent) : InterpBase<Set> (parent) { }

        //! Set the current position to <b>voxel space</b> position \a pos
        /*! This will set the position from which the image intensity values will
         * be interpolated, assuming that \a pos provides the position as a
         * (floating-point) voxel coordinate within the dataset. */
        bool voxel (const Point& pos);
        //! Set the current position to <b>image space</b> position \a pos
        /*! This will set the position from which the image intensity values will
         * be interpolated, assuming that \a pos provides the position as a
         * coordinate relative to the axes of the dataset, in units of
         * millimeters. The origin is taken to be the centre of the voxel at [
         * 0 0 0 ]. */
        bool image (const Point& pos) { return (voxel (Base::image2voxel (pos))); }
        //! Set the current position to the <b>scanner space</b> position \a pos
        /*! This will set the position from which the image intensity values will
         * be interpolated, assuming that \a pos provides the position as a
         * scanner space coordinate, in units of millimeters. */
        bool scanner (const Point& pos) { return (voxel (Base::scanner2voxel (pos))); }

        value_type value () const;

      private:
        float  faaa, faab, faba, fabb, fbaa, fbab, fbba, fbbb;

    };

    //! @}








    template <class Set> inline bool Interp<Set>::voxel (const Point& pos)
    {
      Point f = Base::set (pos);
      if (Base::out_of_bounds) return (true);

      if (pos[0] < 0.0) { f[0] = 0.0; Base::data[0] = 0; }
      else {
        Base::data[0] = Math::floor(pos[0]);
        if (pos[0] > Base::bounds[0]-0.5) f[0] = 0.0;
      }
      if (pos[1] < 0.0) { f[1] = 0.0; Base::data[1] = 0; }
      else {
        Base::data[1] = Math::floor(pos[1]);
        if (pos[1] > Base::bounds[1]-0.5) f[1] = 0.0;
      }

      if (pos[2] < 0.0) { f[2] = 0.0; Base::data[2] = 0; }
      else {
        Base::data[2] = Math::floor(pos[2]);
        if (pos[2] > Base::bounds[2]-0.5) f[2] = 0.0;
      }

      faaa = (1.0-f[0]) * (1.0-f[1]) * (1.0-f[2]); if (faaa < 1e-6) faaa = 0.0;
      faab = (1.0-f[0]) * (1.0-f[1]) *      f[2];  if (faab < 1e-6) faab = 0.0;
      faba = (1.0-f[0]) *      f[1]  * (1.0-f[2]); if (faba < 1e-6) faba = 0.0;
      fabb = (1.0-f[0]) *      f[1]  *      f[2];  if (fabb < 1e-6) fabb = 0.0;
      fbaa =      f[0]  * (1.0-f[1]) * (1.0-f[2]); if (fbaa < 1e-6) fbaa = 0.0;
      fbab =      f[0]  * (1.0-f[1])      * f[2];  if (fbab < 1e-6) fbab = 0.0;
      fbba =      f[0]  *      f[1]  * (1.0-f[2]); if (fbba < 1e-6) fbba = 0.0;
      fbbb =      f[0]  *      f[1]  *      f[2];  if (fbbb < 1e-6) fbbb = 0.0;

      return (false);
    }


    template <class Set> inline typename Interp<Set>::value_type Interp<Set>::value () const 
    {
      if (Base::out_of_bounds) return (NAN);
      value_type val = 0.0;
      if (faaa) val  = faaa * Base::data.value(); Base::data[2]++;
      if (faab) val += faab * Base::data.value(); Base::data[1]++;
      if (fabb) val += fabb * Base::data.value(); Base::data[2]--;
      if (faba) val += faba * Base::data.value(); Base::data[0]++;
      if (fbba) val += fbba * Base::data.value(); Base::data[1]--;
      if (fbaa) val += fbaa * Base::data.value(); Base::data[2]++;
      if (fbab) val += fbab * Base::data.value(); Base::data[1]++;
      if (fbbb) val += fbbb * Base::data.value(); Base::data[0]--; Base::data[1]--; Base::data[2]--;
      return (val);
    }

  }
}

#endif

