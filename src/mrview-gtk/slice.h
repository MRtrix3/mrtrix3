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

#ifndef __mrview_slice_h__
#define __mrview_slice_h__

#include "point.h"
#include "math/quaternion.h"
#include "image/object.h"
#include "mrview/texture.h"
#include "mrview/image.h"
#include "mrview/scaling.h"


namespace MR {
  namespace Viewer {
    class Pane;
    namespace Slice {
      class Current;

      class Info {
        public:
          Info () : 
            image (NULL), 
            colourmap (0), 
            format (MR::Image::Real), 
            orientation (NAN, NAN, NAN, NAN), 
            projection (2),
            interpolate (true) { memset (channel, 0, MRTRIX_MAX_NDIMS*sizeof(int)); }
          Info (Current& S);
          Info (Pane& P);

          RefPtr<Image>         image;
          int                   colourmap;
          MR::Image::OutputType format;
          Scaling               scaling;
          int                   channel[MRTRIX_MAX_NDIMS];
          Math::Quaternion      orientation;
          uint                 projection;
          Point                 focus;
          bool                  interpolate;

          bool operator== (const Info& S) const { return (!operator!= (S)); }
          bool operator!= (const Info& S) const;

          bool operator== (const Current& S) const { return (!operator!= (S)); }
          bool operator!= (const Current& S) const;

          bool same_channel (const int* C, uint up_to_dim) const { return (memcmp (channel+3, C+3, (up_to_dim-3)*sizeof(int)) == 0); }
      };




      class Current {
        public:
          Current (Pane& P);
          Current (Info& S);

          RefPtr<Image>&         image;
          int&                   colourmap;
          MR::Image::OutputType& format;
          Scaling&               scaling;
          int*                   channel;
          Math::Quaternion&      orientation;
          uint&                 projection;
          Point&                 focus;
          bool&                  interpolate;

          bool operator== (const Info& S) const { return (S == *this); }
          bool operator!= (const Info& S) const { return (S != *this); }
      };




      class Renderer {
        public:
          Renderer (bool is_RGB = true) : tex (is_RGB) { }

          void            update (Current& S);
          void            draw ();
          const float*    projection_matrix () const { return (cached.T); }
          void            focus_to_image_plane (Point& p) const;


        protected:
          Texture     tex;
          Info        slice;

          class Settings { 
            public:
              Point corners[4];
              float T[16];
              int   dim[2];
              int   slice;
              Point vx, vy, anchor;
          };
          Settings cached;

          void calculate_traversal_vectors ();
          void calculate_projection_matrix ();
          void update_scaling_free (Scaling& scaling);
          void update_scaling_fixed (Scaling& scaling);
          void update_texture_free ();
          void update_texture_fixed ();
      };







      class Source {
        public:
          typedef enum { WINDOW, PANE, IMAGE } Type;
          Source () : 
            image (PANE), colourmap (IMAGE), format (IMAGE), scaling (IMAGE), channel (IMAGE), orientation (PANE), 
            projection (PANE), focus (PANE), interpolate (IMAGE) { }

          Type image, colourmap, format, scaling, channel, orientation, projection, focus, interpolate;
          const char* str (Type t) 
          {
            switch (t) {
              case IMAGE: return ("[IMAGE]");
              case PANE:  return ("[PANE]");
              case WINDOW: return ("[WINDOW]");
            }
            return ("");
          }
      };




      inline Current::Current (Info& S) :
        image (S.image),
        colourmap (S.colourmap),
        format (S.format),
        scaling (S.scaling),
        channel (S.channel),
        orientation (S.orientation),
        projection (S.projection),
        focus (S.focus),
        interpolate (S.interpolate)
      {
      }


      inline bool Info::operator!= (const Info& S) const
      {
        if (!S.image) return (false);

        if (image != S.image) return (true);
        if (orientation != S.orientation) return (true);
        if (format != S.format) return (true);
        if (focus != S.focus) return (true);
        if (projection != S.projection) return (true);
        if (scaling != S.scaling) return (true);
        if (colourmap != S.colourmap) return (true);
        if (!same_channel (S.channel, image->interp->ndim())) return (true);
        if (interpolate != S.interpolate) return (true);

        return (false);
      }




      inline bool Info::operator!= (const Current& S) const
      {
        if (!S.image) return (false);

        if (image != S.image) return (true);
        if (orientation != S.orientation) return (true);
        if (format != S.format) return (true);
        if (focus != S.focus) return (true);
        if (projection != S.projection) return (true);
        if (scaling != S.scaling) return (true);
        if (colourmap != S.colourmap) return (true);
        if (!same_channel (S.channel, image->interp->ndim())) return (true);
        if (interpolate != S.interpolate) return (true);

        return (false);
      }




      inline void get_fixed_axes (uint proj, int& axis1, int& axis2)
      {
        switch (proj) {
          case 0: axis1 = 1; axis2 = 2; break;
          case 1: axis1 = 0; axis2 = 2; break;
          case 2: axis1 = 0; axis2 = 1; break;
          default: axis1 = -1; axis2 = -1; assert (false); return;
        }
      }




      inline void Renderer::focus_to_image_plane (Point& p) const
      {
        float n =
          cached.T[2]  * (p[0] - cached.corners[0][0]) +
          cached.T[6]  * (p[1] - cached.corners[0][1]) +
          cached.T[10] * (p[2] - cached.corners[0][2]);

        p[0] = n * cached.T[2]  - p[0]; 
        p[1] = n * cached.T[6]  - p[1];
        p[2] = n * cached.T[10] - p[2];
      }




      inline std::ostream& operator<< (std::ostream& stream, const Info& S) 
      {
        stream << "Image: " << ( S.image ? S.image->interp->name() : "NULL" );
        if (!S.image) return (stream);
        stream << ", colourmap: " << S.colourmap
          << ", format " << S.format << ", scaling: " << S.scaling << ", focus: " << S.focus << ", orientation: " << S.orientation 
          << ", projection: " << S.projection << ", interp: " << S.interpolate << ", channel: [ ";
        for (int n = 3; n < S.image->interp->ndim(); n++) stream << S.channel[n] << " ";
        stream << "]";
        return (stream); 
      }

      inline std::ostream& operator<< (std::ostream& stream, const Current& S) 
      {
        stream << "Image: " << ( S.image ? S.image->interp->name() : "NULL" );
        if (!S.image) return (stream);
        stream << ", colourmap: " << S.colourmap
          << ", format " << S.format << ", scaling: " << S.scaling << ", focus: " << S.focus << ", orientation: " << S.orientation 
          << ", projection: " << S.projection << ", interp: " << S.interpolate << ", channel: [ ";
        for (int n = 3; n < S.image->interp->ndim(); n++) stream << S.channel[n] << " ";
        stream << "]";
        return (stream); 
      }

    }
  }
}

#endif



