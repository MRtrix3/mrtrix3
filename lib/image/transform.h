/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 24/05/09.

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

#ifndef __image_transform_h__
#define __image_transform_h__

#include "math/matrix.h"
#include "math/LU.h"
#include "point.h"

namespace MR
{
  namespace Image
  {

    class Transform
    {
      public:

        //! An object for transforming between voxel, scanner and image coordinate spaces
        template <class InfoType>
          Transform (const InfoType& info) :
          out_of_bounds (true)
        {
          bounds[0] = info.dim(0) - 0.5;
          bounds[1] = info.dim(1) - 0.5;
          bounds[2] = info.dim(2) - 0.5;
          voxelsize[0] = info.vox(0);
          voxelsize[1] = info.vox(1);
          voxelsize[2] = info.vox(2);

          set_matrix (V2S, info.transform());
          for (size_t i = 0; i < 3; i++) {
            for (size_t j = 0; j < 3; j++) {
              V2S[i][j] *= info.vox (j);
            }
          }

          int signum;
          Math::Permutation p (4);
          Math::Matrix<float> D (4,4);
          Math::Matrix<float> M (4,4);
          for (size_t i = 0; i < 3; i++)
            for (size_t j = 0; j < 4; j++)
              D(i,j) = V2S[i][j];
          D (3,0) = D (3,1) = D (3,2) = 0.0;
          D (3,3) = 1.0;
          Math::LU::decomp (D, p, signum);
          Math::LU::inv (M, D, p);
          M (3,0) = M (3,1) = M (3,2) = 0.0;
          M (3,3) = 1.0;
          set_matrix (S2V, M);

          set_matrix (I2S, info.transform());

          D = info.transform();
          Math::LU::decomp (D, p, signum);
          Math::LU::inv (M, D, p);
          M (3,0) = M (3,1) = M (3,2) = 0.0;
          M (3,3) = 1.0;
          set_matrix (S2I, M);
        }

        //! Transform the position \p s from scanner-space to voxel-space \p v
        template <class P1, class P2> void scanner2voxel (const P1& s, P2& v) const {
          transform_position (v, S2V, s);
        }
        //! Transform the position \p v from voxel-space to scanner-space \p s
        template <class P1, class P2> void voxel2scanner (const P1& v, P2& s) const {
          transform_position (s, V2S, v);
        }
        //! Transform the position \p i from image-space to voxel-space \p v
        template <class P1, class P2> void image2voxel (const P1& i, P2& v) const {
          v[0] = i[0]/voxelsize[0]; v[1] = i[1]/voxelsize[1]; v[2] = i[2]/voxelsize[2];
        }
        //! Transform the position \p v from voxel-space to image-space \p i
        template <class P1, class P2> void voxel2image (const P1& v, P2& i) const {
          i[0] = v[0]*voxelsize[0]; i[1] = v[1]*voxelsize[1]; i[2] = v[2]*voxelsize[2];
        }
        //! Transform the position \p i from image-space to scanner-space \p s
        template <class P1, class P2> void image2scanner (const P1& i, P2& s) const {
          transform_position (s, I2S, i);
        }
        //! Transform the position \p v from scanner-space to image-space \p i
        template <class P1, class P2> void scanner2image (const P1& s, P2& i) const {
          transform_position (i, S2I, s);
        }
        //! Transform the orientation \p s from scanner-space to voxel-space \p v
        template <class P1, class P2> void scanner2voxel_dir (const P1& s, P2& v) const {
          transform_direction (v, S2V, s);
        }
        //! Transform the orientation \p v from voxel-space to scanner-space \p s
        template <class P1, class P2> void voxel2scanner_dir (const P1& v, P2& s) const {
          transform_direction (s, V2S, v);
        }

        //! Transform the position \p r from scanner-space to voxel-space
        template <class P1> Point<float> scanner2voxel (const P1& r) const {
          return transform_position (S2V, r);
        }
        //! Transform the position \p r from voxel-space to scanner-space
        template <class P1> Point<float> voxel2scanner (const P1& r) const {
          return transform_position (V2S, r);
        }
        //! Transform the position \p r from image-space to voxel-space
        template <class P1> Point<float> image2voxel (const P1& r) const {
          return Point<float> (r[0]/voxelsize[0], r[1]/voxelsize[1], r[2]/voxelsize[2]);
        }
        //! Transform the position \p r from voxel-space to image-space
        template <class P1> Point<float> voxel2image (const P1& r) const {
          return Point<float> (r[0]*voxelsize[0], r[1]*voxelsize[1], r[2]*voxelsize[2]);
        }
        //! Transform the position \p r from image-space to scanner-space
        template <class P1> Point<float> image2scanner (const P1& r) const {
          return transform_position (I2S, r);
        }
        //! Transform the position \p r from scanner-space to image-space
        template <class P1> Point<float> scanner2image (const P1& r) const {
          return transform_position (S2I, r);
        }
        //! Transform the orientation \p r from scanner-space to voxel-space
        template <class P1> Point<float> scanner2voxel_dir (const P1& r) const {
          return transform_vector (S2V, r);
        }
        //! Transform the orientation \p r from voxel-space to scanner-space
        template <class P1> Point<float> voxel2scanner_dir (const P1& r) const {
          return transform_vector (V2S, r);
        }

        const float* scanner2voxel_matrix () {
          return *S2V;
        }

        const float* voxel2scanner_matrix () {
          return *V2S;
        }

        const float* image2scanner_matrix () {
          return *I2S;
        }

        const float* scanner2image_matrix () {
          return *S2I;
        }


        void scanner2voxel_matrix (Math::Matrix<float>& M) {
          get_matrix (M, S2V);
        }

        void voxel2scanner_matrix (Math::Matrix<float>& M) {
          get_matrix (M, V2S);
        }

        void voxel2image_matrix (Math::Matrix<float>& M) {
          M.allocate (4,4);
          M.identity();
          M(0,0) = voxelsize[0];
          M(1,1) = voxelsize[1];
          M(2,2) = voxelsize[2];
        }

        void image2voxel_matrix (Math::Matrix<float>& M) {
          M.allocate (4,4);
          M.identity();
          M(0,0) = 1 / voxelsize[0];
          M(1,1) = 1 / voxelsize[1];
          M(2,2) = 1 / voxelsize[2];
        }

        void image2scanner_matrix (Math::Matrix<float>& M) {
          get_matrix (M, I2S);
        }

        void scanner2image_matrix (Math::Matrix<float>& M) {
          get_matrix (M, S2I);
        }

        template <class T, class P1, class P2>
        static inline void transform_position (P1& y, const Math::Matrix<T>& M, const P2& x)
        {
          y[0] = M (0,0) *x[0] + M (0,1) *x[1] + M (0,2) *x[2] + M (0,3);
          y[1] = M (1,0) *x[0] + M (1,1) *x[1] + M (1,2) *x[2] + M (1,3);
          y[2] = M (2,0) *x[0] + M (2,1) *x[1] + M (2,2) *x[2] + M (2,3);
        }

        template <typename T, class P1, class P2>
        static inline void transform_direction (P1& y, const Math::Matrix<T>& M, const P2& x)
        {
          y[0] = M (0,0) *x[0] + M (0,1) *x[1] + M (0,2) *x[2];
          y[1] = M (1,0) *x[0] + M (1,1) *x[1] + M (1,2) *x[2];
          y[2] = M (2,0) *x[0] + M (2,1) *x[1] + M (2,2) *x[2];
        }

        template <class Info, typename T>
        static inline Math::Matrix<T>& set_default (Math::Matrix<T>& M, const Info& ds) {
          M.allocate (4,4);
          M.identity();
          M (0,3) = -0.5 * (ds.dim (0)-1) * ds.vox (0);
          M (1,3) = -0.5 * (ds.dim (1)-1) * ds.vox (1);
          M (2,3) = -0.5 * (ds.dim (2)-1) * ds.vox (2);
          return M;
        }

        bool check_bounds (const Point<float>& pos) const {
          if (pos[0] <= -0.5 || pos[0] >= bounds[0] ||
              pos[1] <= -0.5 || pos[1] >= bounds[1] ||
              pos[2] <= -0.5 || pos[2] >= bounds[2]) {
            return true;
          }
          return false;
        }

      protected:
        float  S2V[3][4], V2S[3][4], I2S[3][4], S2I[3][4], voxelsize[3];
        float  bounds[3];
        bool   out_of_bounds;

        //! test whether current position is within bounds.
        /*! \return true if the current position is out of bounds, false otherwise */
        bool  operator! () const {
          return out_of_bounds;
        }

        void set_matrix (float M[3][4], const Math::Matrix<float>& MV) {
          for (size_t i = 0; i < 3; i++)
            for (size_t j = 0; j < 4; j++)
              M[i][j] = MV(i, j);
        }

        void set_matrix (Math::Matrix<float>& MV, float M[3][4]) {
          for (size_t i = 0; i < 3; i++)
            for (size_t j = 0; j < 4; j++)
              MV(i, j) = M[i][j];
        }

        void get_matrix (Math::Matrix<float> & MV, float M[3][4]) {
          MV.allocate (4,4);
          MV.identity();
          set_matrix (MV, M);
        }

        Point<float> set_to_nearest (const Point<float>& pos) {
          out_of_bounds = check_bounds(pos);
          if (out_of_bounds)
            return Point<float> ();
          else
            return Point<float> (pos[0]-Math::floor (pos[0]), pos[1]-Math::floor (pos[1]), pos[2]-Math::floor (pos[2]));
        }

        Point<float> transform_position (const float M[3][4], const Point<float>& p) const {
          return Point<float> (
            M[0][0]*p[0] + M[0][1]*p[1] + M[0][2]*p[2] + M[0][3],
            M[1][0]*p[0] + M[1][1]*p[1] + M[1][2]*p[2] + M[1][3],
            M[2][0]*p[0] + M[2][1]*p[1] + M[2][2]*p[2] + M[2][3]);
        }

        Point<float> transform_vector (const float M[3][4], const Point<float>& p) const {
          return Point<float> (
            M[0][0]*p[0] + M[0][1]*p[1] + M[0][2]*p[2],
            M[1][0]*p[0] + M[1][1]*p[1] + M[1][2]*p[2],
            M[2][0]*p[0] + M[2][1]*p[1] + M[2][2]*p[2]);
        }

        template <class P1, class P2>
        static inline void transform_position (P1& y, const float M[3][4], const P2& x) {
          y[0] = M[0][0]*x[0] + M[0][1]*x[1] + M[0][2]*x[2] + M[0][3];
          y[1] = M[1][0]*x[0] + M[1][1]*x[1] + M[1][2]*x[2] + M[1][3],
          y[2] = M[2][0]*x[0] + M[2][1]*x[1] + M[2][2]*x[2] + M[2][3];
        }

        template <class P1, class P2>
        static inline void transform_direction (P1& y, const float M[3][4], const P2& x) {
          y = M[0][0]*x[0] + M[0][1]*x[1] + M[0][2]*x[2];
          y = M[1][0]*x[0] + M[1][1]*x[1] + M[1][2]*x[2];
          y = M[2][0]*x[0] + M[2][1]*x[1] + M[2][2]*x[2];
        }

    };
  }
}

#endif

