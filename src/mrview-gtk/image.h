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

#ifndef __mrview_image_h__
#define __mrview_image_h__

#include "ptr.h"
#include "math/quaternion.h"
#include "image/object.h"
#include "image/interp.h"
#include "mrview/scaling.h"

namespace MR {
  namespace Viewer {

    class Image 
    {
      public:
        Image () :
          colourmap (0), 
          format (MR::Image::Real), 
          orientation (NAN, NAN, NAN, NAN), 
          projection (2),
          interpolate (true) { memset (channel, 0, MRTRIX_MAX_NDIMS*sizeof(int)); }

        Image (RefPtr<MR::Image::Object> I) :
          colourmap (0), 
          format (MR::Image::Real), 
          orientation (NAN, NAN, NAN, NAN), 
          projection (2),
          interpolate (true) { memset (channel, 0, MRTRIX_MAX_NDIMS*sizeof(int)); set (I); }

        RefPtr<MR::Image::Object> image;
        mutable Ptr<MR::Image::Interp> interp;

        int                   colourmap;
        MR::Image::OutputType format;
        Scaling               scaling;
        int                   channel[MRTRIX_MAX_NDIMS];
        Math::Quaternion      orientation;
        uint                 projection;
        Point                 focus;
        bool                  interpolate;

        void                  set (RefPtr<MR::Image::Object> I);

        operator bool () const              { return (image); }
        bool operator! () const             { return (!image); }

        bool operator== (const Image& I) const { return (image == I.image); }
        bool operator!= (const Image& I) const { return (image != I.image); }

        void    span_vectors (float pix[2], Point& vx, Point& vy);
        void    get_bounds (float xbounds[2], float ybounds[2], const Point& vx, const Point& vy, const Point& pos) const;
        void    vox_vector (Point& dest, const Point& src);

      protected:
        float V[3][3];

        friend std::ostream& operator<< (std::ostream& stream, const Image& ima);
    };





    inline void Image::vox_vector (Point& dest, const Point& src)
    {   
      dest[0] = V[0][0]*src[0] + V[0][1]*src[1] + V[0][2]*src[2];
      dest[1] = V[1][0]*src[0] + V[1][1]*src[1] + V[1][2]*src[2]; 
      dest[2] = V[2][0]*src[0] + V[2][1]*src[1] + V[2][2]*src[2]; 
    }   




    inline std::ostream& operator<< (std::ostream& stream, const Image& ima)
    {
      if (ima.image) stream << *ima.image;
      else stream << "(null)";
      return (stream);
    }


  }
}

#endif


