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

#include "datatype.h"
#include "dwi/tractography/field_registry.h"
#include "dwi/tractography/shared.h"
#include "dwi/tractography/sidecar_value.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/tractogram_item.h"
#include "exception.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <variant>

using namespace MR;
using namespace MR::DWI::Tractography;

namespace {

//! dtype-generic functor: build a 1-element ScalarOrVector<T> wrapped as DPSValue.
struct MakeScalar {
  template <typename T> DPSValue operator()() const {
    ScalarOrVector<T> v(1);
    v(0, 0) = static_cast<T>(1);
    return make_dps(std::move(v));
  }
};

// D3: an M==1 dps field surfaces as a SCALAR via scalar(), never as a 1-element
//   vector that a consumer must unwrap.
TEST(Sidecar, DpsScalarAccessor) {
  ScalarOrVector<float> v(1);
  v(0, 0) = 2.5F;
  EXPECT_EQ(v.cols(), 1);
  EXPECT_FLOAT_EQ(v.scalar(), 2.5F);
}

// D3: an M>1 dps field surfaces as a length-M row vector (no scalar collapse).
TEST(Sidecar, DpsVectorAccessor) {
  ScalarOrVector<uint8_t> v(3);
  v << 10, 20, 30;
  EXPECT_EQ(v.cols(), 3);
  EXPECT_EQ(v.vector().cols(), 3);
  EXPECT_EQ(v.vector()(0), 10);
  EXPECT_EQ(v.vector()(2), 30);
}

// D3: a dpv M==1 field is one scalar per vertex (a vector view of column 0).
TEST(Sidecar, DpvVectorView) {
  VectorOrMatrix<float> m(4, 1);
  m.col(0) << 1.0F, 2.0F, 3.0F, 4.0F;
  EXPECT_EQ(m.rows(), 4);
  EXPECT_EQ(m.cols(), 1);
  EXPECT_FLOAT_EQ(m.vector()(2), 3.0F);
}

// D3: a dpv M>1 field is an n_vertices x M matrix.
TEST(Sidecar, DpvMatrix) {
  VectorOrMatrix<uint8_t> m(2, 3); // 2 vertices, RGB
  m << 1, 2, 3, 4, 5, 6;
  EXPECT_EQ(m.rows(), 2);
  EXPECT_EQ(m.cols(), 3);
  EXPECT_EQ(m(1, 2), 6);
}

// D7: the variant preserves each field's NATIVE element type; an integer field
//   stored through DPSValue comes back out as the same integer type.
TEST(Sidecar, VariantNativeDtypePreserved) {
  ScalarOrVector<int32_t> iv(2);
  iv << -7, 13;
  DPSValue value = make_dps(std::move(iv));
  ASSERT_TRUE(std::holds_alternative<ScalarOrVector<int32_t>>(value));
  EXPECT_FALSE(std::holds_alternative<ScalarOrVector<float>>(value));
  const auto &back = std::get<ScalarOrVector<int32_t>>(value);
  EXPECT_EQ(back(0), -7);
  EXPECT_EQ(back(1), 13);
}

// sidecar_datatype<T>() reports the canonical native DataType for each element.
TEST(Sidecar, SidecarDatatypeMapping) {
  EXPECT_EQ(sidecar_datatype<uint8_t>()(), DataType::UInt8);
  EXPECT_EQ(sidecar_datatype<int8_t>()(), DataType::Int8);
  EXPECT_TRUE(sidecar_datatype<float>().is_floating_point());
  EXPECT_TRUE(sidecar_datatype<int16_t>().is_signed());
}

// dispatch_sidecar_datatype() maps a runtime DataType back to its C++ element
//   type, constructing the matching variant alternative.
TEST(Sidecar, DispatchByDatatype) {
  auto maker = MakeScalar{};

  DPSValue as_u16 = dispatch_sidecar_datatype(DataType(DataType::UInt16), maker);
  EXPECT_TRUE(std::holds_alternative<ScalarOrVector<uint16_t>>(as_u16));

  // A "bit" field is carried as uint8_t in memory.
  DPSValue as_bit = dispatch_sidecar_datatype(DataType(DataType::Bit), maker);
  EXPECT_TRUE(std::holds_alternative<ScalarOrVector<uint8_t>>(as_bit));

  DPSValue as_f64 = dispatch_sidecar_datatype(DataType::native(DataType(DataType::Float64)), maker);
  EXPECT_TRUE(std::holds_alternative<ScalarOrVector<double>>(as_f64));
}

// Streamline::weight is the single source of truth for the reserved "weight"
//   field: it is a member of the embedded Streamline, NOT the dps vector, so
//   the no-additional-sidecar case keeps an empty dps vector.
TEST(Sidecar, WeightIsSingleSourceOfTruth) {
  TractogramItem<float> item;
  item.streamline.weight = 0.75F;
  EXPECT_TRUE(item.dps.empty());
  EXPECT_TRUE(item.dpv.empty());
  EXPECT_TRUE(item.groups.empty());
  EXPECT_FLOAT_EQ(item.streamline.weight, 0.75F);

  // Move semantics keep index/weight/sidecar consistent.
  item.set_index(42);
  ScalarOrVector<float> wv(1);
  wv(0, 0) = 3.0F;
  item.dps.push_back(make_dps(std::move(wv)));
  TractogramItem<float> moved(std::move(item));
  EXPECT_EQ(moved.get_index(), 42u);
  EXPECT_FLOAT_EQ(moved.streamline.weight, 0.75F);
  ASSERT_EQ(moved.dps.size(), 1u);
  EXPECT_FLOAT_EQ(std::get<ScalarOrVector<float>>(moved.dps[0]).scalar(), 3.0F);
}

//! register a field of the given role; returns its assigned ordinal.
size_t add_field(FieldRegistry &reg, const std::string &name, FieldRole role, DataType dtype, size_t M) {
  FieldDescriptor d;
  d.name = name;
  d.role = role;
  d.dtype = dtype;
  d.columns = M;
  d.source = FieldSource::Internal;
  d.ordinal = 0;
  return reg.add(d);
}

// The registry counts ordinals independently per role (the dps and dpv payload
//   vectors of TractogramItem are separate ordinal spaces, §2.1).
TEST(FieldRegistry, RoleLocalOrdinals) {
  FieldRegistry reg;
  EXPECT_EQ(add_field(reg, "weight", FieldRole::DPS, DataType(DataType::Float32), 1), 0u);
  EXPECT_EQ(add_field(reg, "fa", FieldRole::DPV, DataType(DataType::Float32), 1), 0u);
  EXPECT_EQ(add_field(reg, "bundle", FieldRole::DPS, DataType(DataType::UInt32), 1), 1u);
  EXPECT_EQ(add_field(reg, "colour", FieldRole::DPV, DataType(DataType::UInt8), 3), 1u);

  EXPECT_EQ(reg.dps_count(), 2u);
  EXPECT_EQ(reg.dpv_count(), 2u);
  EXPECT_EQ(reg.ordinal("bundle", FieldRole::DPS), 1u);
  EXPECT_EQ(reg.ordinal("colour", FieldRole::DPV), 1u);
  EXPECT_FALSE(reg.ordinal("missing").has_value());
}

// A duplicate name+role registration is rejected with a clean Exception.
TEST(FieldRegistry, DuplicateRejected) {
  FieldRegistry reg;
  add_field(reg, "fa", FieldRole::DPV, DataType(DataType::Float32), 1);
  EXPECT_THROW(add_field(reg, "fa", FieldRole::DPV, DataType(DataType::Float32), 1), MR::Exception);
  // Same name, different role is allowed (distinct ordinal spaces).
  EXPECT_NO_THROW(add_field(reg, "fa", FieldRole::DPS, DataType(DataType::Float32), 1));
}

// The Shared object precomputes an identity pass-through map when the output
//   registry equals the input registry (the lossless-copy case, §2.7).
TEST(Shared, IdentityPassThrough) {
  FieldRegistry reg;
  add_field(reg, "weight2", FieldRole::DPS, DataType(DataType::Float32), 1);
  add_field(reg, "bundle", FieldRole::DPS, DataType(DataType::UInt32), 1);
  add_field(reg, "fa", FieldRole::DPV, DataType(DataType::Float32), 1);

  Shared shared(reg);
  EXPECT_EQ(shared.dps_passthrough().size(), 2u);
  EXPECT_EQ(shared.dpv_passthrough().size(), 1u);
  for (const auto &p : shared.dps_passthrough())
    EXPECT_EQ(p.input_ordinal, p.output_ordinal);
}

// A field present in both registries but with mismatched dtype or M is NOT a
//   pass-through (it would require conversion the framework does not do, §2.7).
TEST(Shared, IncompatibleFieldsExcluded) {
  FieldRegistry in;
  add_field(in, "same", FieldRole::DPS, DataType(DataType::Float32), 1);
  add_field(in, "dtype_changed", FieldRole::DPS, DataType(DataType::UInt8), 1);
  add_field(in, "m_changed", FieldRole::DPV, DataType(DataType::Float32), 1);
  add_field(in, "dropped", FieldRole::DPS, DataType(DataType::Float32), 2);

  FieldRegistry out;
  add_field(out, "same", FieldRole::DPS, DataType(DataType::Float32), 1);
  add_field(out, "dtype_changed", FieldRole::DPS, DataType(DataType::Float32), 1); // dtype differs
  add_field(out, "m_changed", FieldRole::DPV, DataType(DataType::Float32), 3);     // M differs

  Shared shared(in, out);
  // Only "same" passes through; the others are excluded.
  ASSERT_EQ(shared.dps_passthrough().size(), 1u);
  EXPECT_TRUE(shared.dpv_passthrough().empty());
  EXPECT_EQ(shared.input()[shared.dps_passthrough()[0].input_ordinal].name, "same");
}

// carry_passthrough() copies pass-through fields verbatim (native dtype) and
//   sizes the output payload to the output registry.
TEST(Shared, CarryPassThroughCopiesNativeDtype) {
  FieldRegistry reg;
  add_field(reg, "bundle", FieldRole::DPS, DataType(DataType::UInt32), 1);
  add_field(reg, "fa", FieldRole::DPV, DataType(DataType::Float32), 1);
  Shared shared(reg);

  TractogramItem<float> in;
  in.dps.resize(1);
  in.dpv.resize(1);
  {
    ScalarOrVector<uint32_t> b(1);
    b(0, 0) = 7u;
    in.dps[0] = make_dps(std::move(b));
  }
  {
    VectorOrMatrix<float> f(3, 1);
    f.col(0) << 0.1F, 0.2F, 0.3F;
    in.dpv[0] = make_dpv(std::move(f));
  }

  TractogramItem<float> out;
  shared.carry_passthrough(in, out);
  ASSERT_EQ(out.dps.size(), 1u);
  ASSERT_EQ(out.dpv.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<ScalarOrVector<uint32_t>>(out.dps[0]));
  EXPECT_EQ(std::get<ScalarOrVector<uint32_t>>(out.dps[0]).scalar(), 7u);
  EXPECT_FLOAT_EQ(std::get<VectorOrMatrix<float>>(out.dpv[0]).vector()(2), 0.3F);
}

} // namespace
