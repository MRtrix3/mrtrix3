/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#include <cassert>
#include <cstdint>
#include <variant>

#include "datatype.h"
#include "exception.h"
#include "half.h"
#include "match_variant.h"
#include "types.h"

namespace MR::DWI::Tractography {

//! \brief Per-streamline sidecar value for one field, one streamline (§2.2).
/*! The data-per-streamline (dps) shape from decision D3: a 1×M row whose column
 * count \c M is a runtime-dynamic Eigen dimension (sidecar fields are discovered
 * when a dataset is opened, never a compile-time constant). The scalar/vector
 * distinction of D3 is preserved and must not be flattened:
 *   - \c M == 1 → behaves as a scalar (e.g. the reserved "weight" field), via
 *     scalar(); it is NOT degraded to a 1-element vector.
 *   - \c M >  1 → behaves as a length-M row vector, via vector().
 * The element type \c T is the field's native on-disk datatype (D7); see the
 * DPSValue std::variant below, which holds one ScalarOrVector alternative per
 * supported element type. */
template <typename T> class ScalarOrVector : public Eigen::Matrix<T, 1, Eigen::Dynamic> {
public:
  using element_type = T;
  using base_type = Eigen::Matrix<T, 1, Eigen::Dynamic>;
  using base_type::base_type;
  using base_type::operator=;

  ScalarOrVector() = default;
  ScalarOrVector(const base_type &row) : base_type(row) {}
  ScalarOrVector(base_type &&row) : base_type(std::move(row)) {}

  //! \brief the single scalar value of an M==1 field (D3).
  /*! \pre cols()==1; asserts otherwise. A scalar dps field (such as the
   * reserved "weight") surfaces through this accessor, never as a 1-element
   * vector. */
  T scalar() const {
    assert(this->cols() == 1);
    return (*this)(0, 0);
  }

  //! \brief the M-element row of a (possibly scalar) field.
  const base_type &vector() const { return *this; }
  base_type &vector() { return *this; }
};

//! \brief Per-vertex sidecar value for one field, one streamline (§2.2).
/*! The data-per-vertex (dpv) shape from decision D3: an n_vertices×M matrix.
 *   - \c M == 1 → one scalar per vertex (a length-n_vertices vector; the legacy
 *     ".tsf" / TrackScalar case), available as a single-column view via
 *     vector().
 *   - \c M >  1 → an n_vertices×M matrix (e.g. per-vertex colour).
 * As with ScalarOrVector, the column count \c M is runtime-dynamic and the
 * element type \c T is the field's native on-disk datatype (D7). */
template <typename T> class VectorOrMatrix : public Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> {
public:
  using element_type = T;
  using base_type = Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic>;
  using base_type::base_type;
  using base_type::operator=;

  VectorOrMatrix() = default;
  VectorOrMatrix(const base_type &matrix) : base_type(matrix) {}
  VectorOrMatrix(base_type &&matrix) : base_type(std::move(matrix)) {}

  //! \brief the single column of an M==1 field as a vector view (the .tsf case).
  /*! \pre cols()==1; asserts otherwise. */
  auto vector() const {
    assert(this->cols() == 1);
    return this->col(0);
  }
  auto vector() {
    assert(this->cols() == 1);
    return this->col(0);
  }
};

//! \brief The supported sidecar element types, in a fixed variant order (§2.2).
/*! Each entry expands to one std::variant alternative; the ordering is fixed and
 * shared verbatim between DPSValue and DPVValue. \c bit is represented in memory
 * as uint8_t (0/1); the on-disk "bit" datatype is recorded separately in the
 * field descriptor (field_registry.h) so it round-trips losslessly.
 * \c Eigen::half carries float16 fields without a float detour. The macro takes
 * the shape-wrapper template (ScalarOrVector for dps, VectorOrMatrix for dpv) so
 * the same element ordering produces both value variants. */
#define MRTRIX_TRACTOGRAPHY_SIDECAR_ALTERNATIVES(Shape)                                                                \
  Shape<uint8_t>, Shape<int8_t>, Shape<uint16_t>, Shape<int16_t>, Shape<uint32_t>, Shape<int32_t>, Shape<uint64_t>,    \
      Shape<int64_t>, Shape<Eigen::half>, Shape<float>, Shape<double>

//! \brief Variant over the per-streamline (dps) value type, one native element
//!   type per alternative (§2.1/§2.2).
using DPSValue = std::variant<MRTRIX_TRACTOGRAPHY_SIDECAR_ALTERNATIVES(ScalarOrVector)>;
//! \brief Variant over the per-vertex (dpv) value type (§2.1/§2.2).
using DPVValue = std::variant<MRTRIX_TRACTOGRAPHY_SIDECAR_ALTERNATIVES(VectorOrMatrix)>;

#undef MRTRIX_TRACTOGRAPHY_SIDECAR_ALTERNATIVES

//! \brief Build a DPSValue holding a ScalarOrVector<T>.
template <typename T> DPSValue make_dps(ScalarOrVector<T> &&v) { return DPSValue(std::move(v)); }
//! \brief Build a DPVValue holding a VectorOrMatrix<T>.
template <typename T> DPVValue make_dpv(VectorOrMatrix<T> &&v) { return DPVValue(std::move(v)); }

//! \brief Build a scalar (M==1) DPSValue of float, the shape of the reserved weight.
inline DPSValue make_dps_scalar(const float value) {
  ScalarOrVector<float> row(1);
  row(0, 0) = value;
  return DPSValue(std::move(row));
}

//! \brief The scalar value of a per-streamline (M==1) field as float (§2.2/D3).
/*! Reads the single element of a dps field whose column count M==1 — the shape of
 * the reserved streamline weight — converting from the field's native on-disk
 * element type to the float processing precision. \pre the value's column count
 * is 1 (asserts via ScalarOrVector::scalar()). */
inline float dps_scalar_to_float(const DPSValue &value) {
  return MR::match_v(value, [](const auto &row) -> float { return static_cast<float>(row.scalar()); });
}

//! \brief The per-vertex scalar value of a per-vertex (M==1) field at \a row, as float.
/*! Reads element (row, 0) of a dpv field whose column count M==1 — the legacy
 * ".tsf" / TrackScalar shape — converting from the field's native on-disk element
 * type to the float processing precision. \pre the value's column count is 1
 * (asserts); \a row is a valid vertex index. */
inline float dpv_scalar_to_float(const DPVValue &value, const Eigen::Index row) {
  return MR::match_v(value, [row](const auto &matrix) -> float {
    assert(matrix.cols() == 1);
    return static_cast<float>(matrix(row, 0));
  });
}

//! \brief The canonical (native-endian, non-complex) DataType for a sidecar
//!   element type \c T (§2.2/D7).
/*! The "bit" datatype maps onto a uint8_t storage element and is reported here
 * as DataType::UInt8; the field descriptor records "bit" separately when that
 * narrower on-disk packing is required. */
template <typename T> DataType sidecar_datatype();
// clang-format off
template <> inline DataType sidecar_datatype<uint8_t>()    { return DataType(DataType::UInt8); }
template <> inline DataType sidecar_datatype<int8_t>()     { return DataType(DataType::Int8); }
template <> inline DataType sidecar_datatype<uint16_t>()   { return DataType::native(DataType(DataType::UInt16)); }
template <> inline DataType sidecar_datatype<int16_t>()    { return DataType::native(DataType(DataType::Int16)); }
template <> inline DataType sidecar_datatype<uint32_t>()   { return DataType::native(DataType(DataType::UInt32)); }
template <> inline DataType sidecar_datatype<int32_t>()    { return DataType::native(DataType(DataType::Int32)); }
template <> inline DataType sidecar_datatype<uint64_t>()   { return DataType::native(DataType(DataType::UInt64)); }
template <> inline DataType sidecar_datatype<int64_t>()    { return DataType::native(DataType(DataType::Int64)); }
template <> inline DataType sidecar_datatype<Eigen::half>(){ return DataType::native(DataType(DataType::Float16)); }
template <> inline DataType sidecar_datatype<float>()      { return DataType::native(DataType(DataType::Float32)); }
template <> inline DataType sidecar_datatype<double>()     { return DataType::native(DataType(DataType::Float64)); }
// clang-format on

//! \brief Dispatch a callable \a fn against the C++ element type a DataType
//!   denotes, for the sidecar-supported element set (§2.2).
/*! Invokes \c fn.template operator()<T>() where \c T is the native element type
 * for \a dtype, allowing dtype-generic construction of a ScalarOrVector<T> /
 * VectorOrMatrix<T> without a switch at every call site. Throws if \a dtype is
 * not a supported sidecar element type. The "bit" datatype is handled as
 * uint8_t. */
template <class Functor> decltype(auto) dispatch_sidecar_datatype(const DataType dtype, Functor &&fn) {
  switch (dtype() & (DataType::Type | DataType::Signed)) {
  case DataType::Bit:
  case DataType::UInt8:
    return fn.template operator()<uint8_t>();
  case DataType::Int8:
    return fn.template operator()<int8_t>();
  case DataType::UInt16:
    return fn.template operator()<uint16_t>();
  case DataType::Int16:
    return fn.template operator()<int16_t>();
  case DataType::UInt32:
    return fn.template operator()<uint32_t>();
  case DataType::Int32:
    return fn.template operator()<int32_t>();
  case DataType::UInt64:
    return fn.template operator()<uint64_t>();
  case DataType::Int64:
    return fn.template operator()<int64_t>();
  case DataType::Float16:
    return fn.template operator()<Eigen::half>();
  case DataType::Float32:
    return fn.template operator()<float>();
  case DataType::Float64:
    return fn.template operator()<double>();
  default:
    throw Exception("unsupported sidecar element datatype \"" + dtype.specifier() + "\"");
  }
}

} // namespace MR::DWI::Tractography
