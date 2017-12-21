/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __dwi_svr_register_h__
#define __dwi_svr_register_h__

#include <Eigen/Dense>
#include <unsupported/Eigen/MatrixFunctions>

#include "types.h"


namespace MR
{
  namespace DWI
  {
    namespace SVR
    {

      /* Exponential Lie mapping on SE(3). */
      Eigen::Matrix4f se3exp(const Eigen::VectorXf& v)
      {
        Eigen::Matrix4f A, T; A.setZero();
        A(0,3) = v[0]; A(1,3) = v[1]; A(2,3) = v[2];
        A(2,1) = v[3]; A(1,2) = -v[3];
        A(0,2) = v[4]; A(2,0) = -v[4];
        A(1,0) = v[5]; A(0,1) = -v[5];
        T = A.exp();
        return T;
      }


      /* Logarithmic Lie mapping on SE(3). */
      Eigen::Matrix<float, 6, 1> se3log(const Eigen::Matrix4f& T)
      {
        Eigen::Matrix4f A = T.log();
        Eigen::Matrix<float, 6, 1> v;
        v[0] = A(0,3); v[1] = A(1,3); v[2] = A(2,3);
        v[3] = (A(2,1) - A(1,2)) / 2;
        v[4] = (A(0,2) - A(2,0)) / 2;
        v[5] = (A(1,0) - A(0,1)) / 2;
        return v;
      }


      /* Slice item for multi-threaded processing. */
      struct SliceIdx {
        // volume and excitation index.
        size_t vol;
        size_t exc;
        // b-value index in MSSH image.
        size_t bidx;
        // rigid motion parameters in Lie vector representation.
        Eigen::Matrix<float, 6, 1> motion;
        // reoriented gradient direction.
        Eigen::Vector3f bvec;
      };


      class SliceAlignSource
      {
      public:
        SliceAlignSource(const size_t nv, const size_t nz, const size_t mb,
                         const Eigen::MatrixXd& grad, const vector<double> bvals,
                         const Eigen::MatrixXf& init)
          : nv(nv), ne((mb) ? nz/mb : 1),
            ne_init (init.rows() / nv), idx (0),
            dirs (grad.leftCols<3>().cast<float>()),
            bidx (), init (init.leftCols<6>())
        {
          if (ne_init > ne)
            throw Exception("initialisation invalid for given multiband factor.");
          for (int i = 0; i < grad.rows(); i++) {
            size_t j = 0;
            for (auto b : bvals) {
              if (grad(i,3) > (b - DWI_SHELLS_EPSILON) && grad(i,3) < (b + DWI_SHELLS_EPSILON)) {
                bidx.push_back(j); break;
              } else j++;
            }
            if (j == bvals.size())
              throw Exception("invalid bvalues in gradient table.");
          }
        }

        bool operator() (SliceIdx& slice)
        {
          slice.vol = idx / ne;
          slice.exc = idx % ne;
          slice.bidx = bidx[idx];
          // create transformation matrix
          size_t idx_init = slice.vol * ne_init + slice.exc % ne_init;
          Eigen::Transform<float, 3, Eigen::Affine> m;
          m = Eigen::AngleAxisf(init(idx_init,3), Eigen::Vector3f::UnitZ())
            * Eigen::AngleAxisf(init(idx_init,4), Eigen::Vector3f::UnitY())
            * Eigen::AngleAxisf(init(idx_init,5), Eigen::Vector3f::UnitX());
          m.translation() = init.row(idx_init).head<3>();
          slice.motion = se3log(m.matrix());
          // reorient vector
          slice.bvec = m.rotation() * dirs.row(slice.vol).normalized().transpose();
          idx++;
          return idx < nv*ne;
        }

      private:
        const size_t nv, ne, ne_init;
        size_t idx;
        const Eigen::Matrix<float, Eigen::Dynamic, 3> dirs;
        vector<size_t> bidx;
        const Eigen::Matrix<float, Eigen::Dynamic, 6> init; // Euler angle representation
      };


      class SliceAlignSink
      {
      public:
        SliceAlignSink(const size_t nv, const size_t nz, const size_t mb)
          : ne ((mb) ? nz/mb : 1),
            motion (nv*ne, 6)
        {
          motion.setZero();
        }

        bool operator() (const SliceIdx& slice)
        {
          size_t idx = slice.vol * ne + slice.exc;
          Eigen::Transform<float, 3, Eigen::Affine> T (se3exp(slice.motion));
          motion.row(idx).head<3>() = T.translation();
          motion.row(idx).tail<3>() = T.rotation().eulerAngles(2, 1, 0);
          return true;
        }

        const Eigen::Matrix<float, Eigen::Dynamic, 6>& get_motion() const { return motion; }

      private:
        const size_t ne;
        Eigen::Matrix<float, Eigen::Dynamic, 6> motion; // Euler angle representation
      };

    }
  }
}

#endif


