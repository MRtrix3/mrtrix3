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

#ifndef __mrview_colourmap_h__
#define __mrview_colourmap_h__

#include "image/interp.h"

#define COLOURMAP_MAX_SCALAR_INDEX 4
#define COLOURMAP_RGB 100
#define COLOURMAP_COMPLEX 101

namespace MR {
  namespace Viewer {

    inline uint8_t clamp (float val)
    {
      if (val < 0.0) return (0);
      if (val > 255.0) return (255);
      return ((uint8_t) (val+0.5)); 
    }





    namespace ColourMap {

      void   map (uint index, float val, uint8_t* RGB);

      void   grey (float val, uint8_t* RGB);
      void   hot  (float val, uint8_t* RGB);
      void   cool (float val, uint8_t* RGB);
      void   jet  (float val, uint8_t* RGB);
      void   RGB (float val[3], uint8_t* RGB);
      void   Z (float re, float im, uint8_t* RGB);


      void   get (uint mode, MR::Image::Position& image, MR::Image::OutputType format, float* val);
      void   get (uint mode, MR::Image::Interp& interp, MR::Image::OutputType format, float* val);
      void   map (uint mode, const Scaling& scale, MR::Image::Position& image, MR::Image::OutputType format, uint8_t* RGB);
      void   map (uint mode, const Scaling& scale, MR::Image::Interp& interp, MR::Image::OutputType format, uint8_t* RGB);









      inline void   map (uint index, float val, uint8_t* RGB)
      {
        switch (index) {
          case 0: grey (val, RGB); break;
          case 1: hot (val, RGB); break;
          case 2: cool (val, RGB); break;
          case 3: jet (val, RGB); break;
          default: RGB[0] = RGB[1] = RGB[2] = 0; break;
        }
      }





      inline void   grey (float val, uint8_t* RGB) { RGB[0] = RGB[1] = RGB[2] = clamp (val); }



      inline void   hot  (float val, uint8_t* RGB)
      {
        RGB[0] = clamp (2.7213*val);
        RGB[1] = clamp (2.7213*(val-94.1));
        RGB[2] = clamp (3.7727*(val-188.1));
      }



      inline void   cool (float val, uint8_t* RGB)
      {
        RGB[0] = clamp (2.0*(val-64.0));
        if (val < 128.0) {
          RGB[1] = clamp (2.0*(val < 64.0 ? val : 128.0-val));
          RGB[2] = clamp (2.0*val);
        }
        else {
          RGB[1] = clamp (2.0*(val-128.0));
          RGB[2] = clamp (4.0*(val < 196.0 ? 196.0-val : val-196.0));
        }
      }




      inline void jet  (float val, uint8_t* RGB)
      {
        RGB[0] = clamp (4.0*(val < 192.0 ? val-96.0 : 288.0-val));
        RGB[1] = clamp (4.0*(val < 128.0 ? val-32.0 : 224.0-val));
        RGB[2] = clamp (4.0*(val < 64.0  ? val+32.0 : 160.0-val));
      }




      inline void RGB (float val[3], uint8_t* RGB)
      {
        RGB[0] = clamp (val[0]);
        RGB[1] = clamp (val[1]);
        RGB[2] = clamp (val[2]);
      }




      inline void Z (float re, float im, uint8_t* RGB)
      {
        float r = sqrt (re*re + im*im);
        float p = atan2f (im, re);
        RGB[0] = clamp (r*fabs(p + 2.0*M_PI/3.0));
        RGB[1] = clamp (r*fabs(p));
        RGB[2] = clamp (r*fabs(p - 2.0*M_PI/3.0));
      }







      inline void get (uint mode, MR::Image::Position& image, MR::Image::OutputType format, float* val)
      {
        val[1] = val[2] = NAN;
        if (mode == COLOURMAP_COMPLEX) format = MR::Image::RealImag;
        image.get (format, val[0], val[1]);

        if (mode == COLOURMAP_RGB) {
          val[0] = fabs (val[0]);
          val[1] = val[2] = 0.0;
          if (image.ndim() > 3) {
            int pos = image[3];
            if (image[3] < (int) image.dim(3)-1) {
              float im;
              image.inc(3);
              image.get (format, val[1], im);
              if (image[3] < (int) image.dim(3)-1) {
                image.inc(3);
                image.get (format, val[2], im);
              }
            }
            image.set(3, pos);
            val[1] = fabs (val[1]);
            val[2] = fabs (val[2]);
          }
        }
      }





      inline void get (uint mode, MR::Image::Interp& interp, MR::Image::OutputType format, float* val)
      {
        if (mode == COLOURMAP_RGB) {
          interp.abs (format, val[0], val[1]);
          val[1] = val[2] = 0.0;
          if (interp.ndim() > 3) {
            int pos = interp[3];
            if (interp[3] < (int) interp.dim(3)) {
              float im;
              interp.inc(3);
              interp.abs (format, val[1], im);
              if (interp[3] < (int) interp.dim(3)) {
                interp.inc(3);
                interp.abs (format, val[2], im);
              }
            }
            interp.set(3, pos);
          }
        }
        else {
          val[1] = val[2] = NAN;
          if (mode == COLOURMAP_COMPLEX) format = MR::Image::RealImag;
          interp.get (format, val[0], val[1]);
        }
      }






      inline void map (uint mode, const Scaling& scale, MR::Image::Position& image, MR::Image::OutputType format, uint8_t* rgb)
      {
        float val[3];
        get (mode, image, format, val);
        if (mode < COLOURMAP_MAX_SCALAR_INDEX) map (mode, scale (val[0]), rgb);
        else if (mode == COLOURMAP_RGB) {
          val[0] = scale (val[0]);
          val[1] = scale (val[1]);
          val[2] = scale (val[2]);
          RGB (val, rgb);
        }
        else if (mode == COLOURMAP_COMPLEX) Z (scale (val[0]), scale (val[1]), rgb);
      }






      inline void map (uint mode, const Scaling& scale, MR::Image::Interp& interp, MR::Image::OutputType format, uint8_t* rgb)
      {
        float val[3];
        get (mode, interp, format, val);
        if (mode < COLOURMAP_MAX_SCALAR_INDEX) map (mode, scale (val[0]), rgb);
        else if (mode == COLOURMAP_RGB) {
          val[0] = scale (val[0]);
          val[1] = scale (val[1]);
          val[2] = scale (val[2]);
          RGB (val, rgb);
        }
        else if (mode == COLOURMAP_COMPLEX) Z (scale (val[0]), scale (val[1]), rgb);
      }


    }
  }
}

#endif




