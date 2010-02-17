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

#include "point.h"
#include "dataset/transform.h"

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

    template <class Set> class Interp {
      public:

        typedef typename Set::value_type value_type;

        //! construct an Interp object to obtain interpolated values using the
        // parent DataSet class 
        Interp (Set& parent);
        ~Interp() { }

        //! test whether current position is within bounds.
        /*! \return true if the current position is out of bounds, false otherwise */
        bool  operator! () const { return (out_of_bounds); }

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
        bool image (const Point& pos) { return (voxel (image2voxel (pos))); }
        //! Set the current position to the <b>scanner space</b> position \a pos
        /*! This will set the position from which the image intensity values will
         * be interpolated, assuming that \a pos provides the position as a
         * scanner space coordinate, in units of millimeters. */
        bool scanner (const Point& pos) { return (voxel (scanner2voxel (pos))); }

        value_type value () const {
          if (out_of_bounds) return (NAN);
          value_type val = 0.0;
          if (faaa) val  = faaa * data.value(); data[2]++;
          if (faab) val += faab * data.value(); data[1]++;
          if (fabb) val += fabb * data.value(); data[2]--;
          if (faba) val += faba * data.value(); data[0]++;
          if (fbba) val += fbba * data.value(); data[1]--;
          if (fbaa) val += fbaa * data.value(); data[2]++;
          if (fbab) val += fbab * data.value(); data[1]++;
          if (fbbb) val += fbbb * data.value(); data[0]--; data[1]--; data[2]--;
          return (val);
        }

        //! Transform the position \p r from scanner-space to voxel-space
        Point scanner2voxel (const Point& r) const { return (transform (S2V, r)); }
        //! Transform the position \p r from voxel-space to scanner-space
        Point voxel2scanner (const Point& r) const { return (transform (V2S, r)); }
        //! Transform the position \p r from image-space to voxel-space
        Point image2voxel (const Point& r) const { return (Point (r[0]/data.vox(0), r[1]/data.vox(1), r[2]/data.vox(2))); }
        //! Transform the position \p r from voxel-space to image-space
        Point voxel2image (const Point& r) const { return (Point (r[0]*data.vox(0), r[1]*data.vox(1), r[2]*data.vox(2))); }
        //! Transform the position \p r from image-space to scanner-space
        Point image2scanner (const Point& r) const { return (transform (I2S, r)); }
        //! Transform the position \p r from scanner-space to image-space
        Point scanner2image (const Point& r) const { return (transform (S2I, r)); }

        //! Transform the orientation \p r from scanner-space to voxel-space
        Point vec_R2P (const Point& r) const { return (transform_vector (S2V, r)); }
        //! Transform the orientation \p r from voxel-space to scanner-space
        Point vec_P2R (const Point& r) const { return (transform_vector (V2S, r)); }

        const float* image2scanner_matrix () const { return (*I2S); }
        const float* scanner2image_matrix () const { return (*S2I); }
        const float* voxel2scanner_matrix () const { return (*V2S); }
        const float* scanner2voxel_matrix () const { return (*S2V); }

      private:
        Set&   data;
        float  S2V[3][4], V2S[3][4], I2S[3][4], S2I[3][4];
        float  bounds[3];
        bool   out_of_bounds;
        float  faaa, faab, faba, fabb, fbaa, fbab, fbba, fbbb;

        Point transform (const float M[3][4], const Point& p) const {
          return (Point (
                M[0][0]*p[0] + M[0][1]*p[1] + M[0][2]*p[2] + M[0][3],
                M[1][0]*p[0] + M[1][1]*p[1] + M[1][2]*p[2] + M[1][3],
                M[2][0]*p[0] + M[2][1]*p[1] + M[2][2]*p[2] + M[2][3] ));
        }

        Point transform_vector (const float M[3][4], const Point& p) const {
          return (Point (
                M[0][0]*p[0] + M[0][1]*p[1] + M[0][2]*p[2],
                M[1][0]*p[0] + M[1][1]*p[1] + M[1][2]*p[2],
                M[2][0]*p[0] + M[2][1]*p[1] + M[2][2]*p[2] ));
        }

        template <class U> void set (float M[3][4], const Math::Matrix<U>& MV) {
          M[0][0] = MV(0,0); M[0][1] = MV(0,1); M[0][2] = MV(0,2); M[0][3] = MV(0,3);
          M[1][0] = MV(1,0); M[1][1] = MV(1,1); M[1][2] = MV(1,2); M[1][3] = MV(1,3);
          M[2][0] = MV(2,0); M[2][1] = MV(2,1); M[2][2] = MV(2,2); M[2][3] = MV(2,3);
        }
        Point set_fractions (const Point& pos);
    };

    //! @}







    template <class Set> Interp<Set>::Interp (Set& parent) : data (parent), out_of_bounds (true)
    { 
      bounds[0] = data.dim(0) - 0.5;
      bounds[1] = data.dim(1) - 0.5;
      bounds[2] = data.dim(2) - 0.5;

      Math::Matrix<float> M (4,4);
      set (S2V, Transform::scanner2voxel (M, data));
      set (V2S, Transform::voxel2scanner (M, data));
      set (I2S, Transform::image2scanner (M, data));
      set (S2I, Transform::scanner2image (M, data));
    }



    template <class Set> inline Point Interp<Set>::set_fractions (const Point& pos)
    {
      if (pos[0] < -0.5 || pos[0] > bounds[0] || 
          pos[1] < -0.5 || pos[1] > bounds[1] || 
          pos[2] < -0.5 || pos[2] > bounds[2]) {
        out_of_bounds = true;
        return (Point (NAN, NAN, NAN));
      }

      out_of_bounds = false;
      data[0] = pos[0];
      data[1] = pos[1];
      data[2] = pos[2];

      return (Point (pos[0]-data[0], pos[1]-data[1], pos[2]-data[2]));
    }






    template <class Set> inline bool Interp<Set>::voxel (const Point& pos)
    {
      Point f = set_fractions (pos);
      if (out_of_bounds) return (true);

      if (pos[0] < 0.0) { f[0] = 0.0; data[0] = 0; }
      else if (pos[0] > bounds[0]-0.5) f[0] = 0.0;

      if (pos[1] < 0.0) { f[1] = 0.0; data[1] = 0; }
      else if (pos[1] > bounds[1]-0.5) f[1] = 0.0;

      if (pos[2] < 0.0) { f[2] = 0.0; data[2] = 0; }
      else if (pos[2] > bounds[2]-0.5) f[2] = 0.0;

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










  }
}

#endif

