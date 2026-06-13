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

#include "gtest/gtest.h"

#include "dwi/tractography/nonfinite.h"

#include "exception.h"

#include <cmath>
#include <limits>
#include <variant>

using namespace MR;
using namespace MR::DWI::Tractography;

namespace {

constexpr float finite_value = 1.5F;
const float nan_value = std::numeric_limits<float>::quiet_NaN();
const float inf_value = std::numeric_limits<float>::infinity();

Streamline<float> make_streamline(const std::vector<Eigen::Matrix<float, 3, 1>> &vertices) {
  Streamline<float> tck;
  tck.set_index(0);
  for (const auto &v : vertices)
    tck.push_back(v);
  return tck;
}

} // namespace

// The single-value classifier honours each tolerance: a finite value is always
//   accepted; NaN unless Forbidden; infinity only under Any.
TEST(NonFinite, PermittedSingleValue) {
  using NF = MR::DWI::Tractography::Formats::NonFinite;
  EXPECT_TRUE(nonfinite_permitted(NF::Forbidden, finite_value));
  EXPECT_FALSE(nonfinite_permitted(NF::Forbidden, nan_value));
  EXPECT_FALSE(nonfinite_permitted(NF::Forbidden, inf_value));

  EXPECT_TRUE(nonfinite_permitted(NF::NaNOnly, finite_value));
  EXPECT_TRUE(nonfinite_permitted(NF::NaNOnly, nan_value));
  EXPECT_FALSE(nonfinite_permitted(NF::NaNOnly, inf_value));

  EXPECT_TRUE(nonfinite_permitted(NF::Any, finite_value));
  EXPECT_TRUE(nonfinite_permitted(NF::Any, nan_value));
  EXPECT_TRUE(nonfinite_permitted(NF::Any, inf_value));
}

// The content overload composes the per-value rule across a scanned channel.
TEST(NonFinite, PermittedContent) {
  using NF = MR::DWI::Tractography::Formats::NonFinite;
  NonFiniteContent finite_only;
  NonFiniteContent has_nan;
  has_nan.nan = true;
  NonFiniteContent has_inf;
  has_inf.inf = true;

  EXPECT_TRUE(nonfinite_permitted(NF::Forbidden, finite_only));
  EXPECT_FALSE(nonfinite_permitted(NF::Forbidden, has_nan));
  EXPECT_FALSE(nonfinite_permitted(NF::Forbidden, has_inf));
  EXPECT_TRUE(nonfinite_permitted(NF::NaNOnly, has_nan));
  EXPECT_FALSE(nonfinite_permitted(NF::NaNOnly, has_inf));
  EXPECT_TRUE(nonfinite_permitted(NF::Any, has_inf));
}

TEST(NonFinite, ScanVertices) {
  const Streamline<float> finite = make_streamline({{0.0F, 1.0F, 2.0F}, {3.0F, 4.0F, 5.0F}});
  EXPECT_FALSE(scan_vertices(finite).any());

  const Streamline<float> with_nan = make_streamline({{0.0F, nan_value, 2.0F}});
  EXPECT_TRUE(scan_vertices(with_nan).nan);
  EXPECT_FALSE(scan_vertices(with_nan).inf);

  const Streamline<float> with_inf = make_streamline({{inf_value, 1.0F, 2.0F}});
  EXPECT_TRUE(scan_vertices(with_inf).inf);
  EXPECT_FALSE(scan_vertices(with_inf).nan);
}

TEST(NonFinite, ScanScalars) {
  TrackScalar<float> finite;
  finite.push_back(1.0F);
  finite.push_back(2.0F);
  EXPECT_FALSE(scan_scalars(finite).any());

  TrackScalar<float> mixed;
  mixed.push_back(nan_value);
  mixed.push_back(inf_value);
  EXPECT_TRUE(scan_scalars(mixed).nan);
  EXPECT_TRUE(scan_scalars(mixed).inf);
}

// enforce_vertices throws only for content the tolerance forbids.
TEST(NonFinite, EnforceVertices) {
  using NF = MR::DWI::Tractography::Formats::NonFinite;
  const Streamline<float> finite = make_streamline({{0.0F, 1.0F, 2.0F}});
  const Streamline<float> with_nan = make_streamline({{0.0F, nan_value, 2.0F}});
  const Streamline<float> with_inf = make_streamline({{inf_value, 1.0F, 2.0F}});

  EXPECT_NO_THROW(enforce_vertices(finite, NF::Forbidden));

  EXPECT_THROW(enforce_vertices(with_nan, NF::Forbidden), Exception);
  EXPECT_THROW(enforce_vertices(with_inf, NF::Forbidden), Exception);

  EXPECT_NO_THROW(enforce_vertices(with_nan, NF::NaNOnly));
  EXPECT_THROW(enforce_vertices(with_inf, NF::NaNOnly), Exception);

  EXPECT_NO_THROW(enforce_vertices(with_inf, NF::Any));
}

// Culling a streamline's vertices sub-samples every per-vertex (dpv) field to the
//   retained rows, keeping the sidecar aligned with the vertices.
TEST(NonFinite, SelectDpvVertices) {
  TractogramItem<float> item;
  VectorOrMatrix<float> field(4, 1);
  field(0, 0) = 10.0F;
  field(1, 0) = 20.0F;
  field(2, 0) = 30.0F;
  field(3, 0) = 40.0F;
  item.dpv.push_back(make_dpv(std::move(field)));

  select_dpv_vertices(item, {0, 2, 3});

  const auto &result = std::get<VectorOrMatrix<float>>(item.dpv.front());
  ASSERT_EQ(result.rows(), 3);
  ASSERT_EQ(result.cols(), 1);
  EXPECT_FLOAT_EQ(result(0, 0), 10.0F);
  EXPECT_FLOAT_EQ(result(1, 0), 30.0F);
  EXPECT_FLOAT_EQ(result(2, 0), 40.0F);
}
