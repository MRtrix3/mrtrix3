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
#include "dwi/tractography/sidecar_value.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/tractogram_item.h"

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

} // namespace
