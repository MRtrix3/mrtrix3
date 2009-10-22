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

#ifndef __image_interp_h__
#define __image_interp_h__

#include "point.h"
#include "image/transform.h"

namespace MR {
  namespace Image {

    //! \addtogroup Image
    // @{

    //! This class provides access to the voxel intensities of a data set, using tri-linear interpolation.
    /*! Interpolation is only performed along the first 3 (spatial) axes. 
     * The (integer) position along the remaining axes should be set using the
     * template DataSet class.
     * The spatial coordinates can be set using the functions P(), I(), and R(). 
     * For example:
     * \code
     * Image::Voxel voxel (image);
     * Image::Interp<Image::Voxel> interp (voxel);  // create an Interp object using voxel as the parent data set
     * interp.R (10.2, 3.59, 54.1);   // set the real-space position to [ 10.2 3.59 54.1 ]
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
     * float re = voxel.real();    // for complex data
     * float im = voxel.imag();    // for complex data
     * Math::Transform<float> M = voxel.transform; // a valid 4x4 transformation matrix
     * \endcode
     */

    template <class DataSet> class Interp {
      public:
        //! construct an Interp object to obtain interpolated values using the
        // parent DataSet class 
        Interp (DataSet& parent);
        ~Interp() { }

        //! test whether current position is within bounds.
        /*! \return true if the current position is out of bounds, false otherwise */
        bool  operator! () const { return (out_of_bounds); }

        //! Set the current position to <b>pixel space</b> position \a pos
        /*! This will set the position from which the image intensity values will
         * be interpolated, assuming that \a pos provides the position as a
         * (floating-point) pixel coordinate within the dataset. */
        bool P (const Point& pos);
        //! Set the current position to <b>image space</b> position \a pos
        /*! This will set the position from which the image intensity values will
         * be interpolated, assuming that \a pos provides the position as a
         * coordinate relative to the axes of the dataset, in units of
         * millimeters. The origin is taken to be the centre of the voxel at [
         * 0 0 0 ]. */
        bool I (const Point& pos) { return (P (I2P (pos))); }
        //! Set the current position to the <b>real space</b> position \a pos
        /*! This will set the position from which the image intensity values will
         * be interpolated, assuming that \a pos provides the position as a
         * real space coordinate, in units of millimeters. */
        bool R (const Point& pos) { return (P (R2P (pos))); }

        float value () const { return (Interp::real()); }
        float real () const;
        float imag () const;

        float real_abs () const;
        float imag_abs () const;

        //! Transform the position \p r from real-space to pixel-space
        Point R2P (const Point& r) const { return (transform (RP, r)); }
        //! Transform the position \p r from pixel-space to real-space
        Point P2R (const Point& r) const { return (transform (PR, r)); }
        //! Transform the position \p r from image-space to pixel-space
        Point I2P (const Point& r) const { return (Point (r[0]/data.vox(0), r[1]/data.vox(1), r[2]/data.vox(2))); }
        //! Transform the position \p r from pixel-space to image-space
        Point P2I (const Point& r) const { return (Point (r[0]*data.vox(0), r[1]*data.vox(1), r[2]*data.vox(2))); }
        //! Transform the position \p r from image-space to real-space
        Point I2R (const Point& r) const { return (transform (IR, r)); }
        //! Transform the position \p r from real-space to image-space
        Point R2I (const Point& r) const { return (transform (RI, r)); }

        //! Transform the orientation \p r from real-space to pixel-space
        Point vec_R2P (const Point& r) const { return (transform_vector (RP, r)); }
        //! Transform the orientation \p r from pixel-space to real-space
        Point vec_P2R (const Point& r) const { return (transform_vector (PR, r)); }

      private:
        DataSet& data;
        float  RP[3][4], PR[3][4], IR[3][4], RI[3][4];
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

        template <class U> void set (float M[3][4], const Math::MatrixView<U>& MV) {
          M[0][0] = MV(0,0); M[0][1] = MV(0,1); M[0][2] = MV(0,2); M[0][3] = MV(0,3);
          M[1][0] = MV(1,0); M[1][1] = MV(1,1); M[1][2] = MV(1,2); M[1][3] = MV(1,3);
          M[2][0] = MV(2,0); M[2][1] = MV(2,1); M[2][2] = MV(2,2); M[2][3] = MV(2,3);
        }
        Point set_fractions (const Point& pos);
    };

    //! @}







    template <class DataSet> Interp<DataSet>::Interp (DataSet& parent) : data (parent), out_of_bounds (true)
    { 
      bounds[0] = data.dim(0) - 0.5;
      bounds[1] = data.dim(1) - 0.5;
      bounds[2] = data.dim(2) - 0.5;

      Math::Matrix<float> M (4,4);
      set (RP, Transform::R2P (M, data));
      set (PR, Transform::P2R (M, data));
      set (IR, Transform::I2R (M, data));
      set (RI, Transform::R2I (M, data));
    }



    template <class DataSet> inline Point Interp<DataSet>::set_fractions (const Point& pos)
    {
      if (pos[0] < -0.5 || pos[0] > bounds[0] || 
          pos[1] < -0.5 || pos[1] > bounds[1] || 
          pos[2] < -0.5 || pos[2] > bounds[2]) {
        out_of_bounds = true;
        return (Point (NAN, NAN, NAN));
      }

      out_of_bounds = false;
      data[0] = int (pos[0]);
      data[1] = int (pos[1]);
      data[2] = int (pos[2]);

      return (Point (pos[0]-data[0], pos[1]-data[1], pos[2]-data[2]));
    }






    template <class DataSet> inline bool Interp<DataSet>::P (const Point& pos)
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









    template <class DataSet> inline float Interp<DataSet>::real () const
    {
      if (out_of_bounds) return (NAN);
      float val = 0.0;
      if (faaa) val  = faaa * data.real (); data[2]++;
      if (faab) val += faab * data.real (); data[1]++;
      if (fabb) val += fabb * data.real (); data[2]--;
      if (faba) val += faba * data.real (); data[0]++;
      if (fbba) val += fbba * data.real (); data[1]--;
      if (fbaa) val += fbaa * data.real (); data[2]++;
      if (fbab) val += fbab * data.real (); data[1]++;
      if (fbbb) val += fbbb * data.real (); data[0]--; data[1]--; data[2]--;
      return (val);
    }





    template <class DataSet> inline float Interp<DataSet>::imag () const
    {
      if (out_of_bounds) return (NAN);
      float val = 0.0;
      if (faaa) val  = faaa * data.imag (); data[2]++;
      if (faab) val += faab * data.imag (); data[1]++;
      if (fabb) val += fabb * data.imag (); data[2]--;
      if (faba) val += faba * data.imag (); data[0]++;
      if (fbba) val += fbba * data.imag (); data[1]--;
      if (fbaa) val += fbaa * data.imag (); data[2]++;
      if (fbab) val += fbab * data.imag (); data[1]++;
      if (fbbb) val += fbbb * data.imag (); data[0]--; data[1]--; data[2]--;
      return (val);
    }




    template <class DataSet> inline float Interp<DataSet>::real_abs () const
    {
      if (out_of_bounds) return (NAN);
      float val = 0.0;
      if (faaa) val  = faaa * Math::abs (data.real()); data[2]++;
      if (faab) val += faab * Math::abs (data.real()); data[1]++;
      if (fabb) val += fabb * Math::abs (data.real()); data[2]--;
      if (faba) val += faba * Math::abs (data.real()); data[0]++;
      if (fbba) val += fbba * Math::abs (data.real()); data[1]--;
      if (fbaa) val += fbaa * Math::abs (data.real()); data[2]++;
      if (fbab) val += fbab * Math::abs (data.real()); data[1]++;
      if (fbbb) val += fbbb * Math::abs (data.real()); data[0]--; data[1]--; data[2]--;
      return (val);
    }





    template <class DataSet> inline float Interp<DataSet>::imag_abs () const
    {
      if (out_of_bounds) return (NAN);
      float val = 0.0;
      if (faaa) val  = faaa * Math::abs (data.imag()); data[2]++;
      if (faab) val += faab * Math::abs (data.imag()); data[1]++;
      if (fabb) val += fabb * Math::abs (data.imag()); data[2]--;
      if (faba) val += faba * Math::abs (data.imag()); data[0]++;
      if (fbba) val += fbba * Math::abs (data.imag()); data[1]--;
      if (fbaa) val += fbaa * Math::abs (data.imag()); data[2]++;
      if (fbab) val += fbab * Math::abs (data.imag()); data[1]++;
      if (fbbb) val += fbbb * Math::abs (data.imag()); data[0]--; data[1]--; data[2]--;
      return (val);
    }


  }
}

#endif

