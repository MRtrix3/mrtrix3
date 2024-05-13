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

#pragma once

#include "datatype.h" // debug
#include "file/config.h"
#include "registration/transform/convergence_check.h"
#include "types.h"
#include <Eigen/Geometry> // Eigen::Translation
#include <Eigen/SVD>
#include <unsupported/Eigen/MatrixFunctions> // Eigen::MatrixBase::sqrt()

namespace MR::Registration::Transform {
template <class ValueType>
inline void param_mat2vec(const Eigen::Matrix<ValueType, 3, 4, Eigen::RowMajor> &transformation_matrix,
                          Eigen::Matrix<ValueType, Eigen::Dynamic, 1> &param_vector) {
  assert(param_vector.size() == 12);
  param_vector = Eigen::Map<Eigen::MatrixXd>(transformation_matrix.data(), 12, 1);
}
template <class ValueType>
inline void param_mat2vec(const Eigen::Matrix<ValueType, 4, 4, Eigen::RowMajor> &transformation_matrix,
                          Eigen::Matrix<ValueType, Eigen::Dynamic, 1> &param_vector) {
  assert(param_vector.size() == 12);
  param_vector = Eigen::Map<Eigen::MatrixXd>((transformation_matrix.template block<3, 4>(0, 0)).data(), 12, 1);
}
template <class ValueType>
inline void param_mat2vec(const Eigen::Matrix<ValueType, 3, 4, Eigen::ColMajor> &transformation_matrix,
                          Eigen::Matrix<ValueType, Eigen::Dynamic, 1> &param_vector) {
  assert(param_vector.size() == 12);
  // create temporary copy of the matrix in row major order and map it's memory as a vector
  param_vector = Eigen::Map<Eigen::Matrix<ValueType, 12, 1>>(
      (Eigen::Matrix<default_type, 3, 4, Eigen::RowMajor>(transformation_matrix)).data());
}
template <class ValueType>
inline void param_mat2vec(const Eigen::Matrix<ValueType, 4, 4, Eigen::ColMajor> &transformation_matrix,
                          Eigen::Matrix<ValueType, Eigen::Dynamic, 1> &param_vector) {
  assert(param_vector.size() == 12);
  // create temporary copy of the matrix in row major order and map it's memory as a vector
  param_vector = Eigen::Map<Eigen::Matrix<ValueType, 12, 1>>(
      (Eigen::Matrix<default_type, 3, 4, Eigen::RowMajor>((transformation_matrix.template block<3, 4>(0, 0)))).data());
}
template <class Derived, class ValueType>
inline void param_vec2mat(const Eigen::MatrixBase<Derived> &param_vector,
                          Eigen::Matrix<ValueType, 3, 4> &transformation_matrix) {
  assert(param_vector.size() == 12);
  transformation_matrix = Eigen::Map<const Eigen::Matrix<ValueType, 3, 4, Eigen::RowMajor>>(&param_vector(0));
}
template <class Derived, class ValueType>
inline void param_vec2mat(const Eigen::MatrixBase<Derived> &param_vector,
                          Eigen::Matrix<ValueType, 4, 4> &transformation_matrix) {
  assert(param_vector.size() == 12);
  transformation_matrix.template block<3, 4>(0, 0) =
      Eigen::Map<const Eigen::Matrix<ValueType, 3, 4, Eigen::RowMajor>>(&param_vector(0));
  transformation_matrix.row(3) << 0.0, 0.0, 0.0, 1.0;
}

/** \addtogroup Transforms
@{ */

/*! A base linear transformation class
 *
 * This class supports the ability to define the centre of rotation.
 * This should be set prior to commencing registration
 * The translation also should be initialised as image1 image centre minus the image2 image centre.
 *
 */
class Base {
public:
  using ParameterType = default_type;
  Base(size_t number_of_parameters);

  template <class OutPointType, class InPointType>
  inline void transform(OutPointType &out, const InPointType &in) const {
    out.noalias() = trafo * in;
  }

  template <class OutPointType, class InPointType>
  inline void transform_half(OutPointType &out, const InPointType &in) const {
    out.noalias() = trafo_half * in;
  }

  template <class OutPointType, class InPointType>
  inline void transform_half_inverse(OutPointType &out, const InPointType &in) const {
    out.noalias() = trafo_half_inverse * in;
  }

  size_t size() const { return number_of_parameters; }

  static Eigen::Matrix<default_type, 4, 1> get_jacobian_vector_wrt_params(const Eigen::Vector3d &p);

  Eigen::Transform<ParameterType, 3, Eigen::AffineCompact> get_transform() const;

  Eigen::Transform<ParameterType, 3, Eigen::AffineCompact> get_transform_half() const { return trafo_half; }

  Eigen::Transform<ParameterType, 3, Eigen::AffineCompact> get_transform_half_inverse() const {
    return trafo_half_inverse;
  }

  template <class TrafoType> void set_transform(const TrafoType &transform) {
    trafo.matrix().template block<3, 4>(0, 0) = transform.matrix().template block<3, 4>(0, 0);
    compute_halfspace_transformations();
  }

  // set_matrix_const_translation updates the 3x3 matrix without updating the translation
  void set_matrix_const_translation(const Eigen::Matrix<ParameterType, 3, 3> &mat);

  // set_matrix updates the 3x3 matrix and also updates the translation
  // void set_matrix (const Eigen::Matrix<ParameterType, 3, 3>& mat) {
  //   transform_type Tc2, To, R0;
  //   Tc2.setIdentity();
  //   To.setIdentity();
  //   R0.setIdentity();
  //   To.translation() = offset;
  //   Tc2.translation() = centre - 0.5 * offset;
  //   R0.linear() = mat;
  //   trafo = Tc2 * To * R0 * Tc2.inverse();
  //   compute_halfspace_transformations();
  // }

  const Eigen::Matrix<ParameterType, 3, 3> get_matrix() const { return trafo.linear(); }

  void set_translation(const Eigen::Matrix<ParameterType, 1, 3> &trans);

  const Eigen::Vector3d get_translation() const { return trafo.translation(); }

  void set_centre_without_transform_update(const Eigen::Vector3d &centre_in);

  void set_centre(const Eigen::Vector3d &centre_in);

  const Eigen::Vector3d get_centre() const { return centre; }

  void set_optimiser_weights(Eigen::VectorXd &weights);

  Eigen::VectorXd get_optimiser_weights() const { return optimiser_weights; }

  bool is_symmetric() const { return !nonsymmetric; }

  void use_nonsymmetric(const bool asym) { nonsymmetric = asym; }

  void set_offset(const Eigen::Vector3d &offset_in);

  std::string info();

  std::string debug();

  template <class ParamType, class VectorType>
  bool robust_estimate(VectorType &gradient,
                       std::vector<VectorType> &grad_estimates,
                       const ParamType &params,
                       const VectorType &parameter_vector,
                       const default_type &weiszfeld_precision,
                       const size_t &weiszfeld_iterations,
                       default_type &learning_rate) const {
    DEBUG("robust estimator is not implemented for this metric");
    for (auto &grad_estimate : grad_estimates) {
      gradient += grad_estimate;
    }
    return true;
  }

protected:
  void compute_offset();

  void compute_halfspace_transformations();

  size_t number_of_parameters;
  Eigen::Transform<ParameterType, 3, Eigen::AffineCompact> trafo;
  Eigen::Transform<ParameterType, 3, Eigen::AffineCompact> trafo_half;
  Eigen::Transform<ParameterType, 3, Eigen::AffineCompact> trafo_half_inverse;
  Eigen::Vector3d centre;
  Eigen::VectorXd optimiser_weights;
  bool nonsymmetric;
};
//! @}
} // namespace MR::Registration::Transform
