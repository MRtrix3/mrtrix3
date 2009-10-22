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

#include "image/interp.h"

namespace MR {
  namespace Image {

    //! \addtogroup Image
    // @{

    /*! \brief This class provides access to the voxel intensities of an image, using tri-cubic interpolation.
     *
     * Usage is identical to MR::Image::Interp.
     * \todo these need to be properly implemented and tested.  */
    class InterpCubic : public Interp {
      public:
        //! construct an InterpCubic object to point to the data contained in the MR::Image::Object \p parent
        /*! All non-spatial coordinates (i.e. axes > 3) will be initialised to zero. 
         * The spatial coordinates are left undefined. */
        InterpCubic (Object& parent) : Interp (parent) { }
        ~InterpCubic() { }

        bool P (const Point& pos);
        bool I (const Point& pos) { return (P (I2P (pos))); }
        bool R (const Point& pos) { return (P (R2P (pos))); }

        float value () const { return (re()); }
        float re () const;
        float im () const;

        float re_abs () const;
        float im_abs () const;

        void  get (OutputType format, float& val, float& val_im);
        void  abs (OutputType format, float& val, float& val_im);

      protected:
        float fx[4], fy[4], fz[4];

        void  set_coefs (float x, float* coefs) 
        {
          coefs[0] = -x*(x-1.0)*(x-2.0)/6.0; 
          coefs[1] = 0.5*(x+1.0)*(x-1.0)*(x-2.0); 
          coefs[2] = -0.5*(x+1.0)*x*(x-2.0);
          coefs[3] = (x+1.0)*x*(x-1.0)/6.0;
        }
        float cubic_interp (const float* coefs, const float* values) const 
        {
          return (coefs[0]*values[0] + coefs[1]*values[1] + coefs[2]*values[2] + coefs[3]*values[3]); 
        }
    };

    //! @}







    inline bool InterpCubic::P (const Point& pos)
    {
      Point f = set_fractions (pos);
      if (out_of_bounds) return (true);

      set_coefs (f[0], fx);
      set_coefs (f[1], fy);
      set_coefs (f[2], fz);

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


    inline void InterpCubic::get (OutputType format, float& val, float& val_im)
    {
      if (out_of_bounds) { val = val_im = NAN; return; }
      switch (format) {
        case Default:   val = re(); return;
        case Real:      val = re(); return;
        case Imaginary: val = im(); return;
        case Magnitude: val = re(); val_im = im(); val = sqrt (val*val + val_im*val_im); return;
        case Phase:     val = re(); val_im = im(); val = atan2 (val_im, val); return;
        case RealImag:  val = re(); val_im = im(); return;
      }
      assert (false);
    }





    inline void InterpCubic::abs (OutputType format, float& val, float& val_im)
    {
      if (out_of_bounds) { val = val_im = NAN; return; }
      switch (format) {
        case Default:   val = re_abs(); return;
        case Real:      val = re_abs(); return;
        case Imaginary: val = im_abs(); return;
        case Magnitude: val = re_abs(); val_im = im_abs(); val = sqrt (val*val + val_im*val_im); return;
        case Phase:     val = re_abs(); val_im = im_abs(); val = atan2 (val_im, val); return;
        case RealImag:  val = re_abs(); val_im = im_abs(); return;
      }
      assert (false);
    }

  }
}

#endif


