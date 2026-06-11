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
#include "dwi/tractography/formats/pipe.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/scalar_file.h"
#include "dwi/tractography/shared.h"
#include "dwi/tractography/sidecar.h"
#include "dwi/tractography/sidecar_value.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/tractogram_item.h"
#include "exception.h"
#include "file/matrix.h"
#include "file/npy.h"

#include <gtest/gtest.h>

#include <Eigen/Core>

#include <cstdint>
#include <filesystem>
#include <string>
#include <variant>

#include <sys/wait.h>
#include <unistd.h>

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

//! build a registry carrying multiple dps/dpv fields of mixed shape and dtype.
MR::DWI::Tractography::FieldRegistry make_sidecar_registry() {
  using namespace MR::DWI::Tractography;
  FieldRegistry reg;
  add_field(reg, "selection", FieldRole::DPS, DataType(DataType::Bit), 1);                      // dps M=1 bit
  add_field(reg, "bundle", FieldRole::DPS, DataType(DataType::UInt32), 1);                      // dps M=1 int
  add_field(reg, "centroid", FieldRole::DPS, DataType::native(DataType(DataType::Float32)), 3); // dps M>1
  add_field(reg, "fa", FieldRole::DPV, DataType::native(DataType(DataType::Float32)), 1);       // dpv M=1
  add_field(reg, "colour", FieldRole::DPV, DataType(DataType::UInt8), 3);                       // dpv M>1
  return reg;
}

//! populate one item's sidecar payload deterministically from its index.
void fill_item(MR::DWI::Tractography::TractogramItem<float> &item,
               const MR::DWI::Tractography::FieldRegistry &reg,
               size_t s,
               size_t n_vertices) {
  using namespace MR::DWI::Tractography;
  item.streamline.clear();
  for (size_t v = 0; v != n_vertices; ++v)
    item.streamline.push_back(Eigen::Vector3f(static_cast<float>(s), static_cast<float>(v), static_cast<float>(s + v)));
  item.set_index(s);

  item.dps.resize(reg.dps_count());
  item.dpv.resize(reg.dpv_count());
  {
    ScalarOrVector<uint8_t> sel(1);
    sel(0, 0) = static_cast<uint8_t>(s % 2);
    item.dps[*reg.ordinal("selection", FieldRole::DPS)] = make_dps(std::move(sel));
  }
  {
    ScalarOrVector<uint32_t> b(1);
    b(0, 0) = static_cast<uint32_t>(100 + s);
    item.dps[*reg.ordinal("bundle", FieldRole::DPS)] = make_dps(std::move(b));
  }
  {
    ScalarOrVector<float> c(3);
    c << static_cast<float>(s), static_cast<float>(s) + 0.5F, static_cast<float>(s) + 1.5F;
    item.dps[*reg.ordinal("centroid", FieldRole::DPS)] = make_dps(std::move(c));
  }
  {
    VectorOrMatrix<float> fa(static_cast<Eigen::Index>(n_vertices), 1);
    for (size_t v = 0; v != n_vertices; ++v)
      fa(static_cast<Eigen::Index>(v), 0) = 0.1F * static_cast<float>(v + s);
    item.dpv[*reg.ordinal("fa", FieldRole::DPV)] = make_dpv(std::move(fa));
  }
  {
    VectorOrMatrix<uint8_t> col(static_cast<Eigen::Index>(n_vertices), 3);
    for (size_t v = 0; v != n_vertices; ++v) {
      col(static_cast<Eigen::Index>(v), 0) = static_cast<uint8_t>(v);
      col(static_cast<Eigen::Index>(v), 1) = static_cast<uint8_t>(s);
      col(static_cast<Eigen::Index>(v), 2) = static_cast<uint8_t>(v + s);
    }
    item.dpv[*reg.ordinal("colour", FieldRole::DPV)] = make_dpv(std::move(col));
  }
}

// End-to-end pipe round-trip carrying multiple dps (M=1 and M>1) and dpv
//   (M=1 and M>1) fields, including an integer and a bit field, through a real
//   OS pipe. Asserts the registry is reconstructed from the header, every field
//   survives in its NATIVE dtype, the M=1 dps surfaces as a scalar, and
//   pass-through fields are carried unchanged (the PipeWriter neither inspects
//   nor modifies them). Exercises both PipeWriter and PipeReader.
TEST(PipeSidecar, RoundTripCarriesMixedFields) {
  using namespace MR::DWI::Tractography;
  constexpr size_t n_streamlines = 4;
  constexpr size_t n_vertices = 5;
  const FieldRegistry registry = make_sidecar_registry();

  int fds[2];
  ASSERT_EQ(pipe(fds), 0);

  const pid_t pid = fork();
  ASSERT_GE(pid, 0);

  if (pid == 0) {
    // Child: the writer. Redirect stdout to the pipe write end.
    close(fds[0]);
    dup2(fds[1], STDOUT_FILENO);
    close(fds[1]);
    ::testing::GTEST_FLAG(throw_on_failure) = false;
    try {
      Properties properties;
      properties["count"] = std::to_string(n_streamlines);
      PipeWriter<float> writer(properties, registry);
      for (size_t s = 0; s != n_streamlines; ++s) {
        TractogramItem<float> item;
        fill_item(item, registry, s, n_vertices);
        writer(item);
      }
    } catch (...) {
      _exit(2);
    }
    _exit(0);
  }

  // Parent: the reader. Redirect stdin from the pipe read end.
  close(fds[1]);
  dup2(fds[0], STDIN_FILENO);
  close(fds[0]);

  Properties properties;
  FieldRegistry recovered;
  PipeReader<float> reader(properties, recovered);

  // The registry must be reconstructed verbatim from the stream header.
  ASSERT_EQ(recovered.size(), registry.size());
  ASSERT_EQ(recovered.dps_count(), 3u);
  ASSERT_EQ(recovered.dpv_count(), 2u);
  const FieldDescriptor *sel = recovered.find("selection", FieldRole::DPS);
  ASSERT_NE(sel, nullptr);
  EXPECT_EQ(sel->dtype(), DataType::Bit);
  EXPECT_EQ(recovered.find("colour", FieldRole::DPV)->columns, 3u);

  std::vector<TractogramItem<float>> out;
  TractogramItem<float> item;
  while (reader(item))
    out.push_back(std::move(item));

  int status = 0;
  waitpid(pid, &status, 0);
  ASSERT_TRUE(WIFEXITED(status));
  ASSERT_EQ(WEXITSTATUS(status), 0);

  ASSERT_EQ(out.size(), n_streamlines);
  for (size_t s = 0; s != n_streamlines; ++s) {
    TractogramItem<float> expect;
    fill_item(expect, registry, s, n_vertices);
    const auto &got = out[s];
    ASSERT_EQ(got.streamline.size(), n_vertices);

    // dps M=1 bit field — native dtype preserved AND surfaces as a scalar.
    const auto &sel_v = std::get<ScalarOrVector<uint8_t>>(got.dps[*recovered.ordinal("selection", FieldRole::DPS)]);
    EXPECT_EQ(sel_v.cols(), 1); // scalar, NOT a 1-element vector that must be unwrapped
    EXPECT_EQ(sel_v.scalar(), static_cast<uint8_t>(s % 2));

    // dps M=1 integer field — native uint32 preserved.
    const auto &bundle_v = std::get<ScalarOrVector<uint32_t>>(got.dps[*recovered.ordinal("bundle", FieldRole::DPS)]);
    EXPECT_EQ(bundle_v.scalar(), static_cast<uint32_t>(100 + s));

    // dps M>1 float field — length-3 vector.
    const auto &cen_v = std::get<ScalarOrVector<float>>(got.dps[*recovered.ordinal("centroid", FieldRole::DPS)]);
    ASSERT_EQ(cen_v.cols(), 3);
    EXPECT_FLOAT_EQ(cen_v(2), static_cast<float>(s) + 1.5F);

    // dpv M=1 float field.
    const auto &fa_v = std::get<VectorOrMatrix<float>>(got.dpv[*recovered.ordinal("fa", FieldRole::DPV)]);
    ASSERT_EQ(fa_v.rows(), static_cast<Eigen::Index>(n_vertices));
    ASSERT_EQ(fa_v.cols(), 1);
    EXPECT_FLOAT_EQ(fa_v.vector()(3), 0.1F * static_cast<float>(3 + s));

    // dpv M>1 uint8 field — native dtype + n_vertices x 3 matrix.
    const auto &col_v = std::get<VectorOrMatrix<uint8_t>>(got.dpv[*recovered.ordinal("colour", FieldRole::DPV)]);
    ASSERT_EQ(col_v.rows(), static_cast<Eigen::Index>(n_vertices));
    ASSERT_EQ(col_v.cols(), 3);
    EXPECT_EQ(col_v(4, 2), static_cast<uint8_t>(4 + s));
  }
}

// §2.4 / step 4: a bare path parses as an unqualified reference (no field name);
//   behaviour is unchanged from a plain file argument.
TEST(Sidecar, ParseBarePath) {
  const SidecarReference ref = parse_sidecar_reference("weights.csv");
  EXPECT_FALSE(ref.is_qualified());
  EXPECT_FALSE(ref.name.has_value());
  EXPECT_EQ(ref.dataset, std::filesystem::path("weights.csv"));
}

// §2.4 / step 4: "DATASET::NAME" splits into a dataset path and a field name.
TEST(Sidecar, ParseQualifiedReference) {
  const SidecarReference ref = parse_sidecar_reference("tracks.tck::fa");
  EXPECT_TRUE(ref.is_qualified());
  ASSERT_TRUE(ref.name.has_value());
  EXPECT_EQ(*ref.name, "fa");
  EXPECT_EQ(ref.dataset, std::filesystem::path("tracks.tck"));
}

// §2.4 / step 4: parsing is on the LAST "::", so a field name is taken from the
//   final segment when the dataset path itself contains "::".
TEST(Sidecar, ParseSplitsOnLastDoubleColon) {
  const SidecarReference ref = parse_sidecar_reference("a::b::c");
  ASSERT_TRUE(ref.name.has_value());
  EXPECT_EQ(*ref.name, "c");
  EXPECT_EQ(ref.dataset, std::filesystem::path("a::b"));
}

// §2.4 / step 4: a Windows drive-letter path ("C:\\...", a single colon) is NOT
//   mistaken for a qualified reference — splitting on the last "::" leaves it intact.
TEST(Sidecar, ParseWindowsDriveLetterUnaffected) {
  const SidecarReference ref = parse_sidecar_reference("C:\\data\\weights.csv");
  EXPECT_FALSE(ref.is_qualified());
  EXPECT_EQ(ref.dataset, std::filesystem::path("C:\\data\\weights.csv"));
}

// §2.4 / step 4: a trailing "::" yields an empty field name (still qualified).
TEST(Sidecar, ParseTrailingDoubleColon) {
  const SidecarReference ref = parse_sidecar_reference("tracks.tck::");
  ASSERT_TRUE(ref.name.has_value());
  EXPECT_TRUE(ref.name->empty());
  EXPECT_EQ(ref.dataset, std::filesystem::path("tracks.tck"));
}

// ---------------------------------------------------------------------------
//  Step 5: standalone input-sidecar injection
// ---------------------------------------------------------------------------

class SidecarIO : public ::testing::Test {
protected:
  std::filesystem::path dir;

  void SetUp() override {
    dir = std::filesystem::temp_directory_path() /
          ("mrtrix_sidecar_io_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()));
    std::filesystem::create_directories(dir);
  }
  void TearDown() override {
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
  }
};

// Step 5: import per-streamline data from a numerical text (.csv) file; one row
//   is yielded per streamline as a dps field named after the file stem.
TEST_F(SidecarIO, ImportCsvPerStreamline) {
  const std::filesystem::path path = dir / "scale.csv";
  Eigen::MatrixXf m(3, 1);
  m << 1.5F, 2.5F, 3.5F;
  MR::File::Matrix::save_matrix(m, path);

  Properties properties;
  FieldRegistry registry;
  auto loader = make_sidecar_loader<float>(parse_sidecar_reference(path.string()), properties, registry);

  const auto ordinal = registry.ordinal("scale", FieldRole::DPS);
  ASSERT_TRUE(ordinal.has_value());

  for (float expected : {1.5F, 2.5F, 3.5F}) {
    TractogramItem<float> item;
    ASSERT_TRUE((*loader)(item));
    ASSERT_GT(item.dps.size(), *ordinal);
    const auto &v = std::get<ScalarOrVector<float>>(item.dps[*ordinal]);
    EXPECT_FLOAT_EQ(v.scalar(), expected);
  }
  TractogramItem<float> spent;
  EXPECT_FALSE((*loader)(spent));
}

// Step 5: import per-streamline data from a NumPy (.npy) file via memory-mapped
//   access, preserving a multi-column row per streamline.
TEST_F(SidecarIO, ImportNpyPerStreamline) {
  const std::filesystem::path path = dir / "metrics.npy";
  Eigen::MatrixXf m(2, 3);
  m << 1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F;
  MR::File::NPY::save_matrix(m, path);

  Properties properties;
  FieldRegistry registry;
  auto loader = make_sidecar_loader<float>(parse_sidecar_reference(path.string()), properties, registry);

  const auto ordinal = registry.ordinal("metrics", FieldRole::DPS);
  ASSERT_TRUE(ordinal.has_value());

  TractogramItem<float> item0;
  ASSERT_TRUE((*loader)(item0));
  const auto &row0 = std::get<ScalarOrVector<float>>(item0.dps[*ordinal]);
  ASSERT_EQ(row0.cols(), 3);
  EXPECT_FLOAT_EQ(row0(0, 0), 1.0F);
  EXPECT_FLOAT_EQ(row0(0, 2), 3.0F);

  TractogramItem<float> item1;
  ASSERT_TRUE((*loader)(item1));
  const auto &row1 = std::get<ScalarOrVector<float>>(item1.dps[*ordinal]);
  EXPECT_FLOAT_EQ(row1(0, 0), 4.0F);
  EXPECT_FLOAT_EQ(row1(0, 2), 6.0F);

  TractogramItem<float> spent;
  EXPECT_FALSE((*loader)(spent));
}

// Step 5: import per-vertex data from a .tsf file; each streamline's scalars
//   surface as a dpv (n_vertices x 1) field.
TEST_F(SidecarIO, ImportTsfPerVertex) {
  const std::filesystem::path path = dir / "fa.tsf";
  {
    Properties properties;
    ScalarWriter<float> writer(path, properties);
    for (size_t s = 0; s != 2; ++s) {
      TrackScalar<float> scalars;
      scalars.set_index(s);
      for (size_t v = 0; v != 3 + s; ++v)
        scalars.push_back(0.1F * static_cast<float>(s) + static_cast<float>(v));
      writer(scalars);
    }
  }

  Properties properties;
  FieldRegistry registry;
  auto loader = make_sidecar_loader<float>(parse_sidecar_reference(path.string()), properties, registry);
  const auto ordinal = registry.ordinal("fa", FieldRole::DPV);
  ASSERT_TRUE(ordinal.has_value());

  TractogramItem<float> item0;
  ASSERT_TRUE((*loader)(item0));
  const auto &v0 = std::get<VectorOrMatrix<float>>(item0.dpv[*ordinal]);
  ASSERT_EQ(v0.rows(), 3);
  ASSERT_EQ(v0.cols(), 1);
  EXPECT_FLOAT_EQ(v0.vector()(1), 1.0F);

  TractogramItem<float> item1;
  ASSERT_TRUE((*loader)(item1));
  const auto &v1 = std::get<VectorOrMatrix<float>>(item1.dpv[*ordinal]);
  EXPECT_EQ(v1.rows(), 4);
}

// Step 5: a qualified "DATASET::NAME" import reference is not yet implemented.
TEST_F(SidecarIO, QualifiedImportNotYetImplemented) {
  const std::filesystem::path tck = dir / "tracks.tck";
  Properties properties;
  FieldRegistry registry;
  EXPECT_THROW(make_sidecar_loader<float>(parse_sidecar_reference(tck.string() + "::fa"), properties, registry),
               MR::Exception);
}

// ---------------------------------------------------------------------------
//  Step 6: standalone output-sidecar export
// ---------------------------------------------------------------------------

// Step 6: export processed per-streamline data to a numerical text file; the
//   exporter stores rows by index and writes on finalise.
TEST_F(SidecarIO, ExportCsvPerStreamline) {
  const std::filesystem::path path = dir / "out.csv";
  {
    Properties properties;
    properties["count"] = "3";
    auto exporter = make_sidecar_exporter<float>(parse_sidecar_reference(path.string()), properties, false);
    for (size_t s = 0; s != 3; ++s) {
      TractogramItem<float> item;
      item.set_index(s);
      ScalarOrVector<float> v(1);
      v(0, 0) = 10.0F + static_cast<float>(s);
      item.dps.push_back(make_dps(std::move(v)));
      ASSERT_TRUE((*exporter)(item));
    }
    exporter->finalise();
  }
  const Eigen::MatrixXf m = MR::File::Matrix::load_matrix<float>(path);
  ASSERT_EQ(m.rows(), 3);
  EXPECT_FLOAT_EQ(m(0, 0), 10.0F);
  EXPECT_FLOAT_EQ(m(2, 0), 12.0F);
}

// Step 6: export processed per-streamline data to a .npy file; round-trips.
//   With no streamline count in Properties the backing array is grown via
//   conservativeResizeLike() (the std::vector<> doubling schedule).
TEST_F(SidecarIO, ExportNpyPerStreamline) {
  const std::filesystem::path path = dir / "out.npy";
  {
    Properties properties; // no count -> grown geometrically
    auto exporter = make_sidecar_exporter<float>(parse_sidecar_reference(path.string()), properties, false);
    for (size_t s = 0; s != 5; ++s) {
      TractogramItem<float> item;
      item.set_index(s);
      ScalarOrVector<float> v(2);
      v << static_cast<float>(s), static_cast<float>(s) * 2.0F;
      item.dps.push_back(make_dps(std::move(v)));
      ASSERT_TRUE((*exporter)(item));
    }
    exporter->finalise();
  }
  const Eigen::MatrixXf m = MR::File::NPY::load_matrix<float>(path);
  ASSERT_EQ(m.rows(), 5);
  ASSERT_EQ(m.cols(), 2);
  EXPECT_FLOAT_EQ(m(4, 0), 4.0F);
  EXPECT_FLOAT_EQ(m(4, 1), 8.0F);
}

// Step 6: export processed per-vertex data to a .tsf file as streamlines arrive.
TEST_F(SidecarIO, ExportTsfPerVertex) {
  const std::filesystem::path path = dir / "out.tsf";
  {
    Properties properties;
    auto exporter = make_sidecar_exporter<float>(parse_sidecar_reference(path.string()), properties, false);
    for (size_t s = 0; s != 2; ++s) {
      TractogramItem<float> item;
      item.set_index(s);
      VectorOrMatrix<float> v(static_cast<Eigen::Index>(3 + s), 1);
      for (Eigen::Index k = 0; k != v.rows(); ++k)
        v(k, 0) = static_cast<float>(k) + static_cast<float>(s);
      item.dpv.push_back(make_dpv(std::move(v)));
      ASSERT_TRUE((*exporter)(item));
    }
  } // exporter + ScalarWriter destructors flush

  Properties properties;
  ScalarReader<float> reader(path, properties);
  TrackScalar<float> scalars;
  ASSERT_TRUE(reader(scalars));
  EXPECT_EQ(scalars.size(), 3u);
  EXPECT_FLOAT_EQ(scalars[2], 2.0F);
}

// Step 6: a qualified "DATASET::NAME" export reference is not yet implemented.
TEST_F(SidecarIO, QualifiedExportNotYetImplemented) {
  const std::filesystem::path tck = dir / "tracks.tck";
  Properties properties;
  EXPECT_THROW(make_sidecar_exporter<float>(parse_sidecar_reference(tck.string() + "::fa"), properties, false),
               MR::Exception);
}

} // namespace
