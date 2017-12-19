/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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


#include <Eigen/Dense>
#include <unsupported/Eigen/MatrixFunctions>
#include <unsupported/Eigen/NonLinearOptimization>

#include "command.h"
#include "image.h"
#include "transform.h"
#include "interp/linear.h"


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "Daan Christiaens (daan.christiaens@kcl.ac.uk)";

  SYNOPSIS = "Asymmetric rigid registration.";

  ARGUMENTS
    + Argument ("target",
                "the target image.").type_image_in ()
    + Argument ("moving",
                "the moving image.").type_image_in ()
    + Argument ("T",
                "the transformation matrix.").type_file_out ();

  OPTIONS
    + Option ("mask", "image mask.")
      + Argument ("image").type_image_in ();

}


using value_type = float;


/* Exponential Lie mapping on SE(3). */
Eigen::Matrix<value_type, 4, 4> se3exp(const Eigen::VectorXf& v)
{
  Eigen::Matrix<value_type, 4, 4> A, T; A.setZero();
  A(0,3) = v[0]; A(1,3) = v[1]; A(2,3) = v[2];
  A(2,1) = v[3]; A(1,2) = -v[3];
  A(0,2) = v[4]; A(2,0) = -v[4];
  A(1,0) = v[5]; A(0,1) = -v[5];
  T = A.exp();
  return T;
}


class RegistrationFunctor
{
public:

  RegistrationFunctor(const Image<value_type>& target, const Image<value_type>& moving)
    : m (0), n (6),
      target (target),
      T0 (target),
      moving (moving, 0.0f),
      Dmoving (moving, 0.0f)
  {
    DEBUG("Constructing LM registration functor.");
    m = target.size(0) * target.size(1) * target.size(2);
  }

  int operator()(const Eigen::VectorXf& x, Eigen::VectorXf& fvec)
  {
    // get transformation matrix
    Eigen::Transform<value_type, 3, Eigen::Affine> T1 (se3exp(x));
    // interpolate and compute error
    Eigen::Vector3f vox, scan, trans;
    size_t i = 0;
    for (auto l = Loop({0, 1, 2})(target); l; l++, i++) {
      assign_pos_of(target).to(vox);
      scan = T0.voxel2scanner.template cast<value_type>() * vox;
      trans = T1 * scan;
      moving.scanner(trans);
      fvec[i] = target.value() - moving.value();
    }
    VAR(fvec.squaredNorm());
    return 0;
  }

  int df(const Eigen::VectorXf& x, Eigen::MatrixXf& fjac)
  {
    // get transformation matrix
    Eigen::Transform<value_type, 3, Eigen::Affine> T1 (se3exp(x));
    // Allocate 3 x 6 Jacobian
    Eigen::Matrix<value_type, 3, 6> J;
    J.setIdentity();
    J *= -1;
    // compute image gradient and Jacobian
    Eigen::Vector3f vox, scan, trans;
    Eigen::RowVector3f grad;
    size_t i = 0;
    for (auto l = Loop({0, 1, 2})(target); l; l++, i++) {
      assign_pos_of(target).to(vox);
      scan = T0.voxel2scanner.template cast<value_type>() * vox;
      trans = T1 * scan;
      Dmoving.scanner(trans);
      grad = Dmoving.gradient_wrt_scanner().template cast<value_type>();
      J(2,4) = trans[0]; J(1,5) = -trans[0];
      J(0,5) = trans[1]; J(2,3) = -trans[1];
      J(1,3) = trans[2]; J(0,4) = -trans[2];
      fjac.row(i) = grad * J;
    }
    return 0;
  }

  size_t values() const { return m; }
  size_t inputs() const { return n; }

private:
  size_t m, n;
  Image<value_type> target;
  Transform T0;
  Interp::LinearInterp<Image<value_type>, Interp::LinearInterpProcessingType::Value> moving;
  Interp::LinearInterp<Image<value_type>, Interp::LinearInterpProcessingType::Derivative> Dmoving;

};


void run ()
{
  auto target = Image<value_type>::open(argument[0]);
  auto moving = Image<value_type>::open(argument[1]);

  Eigen::VectorXf x (6);
  x.setZero();

  RegistrationFunctor F (target, moving);
  Eigen::LevenbergMarquardt<RegistrationFunctor, value_type> LM (F);
  INFO("Minimizing SSD cost function.");
  LM.minimize(x);

  VAR(x.transpose());
  save_matrix(se3exp(x), argument[2]);

}



