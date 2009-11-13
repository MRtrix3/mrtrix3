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

#include "mrview/slice.h"
#include "mrview/pane.h"
#include "mrview/window.h"
#include "mrview/colourmap.h"

namespace MR {
  namespace Viewer {
    namespace Slice {

    void Renderer::update (Current& S)
    {
      MR::Image::Interp &I (*S.image->interp);
      bool update_texture (S.orientation != slice.orientation);

      if (update_texture || S.image != slice.image || S.projection != slice.projection) {
        slice.image = S.image; 
        slice.orientation = S.orientation;
        slice.projection = S.projection;
        slice.focus = S.focus;
        update_texture = true;
        calculate_projection_matrix();
      }

      if (S.focus != slice.focus) {
        slice.focus = S.focus;
        update_texture = true;
        calculate_traversal_vectors();
      }

      if (!slice.same_channel (S.channel, I.ndim())) {
        for (int n = 3; n < I.ndim(); n++) { 
          slice.channel[n] = S.channel[n];
          if (slice.channel[n] < 0) slice.channel[n] = 0;
          if (slice.channel[n] >= I.dim(n)) slice.channel[n] = I.dim(n)-1;
        }
        update_texture = true;
      }


      if (S.format != slice.format || S.focus != slice.focus || S.colourmap != slice.colourmap) {
        slice.format = S.format;
        slice.focus = S.focus;
        slice.colourmap = S.colourmap;
        update_texture = true;
      }


      if (S.scaling != slice.scaling) update_texture = true;

      if (update_texture) {
        tex.allocate (MAX (cached.dim[0], cached.dim[1]));
        tex.clear();

        I = slice.channel;
        if (!S.scaling && tex.is_rgba()) {
          if (slice.orientation) update_scaling_free (S.scaling);
          else update_scaling_fixed (S.scaling);
        }
        slice.scaling = S.scaling;

        if (slice.orientation) update_texture_free();
        else update_texture_fixed();
      }

      slice.interpolate = S.interpolate;
    }





    void Renderer::draw ()
    {
      float wtex = cached.dim[0] / (float) tex.width();
      float htex = cached.dim[1] / (float) tex.width();

      tex.select();

      int i = slice.interpolate ? GL_LINEAR : GL_NEAREST;
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, i);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, i);

      glBegin (GL_QUADS);
      glTexCoord2f ( 0.0,  0.0); glVertex3fv (cached.corners[0].get());
      glTexCoord2f ( 0.0, htex); glVertex3fv (cached.corners[1].get());
      glTexCoord2f (wtex, htex); glVertex3fv (cached.corners[2].get());
      glTexCoord2f (wtex,  0.0); glVertex3fv (cached.corners[3].get());
      glEnd();
    }










    void Renderer::calculate_projection_matrix ()
    {
      MR::Image::Interp &I (*slice.image->interp);
      if (slice.orientation) {
        const Math::Quaternion& R (slice.orientation);

        float a2 = R[0]*R[0];
        float b2 = R[1]*R[1];
        float c2 = R[2]*R[2];
        float d2 = R[3]*R[3];
        float ab = 2.0*R[0]*R[1];
        float ac = 2.0*R[0]*R[2];
        float ad = 2.0*R[0]*R[3];
        float bc = 2.0*R[1]*R[2];
        float bd = 2.0*R[1]*R[3];
        float cd = 2.0*R[2]*R[3];

        cached.T[0] = a2 + b2 - c2 - d2;
        cached.T[1] = bc - ad;
        cached.T[2] = ac + bd;

        cached.T[4] = ad + bc;
        cached.T[5] = a2 - b2 + c2 - d2;
        cached.T[6] = cd - ab;

        cached.T[8] = bd - ac;
        cached.T[9] = ab + cd;
        cached.T[10] = a2 - b2 - c2 + d2;

        cached.T[3] = cached.T[7] = cached.T[11] = cached.T[12] = cached.T[13] = cached.T[14] = 0.0;
        cached.T[15] = 1.0;
         
        /*
        std::cerr << cached.T[0] << "\t" << cached.T[1] << "\t" << cached.T[2] << "\t" << cached.T[3] << "\n";
        std::cerr << cached.T[4] << "\t" << cached.T[5] << "\t" << cached.T[6] << "\t" << cached.T[7] << "\n";
        std::cerr << cached.T[8] << "\t" << cached.T[9] << "\t" << cached.T[10] << "\t" << cached.T[11] << "\n";
        std::cerr << cached.T[12] << "\t" << cached.T[13] << "\t" << cached.T[14] << "\t" << cached.T[15] << "\n";
        */
      }
      else {

        const Math::Matrix &I2R (I.image.header().I2R());
        cached.T[0]  = -I2R(0,0); cached.T[1]  = I2R(0,1); cached.T[2]  = -I2R(0,2); cached.T[3]  = 0.0;
        cached.T[4]  = -I2R(1,0); cached.T[5]  = I2R(1,1); cached.T[6]  = -I2R(1,2); cached.T[7]  = 0.0;
        cached.T[8]  = -I2R(2,0); cached.T[9]  = I2R(2,1); cached.T[10] = -I2R(2,2); cached.T[11] = 0.0;
        cached.T[12] =       0.0; cached.T[13] =      0.0; cached.T[14] =       0.0; cached.T[15] = 1.0;

      }

      if (slice.projection == 0) {
        for (uint n = 0; n < 3; n++) {
          float f = cached.T[4*n];
          cached.T[4*n] = -cached.T[4*n+1];
          cached.T[4*n+1] = -cached.T[4*n+2];
          cached.T[4*n+2] = f;
        }
      }
      else if (slice.projection == 1) {
        for (uint n = 0; n < 3; n++) {
          float f = cached.T[4*n+1];
          cached.T[4*n+1] = -cached.T[4*n+2];
          cached.T[4*n+2] = f;
        }
      }

      calculate_traversal_vectors();
    }










    void Renderer::calculate_traversal_vectors ()
    {
      MR::Image::Interp &I (*slice.image->interp);
      if (slice.orientation) {

        Point vx ( cached.T[0], cached.T[4], cached.T[8] );
        Point vy ( cached.T[1], cached.T[5], cached.T[9] );
        float ev[2];

        slice.image->span_vectors (ev, vx, vy);

        float xbounds[2], ybounds[2];
        slice.image->get_bounds (xbounds, ybounds, vx, vy , slice.focus);

        cached.dim[0] = (uint) ((xbounds[1]-xbounds[0])/ev[0]) + 1;
        cached.dim[1] = (uint) ((ybounds[1]-ybounds[0])/ev[1]) + 1;

        cached.corners[0] = slice.focus + (xbounds[0]+0.5*ev[0]) * vx + (ybounds[0]+0.5*ev[1]) * vy;
        cached.corners[1] = cached.corners[0] + ev[1]*cached.dim[1] * vy;
        cached.corners[2] = cached.corners[1] + ev[0]*cached.dim[0] * vx;
        cached.corners[3] = cached.corners[0] + ev[0]*cached.dim[0] * vx;

        cached.vx = ev[0] * I.vec_R2P (vx); 
        cached.vy = ev[1] * I.vec_R2P (vy); 
        cached.anchor = I.R2P (cached.corners[0]) + 0.5 * (cached.vx + cached.vy);

      } 
      else {

        int ix, iy;
        get_fixed_axes (slice.projection, ix, iy);

        cached.dim[0] = I.dim (ix);
        cached.dim[1] = I.dim (iy);

        Point pix = I.R2P (slice.focus);
        cached.slice = round (pix[slice.projection]);

        pix[ix] = -0.5;
        pix[iy] = -0.5;
        pix[slice.projection] = cached.slice;

        cached.corners[0] = I.P2R (pix);

        pix[iy] = cached.dim[1] - 0.5;
        cached.corners[1] = I.P2R (pix);

        pix[ix] = cached.dim[0] - 0.5;
        cached.corners[2] = I.P2R (pix);

        pix[iy] = -0.5;
        cached.corners[3] = I.P2R (pix);
      }

    }




    void Renderer::update_scaling_free (Scaling& scaling)
    {
      MR::Image::Interp &I (*slice.image->interp);
      scaling.rescale_start ();
      float re = 0.0, im = 0.0;
      for (int y = 0; y < cached.dim[1]; y++) {
        for (int x = 0; x < cached.dim[0]; x++) {
          if (!I.P (cached.anchor + x*cached.vx + y*cached.vy)) {
            I.get (slice.format, re, im);
            scaling.rescale_add (re);
          }
        }
      }
      scaling.rescale_end ();
    }







    void Renderer::update_texture_free ()
    {
      MR::Image::Interp &I (*slice.image->interp);

      tex.allocate (MAX (cached.dim[0], cached.dim[1]));
      tex.clear();
      uint8_t RGB[] = { 0, 0, 0 };
      for (int y = 0; y < cached.dim[1]; y++) {
        for (int x = 0; x < cached.dim[0]; x++) {
          if (!I.P (cached.anchor + x*cached.vx + y*cached.vy)) {
            if (tex.is_rgba()) {
              ColourMap::map (slice.colourmap, slice.scaling, I, slice.format, RGB);
              tex.rgba(x,y).RGB (RGB[0], RGB[1], RGB[2]);
            }
            else tex.alpha(x,y) = I.value() > 0.5 ? 255 : 0;
          }
        }
      }
      tex.commit();
    }







    void Renderer::update_scaling_fixed (Scaling& scaling)
    {
      int ix, iy;
      get_fixed_axes (slice.projection, ix, iy);
      MR::Image::Position &pos (*slice.image->interp);

      if (cached.slice < 0 || cached.slice >= (int) pos.dim (slice.projection)) return;
      pos.set (slice.projection, cached.slice);

      scaling.rescale_start ();
      for (pos.set(iy,0); pos[iy] < cached.dim[1]; pos.inc(iy)) {
        for (pos.set(ix,0); pos[ix] < cached.dim[0]; pos.inc(ix)) {
          float val[3];
          ColourMap::get (slice.colourmap, pos, slice.format, val);
          scaling.rescale_add (val);
        }
      }
      scaling.rescale_end ();
    }








    void Renderer::update_texture_fixed ()
    {
      MR::Image::Position& pos (*slice.image->interp);
      pos.set (slice.projection, cached.slice);

      int ix, iy;
      get_fixed_axes (slice.projection, ix, iy);

      tex.allocate (MAX (cached.dim[0], cached.dim[1]));
      tex.clear();

      if (cached.slice >= 0 && cached.slice < (int) pos.dim (slice.projection)) {
        uint8_t RGB[] = { 0, 0, 0 };
        for (pos.set(iy,0); pos[iy] < cached.dim[1]; pos.inc(iy)) {
          for (pos.set(ix,0); pos[ix] < cached.dim[0]; pos.inc(ix)) {
            if (tex.is_rgba()) {
              ColourMap::map (slice.colourmap, slice.scaling, pos, slice.format, RGB);
              tex.rgba(pos[ix], pos[iy]).RGB (RGB[0], RGB[1], RGB[2]);
            }
            else tex.alpha(pos[ix], pos[iy]) = pos.value() > 0.5 ? 255 : 0;
          }
        }
      }
      tex.commit();
    }







      Current::Current (Pane& P) :
        image (P.source.image == Source::PANE ?  P.slice.image : Window::Main->slice.image),
        colourmap (P.source.colourmap == Source::IMAGE ?  image->colourmap : 
            ( P.source.colourmap == Source::PANE ?  P.slice.colourmap : Window::Main->slice.colourmap )),
        format (P.source.format == Source::IMAGE ?  image->format : 
            ( P.source.format == Source::PANE ?  P.slice.format : Window::Main->slice.format )),
        scaling (P.source.scaling == Source::IMAGE ?  image->scaling : 
            ( P.source.scaling == Source::PANE ?  P.slice.scaling : Window::Main->slice.scaling )),
        channel (P.source.channel == Source::IMAGE ? image->channel : 
            ( P.source.channel == Source::PANE ? P.slice.channel : Window::Main->slice.channel )),
        orientation (P.source.orientation == Source::IMAGE ? image->orientation : 
            ( P.source.orientation == Source::PANE ? P.slice.orientation : Window::Main->slice.orientation )),
        projection (P.source.projection == Source::IMAGE ? image->projection : 
            ( P.source.projection == Source::PANE ? P.slice.projection : Window::Main->slice.projection )),
        focus (P.source.focus == Source::IMAGE ? image->focus : 
            ( P.source.focus == Source::PANE ? P.slice.focus : Window::Main->slice.focus )),
        interpolate (P.source.interpolate == Source::IMAGE ? image->interpolate :
            ( P.source.interpolate == Source::PANE ? P.slice.interpolate : Window::Main->slice.interpolate ))
        {
          assert (P.source.image != Source::IMAGE);
          if (!image) return;
          MR::Image::Interp &I (*image->interp);

          if (projection > 2) projection = minindex (I.dim(0)*I.vox(0), I.dim(1)*I.vox(1), I.dim(2)*I.vox(2));
          if (!focus) focus = I.P2R (Point (I.dim(0)/2.0, I.dim(1)/2.0, I.dim(2)/2.0));
        }



    }
  }
}

