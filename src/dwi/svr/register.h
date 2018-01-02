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
#include <unsupported/Eigen/LevenbergMarquardt>

#include "types.h"
#include "image.h"
#include "transform.h"
#include "interp/linear.h"


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


      /* Register prediction to slices. */
      class SliceRegistrationFunctor : public Eigen::DenseFunctor<float>
      {  MEMALIGN(SliceRegistrationFunctor);
      public:
      
        SliceRegistrationFunctor(const Image<Scalar>& target, const Image<Scalar>& moving, 
                                 const Image<bool>& mask, const size_t mb, const size_t v, const size_t e)
          : m (0), nexc ((mb) ? target.size(2)/mb : 1), vol (v), exc (e), T0 (target),
            mask (mask), target (target),
            moving (moving, 0.0f), Dmoving (moving, 0.0f)
        {
          DEBUG("Constructing LM registration functor.");
          m = calcMaskSize();
        }
        
        int operator() (const InputType& x, ValueType& fvec)
        {
          // get transformation matrix
          Eigen::Transform<Scalar, 3, Eigen::Affine> T1 (se3exp(x));
          // interpolate and compute error
          Eigen::Vector3f trans;
          size_t i = 0;
          target.index(3) = vol;
          for (target.index(2) = exc; target.index(2) < target.size(2); target.index(2) += nexc) {
            for (auto l = Loop(0,2)(target); l; l++) {
              if (!isInMask()) continue;
              trans = T1 * getScanPos();
              moving.scanner(trans);
              fvec[i] = target.value() - moving.value();
              i++;
            }
          }
          return 0;
        }
        
        int df(const InputType& x, JacobianType& fjac)
        {
          // get transformation matrix
          Eigen::Transform<Scalar, 3, Eigen::Affine> T1 (se3exp(x));
          // Allocate 3 x 6 Jacobian
          Eigen::Matrix<Scalar, 3, 6> J;
          J.setIdentity();
          J *= -1;
          // compute image gradient and Jacobian
          Eigen::Vector3f trans;
          Eigen::RowVector3f grad;
          size_t i = 0;
          target.index(3) = vol;
          for (target.index(2) = exc; target.index(2) < target.size(2); target.index(2) += nexc) {
            for (auto l = Loop(0,2)(target); l; l++) {
              if (!isInMask()) continue;
              trans = T1 * getScanPos();
              Dmoving.scanner(trans);
              grad = Dmoving.gradient_wrt_scanner().template cast<Scalar>();
              J(2,4) = trans[0]; J(1,5) = -trans[0];
              J(0,5) = trans[1]; J(2,3) = -trans[1];
              J(1,3) = trans[2]; J(0,4) = -trans[2];
              fjac.row(i) = 2.0f * grad * J;
              i++;
            }
          }
          return 0;
        }
        
        size_t values() const { return m; }
        size_t inputs() const { return 6; }
        
      private:
        size_t m, nexc, vol, exc;
        Transform T0;
        Image<bool> mask;
        Image<Scalar> target;
        Interp::LinearInterp<Image<Scalar>, Interp::LinearInterpProcessingType::Value> moving;
        Interp::LinearInterp<Image<Scalar>, Interp::LinearInterpProcessingType::Derivative> Dmoving;
        
        inline size_t calcMaskSize()
        {
          if (!mask) return target.size(0)*target.size(1)*target.size(2)/nexc;
          size_t count = 0;
          for (mask.index(2) = exc; mask.index(2) < mask.size(2); mask.index(2) += nexc) {
            for (auto l = Loop(0,2)(mask); l; l++) {
              if (mask.value()) count++;
            }
          }
          return count;
        }
        
        inline bool isInMask() {
          if (mask.valid()) {
            assign_pos_of(target).to(mask);
            return mask.value();
          } else {
            return true;
          }
        }
        
        inline Eigen::Vector3f getScanPos() {
          Eigen::Vector3f vox, scan;
          assign_pos_of(target).to(vox);
          scan = T0.voxel2scanner.template cast<Scalar>() * vox;
          return scan;
        }
      
      };
      


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
      {  MEMALIGN(SliceAlignSource);
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
          slice.bidx = bidx[slice.vol];
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
          return idx <= nv*ne;
        }

      private:
        const size_t nv, ne, ne_init;
        size_t idx;
        const Eigen::Matrix<float, Eigen::Dynamic, 3> dirs;
        vector<size_t> bidx;
        const Eigen::Matrix<float, Eigen::Dynamic, 6> init; // Euler angle representation
      };


      class SliceAlignPipe
      {  MEMALIGN(SliceAlignPipe);
      public:
        SliceAlignPipe(const Image<float>& data, const Image<float>& mssh, const Image<bool>& mask,
                       const size_t mb, const size_t maxiter)
          : data (data), mssh (mssh), mask(mask), mb (mb), 
            maxiter (maxiter), lmax (Math::SH::LforN(mssh.size(4)))
        { }
      
        bool operator() (const SliceIdx& slice, SliceIdx& out)
        {
          out = slice;
          // calculate dwi contrast
          Eigen::VectorXf delta;
          Math::SH::delta(delta, slice.bvec, lmax);
          Header header (mssh);
          header.ndim() = 3;
          auto pred = Image<float>::scratch(header);
          mssh.index(3) = slice.bidx;
          for (auto l = Loop(0,3) (mssh, pred); l; l++)
          {
            pred.value() = 0;
            size_t j = 0;
            for (auto k = Loop(4) (mssh); k; k++, j++)
              pred.value() += delta[j] * mssh.value();
          }
          // register prediction to data
          SliceRegistrationFunctor func (data, pred, mask, mb, slice.vol, slice.exc);
          Eigen::LevenbergMarquardt<SliceRegistrationFunctor> lm (func);
          if (maxiter > 0)
            lm.setMaxfev(maxiter);
          Eigen::VectorXf x (slice.motion);
          lm.minimize(x);
          if (lm.info() == Eigen::ComputationInfo::Success || lm.info() == Eigen::ComputationInfo::NoConvergence)
            out.motion = x.head<6>();
          return true;
        }

      private:
        Image<float> data;
        Image<float> mssh;
        Image<bool> mask;
        const size_t mb, maxiter;
        const int lmax;
      
      };


      class SliceAlignSink
      {  MEMALIGN(SliceAlignSink);
      public:
        SliceAlignSink(const size_t nv, const size_t nz, const size_t mb)
          : ne ((mb) ? nz/mb : 1),
            motion (nv*ne, 6),
            progress ("Registering slices to template volume.", nv*ne)
        {
          motion.setZero();
        }

        bool operator() (const SliceIdx& slice)
        {
          size_t idx = slice.vol * ne + slice.exc;
          Eigen::Transform<float, 3, Eigen::Affine> T (se3exp(slice.motion));
          motion.row(idx).head<3>() = T.translation();
          motion.row(idx).tail<3>() = T.rotation().eulerAngles(2, 1, 0);
          progress++;
          return true;
        }

        const Eigen::Matrix<float, Eigen::Dynamic, 6>& get_motion() const { return motion; }

      private:
        const size_t ne;
        Eigen::Matrix<float, Eigen::Dynamic, 6> motion; // Euler angle representation
        ProgressBar progress;

      };

    }
  }
}

#endif


