/* Copyright (c) 2008-2024 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */
#include "registration/transform/base.h"

namespace MR::Registration::Transform {

Base::Base(size_t number_of_parameters)
    : number_of_parameters(number_of_parameters), optimiser_weights(number_of_parameters), nonsymmetric(false) {
  trafo.matrix().setIdentity();
  trafo_half.matrix().setIdentity();
  trafo_half_inverse.matrix().setIdentity();
  centre.setZero();
}

Eigen::Matrix<default_type, 4, 1> Base::get_jacobian_vector_wrt_params(const Eigen::Vector3d &) {
  throw Exception("FIXME: get_jacobian_vector_wrt_params not implemented for this metric");
  Eigen::Matrix<default_type, 4, 1> jac{};
  return jac;
}

Eigen::Transform<Base::ParameterType, 3, Eigen::AffineCompact> Base::get_transform() const {
  Eigen::Transform<ParameterType, 3, Eigen::AffineCompact> transform{};
  transform.matrix() = trafo.matrix();
  return transform;
}

void Base::set_matrix_const_translation(const Eigen::Matrix<ParameterType, 3, 3> &mat) {
  trafo.linear() = mat;
  compute_halfspace_transformations();
}

void Base::set_translation(const Eigen::Matrix<ParameterType, 1, 3> &trans) {
  trafo.translation() = trans;
  compute_offset();
  compute_halfspace_transformations();
}

void Base::set_centre_without_transform_update(const Eigen::Vector3d &centre_in) {
  centre = centre_in;
  DEBUG("centre: " + str(centre.transpose()));
}

void Base::set_centre(const Eigen::Vector3d &centre_in) {
  centre = centre_in;
  DEBUG("centre: " + str(centre.transpose()));
  compute_offset();
  compute_halfspace_transformations();
}

void Base::set_optimiser_weights(Eigen::VectorXd &weights) {
  assert(size() == (size_t)optimiser_weights.size());
  optimiser_weights = weights;
}

void Base::set_offset(const Eigen::Vector3d &offset_in) {
  trafo.translation() = offset_in;
  compute_halfspace_transformations();
}

std::string Base::info() {
  const Eigen::IOFormat fmt(Eigen::FullPrecision, 0, ", ", "\n", "", "", "", "");
  INFO("transformation:\n" + str(trafo.matrix().format(fmt)));
  DEBUG("transformation_half:\n" + str(trafo_half.matrix().format(fmt)));
  DEBUG("transformation_half_inverse:\n" + str(trafo_half_inverse.matrix().format(fmt)));
  return "centre: " + str(centre.transpose(), 12);
}

std::string Base::debug() {
  const Eigen::IOFormat fmt(Eigen::FullPrecision, 0, ", ", "\n", "", "", "", "");
  CONSOLE("trafo:\n" + str(trafo.matrix().format(fmt)));
  CONSOLE("trafo_inverse:\n" + str(trafo.inverse().matrix().format(fmt)));
  CONSOLE("trafo_half:\n" + str(trafo_half.matrix().format(fmt)));
  CONSOLE("trafo_half_inverse:\n" + str(trafo_half_inverse.matrix().format(fmt)));
  CONSOLE("centre: " + str(centre.transpose(), 12));
  return "";
}

void Base::compute_offset() { trafo.translation() = (trafo.translation() + centre - trafo.linear() * centre).eval(); }

void Base::compute_halfspace_transformations() {
  Eigen::Matrix<ParameterType, 4, 4> tmp{};
  tmp.setIdentity();
  tmp.template block<3, 4>(0, 0) = trafo.matrix();
  assert((tmp.template block<3, 3>(0, 0).isApprox(trafo.linear())));
  assert(tmp.determinant() > 0);
  if (nonsymmetric) {
    trafo_half.matrix() = tmp.template block<3, 4>(0, 0);
    trafo_half_inverse.setIdentity();
    assert(trafo.matrix().isApprox(trafo.matrix()));
  } else {
    tmp = tmp.sqrt().eval();
    trafo_half.matrix() = tmp.template block<3, 4>(0, 0);
    trafo_half_inverse.matrix() = trafo_half.inverse().matrix();
    assert(trafo.matrix().isApprox((trafo_half * trafo_half).matrix()));
    assert(trafo.inverse().matrix().isApprox((trafo_half_inverse * trafo_half_inverse).matrix()));
  }
  // debug();
}

} // namespace MR::Registration::Transform
