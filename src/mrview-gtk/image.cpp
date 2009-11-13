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


    18-12-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * fix random crashes in MRView when the "View->snap to image axes" option is unticked.

*/

#include "math/linalg.h"
#include "mrview/image.h"
#include "mrview/window.h"
#include "mrview/colourmap.h"

namespace MR {
  namespace Viewer {

    namespace {
      inline void sym_eig_2D (float ev[2], Point& vx, Point& vy, const float D[3])
      {
        float a = D[0] + D[1];
        float b = a*a - 4.0*(D[0]*D[1] - D[2]*D[2]);
        a *= 0.5;
        b = b > 0.0 ? 0.5 * sqrt(b) : 0.0;

        float norm;
        if (fabs (b) < 1e-5) {
          ev[0] = 1.0;
          ev[1] = 0.0;
        }
        else {
          ev[0] = D[2];
          ev[1] = D[0] - a - b;
          norm = sqrt (ev[0]*ev[0] + ev[1]*ev[1]);
          ev[0] /= norm; ev[1] /= norm;
        }

        // these vectors now span the image plane in real space:
        norm  = ev[0]*vx[0] - ev[1]*vy[0];
        vy[0] = ev[1]*vx[0] + ev[0]*vy[0];
        vx[0] = norm;

        norm  = ev[0]*vx[1] - ev[1]*vy[1];
        vy[1] = ev[1]*vx[1] + ev[0]*vy[1];
        vx[1] = norm;

        norm  = ev[0]*vx[2] - ev[1]*vy[2];
        vy[2] = ev[1]*vx[2] + ev[0]*vy[2];
        vx[2] = norm;

        // these are the corresponding eigenvalues / pixel sizes:
        ev[0] = a+b;
        ev[1] = a-b;
      }
    }





    void Image::span_vectors (float pix[2], Point& vx, Point& vy)
    {   
      Point p;
      vox_vector (p, vx);

      float D[3];
      D[0] = vx.dot (p);

      vox_vector (p, vy);
      D[1] = vy.dot (p); 
      D[2] = vx.dot (p); 

      sym_eig_2D (pix, vx, vy, D); 
    }   





    void Image::get_bounds (float xbounds[2], float ybounds[2], const Point& vx, const Point& vy, const Point& pos) const
    {   
      xbounds[0] = ybounds[0] = INFINITY;
      xbounds[1] = ybounds[1] = -INFINITY;

      Math::Matrix M (3,3);
      Math::Vector x(3), y(3);

      for (uint m = 0; m < 3; m++) {
        Point axis_vec_pix (0.0, 0.0, 0.0);
        axis_vec_pix[m] = image->dim(m)-1;
        Point axis_vec_real (interp->vec_P2R (axis_vec_pix));
        int axis1 = (m+1) % 3;
        int axis2 = (m+2) % 3;

        for (uint n = 0; n < 4; n++) {
          M(0,0) =            vx[0]; M(1,0) =            vx[1]; M(2,0) =            vx[2];
          M(0,1) =            vy[0]; M(1,1) =            vy[1]; M(2,1) =            vy[2];
          M(0,2) = axis_vec_real[0]; M(1,2) = axis_vec_real[1]; M(2,2) = axis_vec_real[2];


          Point ref_point_pix (0.0, 0.0, 0.0);
          if (n%2) ref_point_pix[axis1] = image->dim (axis1) - 1;
          if (n/2) ref_point_pix[axis2] = image->dim (axis2) - 1;

          Point ref_point_real (interp->P2R (ref_point_pix));

          y[0] = ref_point_real[0] - pos[0];
          y[1] = ref_point_real[1] - pos[1];
          y[2] = ref_point_real[2] - pos[2];
          Math::QR_solve (M, y, x);

          if (x[2] <= 0.0 && x[2] >= -1.0) {
            if (x[0] < xbounds[0]) xbounds[0] = x[0];
            if (x[0] > xbounds[1]) xbounds[1] = x[0];
            if (x[1] < ybounds[0]) ybounds[0] = x[1];
            if (x[1] > ybounds[1]) ybounds[1] = x[1];
          }
        }
      }

      if (xbounds[0] > xbounds[1]) xbounds[0] = xbounds[1] = 0.0;
      if (ybounds[0] > ybounds[1]) ybounds[0] = ybounds[1] = 0.0;
    }







    void Image::set (RefPtr<MR::Image::Object> I) 
    {
      image = I;
      interp = new MR::Image::Interp (*image); 

      const Math::Matrix& A (image->header().P2R());
      const Math::Matrix& B (image->header().I2R());
      V[0][0] = A(0,0)*B(0,0) + A(0,1)*B(0,1) + A(0,2)*B(0,2);
      V[0][1] = A(0,0)*B(1,0) + A(0,1)*B(1,1) + A(0,2)*B(1,2);
      V[0][2] = A(0,0)*B(2,0) + A(0,1)*B(2,1) + A(0,2)*B(2,2);
      V[1][0] = A(1,0)*B(0,0) + A(1,1)*B(0,1) + A(1,2)*B(0,2);
      V[1][1] = A(1,0)*B(1,0) + A(1,1)*B(1,1) + A(1,2)*B(1,2);
      V[1][2] = A(1,0)*B(2,0) + A(1,1)*B(2,1) + A(1,2)*B(2,2);
      V[2][0] = A(2,0)*B(0,0) + A(2,1)*B(0,1) + A(2,2)*B(0,2);
      V[2][1] = A(2,0)*B(1,0) + A(2,1)*B(1,1) + A(2,2)*B(1,2);
      V[2][2] = A(2,0)*B(2,0) + A(2,1)*B(2,1) + A(2,2)*B(2,2);

      projection = minindex (image->dim(0)*image->vox(0), image->dim(1)*image->vox(1), image->dim(2)*image->vox(2));
      focus = interp->P2R (Point (image->dim(0)/2.0, image->dim(1)/2.0, image->dim(2)/2.0));
      if (image->ndim() == 4)
        if (image->dim(3) == 3)
          colourmap = COLOURMAP_RGB;
    }

  }
}

