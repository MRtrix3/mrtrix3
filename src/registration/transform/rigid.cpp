/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include <deque>
#include <algorithm> // std::min_element
#include <iterator>
#include "registration/transform/rigid.h"
#include "math/gradient_descent.h"
#include "math/math.h"
#include "math/median.h"
#include "debug.h"


namespace MR
{
  using namespace MR::Math;

  namespace Registration
  {
    namespace Transform
    {
      void project_linear2rotation(const Eigen::Matrix<default_type, 3, 3>& linear, Eigen::Matrix<default_type, 3, 3>& rotation) {
        Eigen::JacobiSVD<Eigen::Matrix<default_type, 3, 3>> svd(linear, Eigen::ComputeFullU | Eigen::ComputeFullV);

        default_type x = (svd.matrixU() * svd.matrixV().adjoint()).determinant(); // so x has absolute value 1
        Eigen::Matrix<default_type, 3, 1> sv(svd.singularValues());
        sv.coeffRef(0) *= x;
        Eigen::Matrix<default_type, 3, 3> m(svd.matrixU());
        m.col(0) /= x;
        rotation = m * svd.matrixV().adjoint();
        assert((rotation * rotation.transpose().eval()).isApprox(Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic>::Identity(3,3)) && "project_linear2rotation orthonormal?");
      }

      bool RigidLinearNonSymmetricUpdate::operator() (Eigen::Matrix<default_type, Eigen::Dynamic, 1>& newx,
          const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x,
          const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& g,
          default_type step_size) {
          assert (newx.size() == 12);
          assert (x.size() == 12);
          assert (g.size() == 12);

          Eigen::Matrix<default_type, 12, 1> delta;
          Eigen::Matrix<default_type, 4, 4> X, Delta, G, A, Asqrt, B, Bsqrt, Bsqrtinv, Xnew, P, Diff, XnewP;
          Registration::Transform::param_vec2mat(g, G);
          Registration::Transform::param_vec2mat(x, X);

          // enforce updates in the range of small angles
          if (step_size * G.block(0,0,3,3).array().abs().maxCoeff() > 0.2) {
            step_size = 0.2 / G.block(0,0,3,3).array().abs().maxCoeff();
          }
          // use control points and coherence length as update criterion
          if (control_points.size()) {
            P = control_points;
            const default_type orig_step_size(step_size);
            const default_type step_down_factor(0.5);
            while (true) {
              delta = g * step_size;
              Registration::Transform::param_vec2mat(delta, Delta);
              if ((X+Delta).determinant() <= 0.0){ step_size *= step_down_factor; continue; }
              Diff.noalias() = ((X+Delta) * P - X * P).cwiseAbs();
              if ((Diff.template block<3,1>(0,0) - coherence_distance).maxCoeff() > 0.0) { step_size *= step_down_factor; continue;}
              if ((Diff.template block<3,1>(0,1) - coherence_distance).maxCoeff() > 0.0) { step_size *= step_down_factor; continue;}
              if ((Diff.template block<3,1>(0,2) - coherence_distance).maxCoeff() > 0.0) { step_size *= step_down_factor; continue;}
              if ((Diff.template block<3,1>(0,3) - coherence_distance).maxCoeff() > 0.0) { step_size *= step_down_factor; continue;}

              A = X - Delta;
              A(3,3) = 1.0;
              if (A.determinant() <= 0.0){ step_size *= step_down_factor; continue; }

              B = X.inverse() + Delta;
              B(3,3) = 1.0;
              if (B.determinant() <= 0.0){ step_size *= step_down_factor; continue; }

              Asqrt = A.sqrt().eval();
              assert(A.isApprox(Asqrt * Asqrt));
              Bsqrt = B.sqrt().eval();
              assert(B.isApprox(Bsqrt * Bsqrt));
              Bsqrtinv = Bsqrt.inverse().eval();

              Xnew = (Asqrt * Bsqrtinv) - ((Asqrt * Bsqrtinv - Bsqrtinv * Asqrt) * 0.5);
              Diff.noalias() = ((Xnew) * P - X * P).cwiseAbs();
              if ((Diff.template block<3,1>(0,0) - coherence_distance).maxCoeff() > 0.0) { step_size *= step_down_factor; continue;}
              if ((Diff.template block<3,1>(0,1) - coherence_distance).maxCoeff() > 0.0) { step_size *= step_down_factor; continue;}
              if ((Diff.template block<3,1>(0,2) - coherence_distance).maxCoeff() > 0.0) { step_size *= step_down_factor; continue;}
              if ((Diff.template block<3,1>(0,3) - coherence_distance).maxCoeff() > 0.0) { step_size *= step_down_factor; continue;}

              break;
            }
            if (orig_step_size != step_size) { DEBUG("step size changed from " + str(orig_step_size) + " to " + str(step_size)); }
          } else {
            // reduce step size if determinant of matrix becomes negative (happens rarely at first few iterations)
            size_t cnt = 0;
            const default_type factor(0.9);
            while (true) {
              delta = g * step_size;
              Registration::Transform::param_vec2mat(delta, Delta);
              if (Delta.block(0,0,3,3).array().abs().maxCoeff() > 0.1) {
                step_size = 0.09 / G.block(0,0,3,3).array().abs().maxCoeff();
                INFO(str(step_size) + " " + str(g * step_size));
                continue;
              }
              if (Delta.block(0,3,3,1).array().abs().maxCoeff() > 10.0){
                step_size = 9.0 / G.block(0,3,3,1).array().abs().maxCoeff();
                INFO(str(step_size) + " " + str(g * step_size));
                continue;
              }
              A = X - Delta;
              A(3,3) = 1.0;
              if (A.determinant() < 0) {
                step_size *= factor;
                ++cnt;
              } else {
                break;
              }
            }
            if (cnt > 0) INFO("rigid: gradient descent step size was too large. Multiplied by factor "
             + str(std::pow (factor, cnt), 4) + " (now: "+ str(step_size, 4) + ")");

            B = X.inverse() + Delta;
            B(3,3) = 1.0;
            assert(B.determinant() > 0.0);
            Asqrt = A.sqrt().eval();
            assert(A.isApprox(Asqrt * Asqrt));
            Bsqrt = B.sqrt().eval();
            assert(B.isApprox(Bsqrt * Bsqrt));
            Bsqrtinv = Bsqrt.inverse().eval();

            // approximation for symmetry reasons as
            // A and B don't commute
            Xnew = (Asqrt * Bsqrtinv) - ((Asqrt * Bsqrtinv - Bsqrtinv * Asqrt) * 0.5);
          }

          // project affine 3x3 matrix to rigid matrix
          // adjust translation to account for scale of affine by matching projected control point
          // centroids of affine and projected rigid transformation
          Eigen::Matrix<default_type, 3, 3> L(Xnew.template block<3,3>(0,0));
          Eigen::Matrix<default_type, 3, 3> R;
          project_linear2rotation(L, R);
          Xnew.template block<3,3>(0,0) = R;
          if (control_points.size()) {
            P = control_points;
            Eigen::Matrix<default_type, 3, 1> T_affine, T_new, centroid;
            T_affine = Xnew.block<3,1>(0,3);
            centroid = P.rowwise().mean().head<3>();
            T_new = L.sqrt() * centroid + T_affine - R.sqrt() * centroid;
            Xnew.template block<3,1>(0,3) = T_new;
          }

          Registration::Transform::param_mat2vec(Xnew, newx);

          if (newx.isApprox(x)) {
            DEBUG ("parameters unchanged");
            return false;
          }

          if (control_points.size()) {
            XnewP = (Xnew * P).eval();

            // stop criterion based on slope of smoothed control point trajectories
            if (use_convergence_check) {
              Registration::Transform::param_mat2vec (XnewP, new_control_points_vec);
              if (MR::File::Config::get_bool ("reg_gd_convergence_debug", false))
                convergence_check.debug(new_control_points_vec);
              if (!convergence_check.go_on (new_control_points_vec)) {
                DEBUG ("control point trajectories converged");
                return false;
              }
            }

            // stop criterion based on maximum shift of control points
            Diff.noalias() = (XnewP - X * P).cwiseAbs();
            Diff.row(0) *= recip_spacing(0);
            Diff.row(1) *= recip_spacing(1);
            Diff.row(2) *= recip_spacing(2);
            Diff.colwise() -= stop_len;
            if (Diff.template block<3,4>(0,0).maxCoeff() <= 0.0) {
              DEBUG ("max control point movement (" + str(Diff.template block<3,4>(0,0).maxCoeff()) +
              ") smaller than tolerance" );
              return false;
            }
          }

          return true;
        }

          void RigidLinearNonSymmetricUpdate::set_control_points (
            const Eigen::Matrix<default_type, Eigen::Dynamic, Eigen::Dynamic>& points,
            const Eigen::Vector3d& coherence_dist,
            const Eigen::Vector3d& stop_length,
            const Eigen::Vector3d& voxel_spacing ) {
            assert(points.rows() == 4);
            assert(points.cols() == 4);
            control_points = points;
            coherence_distance = coherence_dist;
            stop_len << stop_length, 0.0;
            recip_spacing << voxel_spacing.cwiseInverse(), 1.0;
          }


          bool RigidRobustEstimator::operator() (Eigen::Matrix<default_type, Eigen::Dynamic, 1>& newx,
            const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& x,
            const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& g,
            default_type step_size) {
          assert (newx.size() == x.size());
          assert (g.size() == x.size());
          newx = x - step_size * g;
          return !(newx.isApprox(x));
          }



      /** \addtogroup Transforms
      @{ */

      /*! A 3D rigid transformation class for registration.
       *
       * This class supports the ability to define the centre of rotation.
       * This should be set prior to commencing registration based on the centre of the target image.
       * The translation also should be initialised as moving image centre minus the target image centre.
       *
       */


          Eigen::Matrix<default_type, 4, 1> Rigid::get_jacobian_vector_wrt_params (const Eigen::Vector3& p) const {
            Eigen::Matrix<default_type, 4, 1> jac;
            jac.head(3) = p - centre;
            jac(3) = 1.0;
            return jac;
          }

          Eigen::MatrixXd Rigid::get_jacobian_wrt_params (const Eigen::Vector3& p) const {
            Eigen::MatrixXd jacobian (3, 12);
            jacobian.setZero();
            const auto v = get_jacobian_vector_wrt_params(p);
            jacobian.block<1, 4>(0, 0) = v;
            jacobian.block<1, 4>(1, 4) = v;
            jacobian.block<1, 4>(2, 8) = v;
            return jacobian;
          }

          void Rigid::set_parameter_vector (const Eigen::Matrix<Rigid::ParameterType, Eigen::Dynamic, 1>& param_vector) {
            this->trafo.matrix() = Eigen::Map<const Eigen::Matrix<ParameterType, 3, 4, Eigen::RowMajor> >(&param_vector(0));
            this->compute_halfspace_transformations();
          }

          void Rigid::get_parameter_vector (Eigen::Matrix<Rigid::ParameterType, Eigen::Dynamic, 1>& param_vector) const {
            param_vector.resize (12);
            param_mat2vec (this->trafo.matrix(), param_vector);
          }


          bool Rigid::robust_estimate (
            Eigen::Matrix<default_type, Eigen::Dynamic, 1>& gradient,
            vector<Eigen::Matrix<default_type, Eigen::Dynamic, 1>>& grad_estimates,
            const Eigen::Matrix<default_type, 4, 4>& control_points,
            const Eigen::Matrix<default_type, Eigen::Dynamic, 1>& parameter_vector,
            const default_type& weiszfeld_precision = 1.0e-6,
            const size_t& weiszfeld_iterations = 1000,
            default_type learning_rate = 1.0) const {
            throw Exception ("TODO robust estimate");
          }

      //! @}
    }
  }
}

