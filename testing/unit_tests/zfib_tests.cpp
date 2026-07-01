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

#include "dwi/tractography/compression/huffman.h"
#include "dwi/tractography/compression/quantization.h"
#include "dwi/tractography/compression/ramer_douglas_peucker.h"
#include "dwi/tractography/streamline.h"

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <map>
#include <vector>

using MR::DWI::Tractography::Streamline;
using MR::DWI::Tractography::Compression::Huffman;
namespace Compression = MR::DWI::Tractography::Compression;

namespace {

Streamline<float> make_streamline(const std::vector<Eigen::Vector3f> &points) {
  Streamline<float> tck;
  for (const Eigen::Vector3f &p : points)
    tck.push_back(p);
  return tck;
}

} // namespace

/* ************************************************************************ */
/*                  Ramer–Douglas–Peucker linearization                    */
/* ************************************************************************ */

// Perfectly collinear interior vertices are all removed; only the endpoints
//   remain.
TEST(ZFIBLinearize, CollinearInteriorDropped) {
  const Streamline<float> tck = make_streamline({{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {3, 0, 0}, {4, 0, 0}});
  const std::vector<size_t> kept = Compression::rdp_retained_indices<float>(tck, 0.01F);
  ASSERT_EQ(kept.size(), 2u);
  EXPECT_EQ(kept.front(), 0u);
  EXPECT_EQ(kept.back(), 4u);
}

// A single interior vertex deviating beyond the tolerance is retained.
TEST(ZFIBLinearize, SpikeRetained) {
  const Streamline<float> tck = make_streamline({{0, 0, 0}, {1, 1, 0}, {2, 0, 0}});
  const Streamline<float> out = Compression::linearize<float>(tck, 0.5F);
  // The interior vertex deviates by 1 mm from the chord (> 0.5 mm tolerance) and
  //   must be kept, along with the two endpoints.
  ASSERT_EQ(out.size(), 3u);
  EXPECT_EQ(out[0], tck[0]);
  EXPECT_EQ(out[1], tck[1]);
  EXPECT_EQ(out[2], tck[2]);
}

// Endpoints, weight and ordering index survive linearization.
TEST(ZFIBLinearize, EndpointsWeightIndexPreserved) {
  Streamline<float> tck = make_streamline({{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {3, 0, 0}});
  tck.weight = 2.5F;
  tck.set_index(7);
  const Streamline<float> out = Compression::linearize<float>(tck, 0.01F);
  EXPECT_FLOAT_EQ(out.weight, 2.5F);
  EXPECT_EQ(out.get_index(), 7u);
  ASSERT_GE(out.size(), 2u);
  EXPECT_EQ(out.front(), tck.front());
  EXPECT_EQ(out.back(), tck.back());
}

// A streamline of fewer than three vertices has no interior point to drop and is
//   returned unchanged.
TEST(ZFIBLinearize, ShortStreamlineUnchanged) {
  const Streamline<float> tck = make_streamline({{0, 0, 0}, {9, 9, 9}});
  const Streamline<float> out = Compression::linearize<float>(tck, 0.01F);
  ASSERT_EQ(out.size(), 2u);
  EXPECT_EQ(out[0], tck[0]);
  EXPECT_EQ(out[1], tck[1]);
}

/* ************************************************************************ */
/*                          Uniform quantization                           */
/* ************************************************************************ */

TEST(ZFIBQuantize, RoundsToGrid) {
  EXPECT_FLOAT_EQ(Compression::uniform_quantize(1.234, 0), 1.0F);
  EXPECT_FLOAT_EQ(Compression::uniform_quantize(1.27, -1), 1.3F);
  EXPECT_FLOAT_EQ(Compression::uniform_quantize(-1.27, -1), -1.3F);
}

TEST(ZFIBQuantize, NegativeZeroNormalised) {
  const float q = Compression::uniform_quantize(-0.01, 0);
  EXPECT_FLOAT_EQ(q, 0.0F);
  EXPECT_FALSE(std::signbit(q));
}

TEST(ZFIBQuantize, ErrorBudgetMatchesTable) {
  // α = √3·10ᵖ (Table 7: p=0 → ≈1.73205, p=-1 → ≈0.17321).
  EXPECT_NEAR(Compression::quantization_error(0), std::sqrt(3.0), 1e-9);
  EXPECT_NEAR(Compression::quantization_error(-1), std::sqrt(3.0) * 0.1, 1e-9);
}

TEST(ZFIBQuantize, PrecisionSelection) {
  EXPECT_EQ(Compression::select_precision(0.1), -1);
  EXPECT_EQ(Compression::select_precision(0.19), -1);
  EXPECT_EQ(Compression::select_precision(0.2), 0);
  EXPECT_EQ(Compression::select_precision(0.5), 0);
}

/* ************************************************************************ */
/*                            Huffman coding                               */
/* ************************************************************************ */

TEST(ZFIBHuffman, LosslessRoundTrip) {
  const std::map<float, uint64_t> histogram = {{1.0F, 5}, {2.0F, 3}, {3.0F, 1}, {4.0F, 1}};
  const Huffman coder = Huffman::from_histogram(histogram);
  const std::vector<float> message = {1, 2, 3, 4, 1, 1, 2, 1, 3, 2};
  const Huffman::Encoded encoded = coder.encode(message);
  const std::vector<float> decoded = coder.decode(encoded.bytes.data(), encoded.bytes.size(), message.size());
  EXPECT_EQ(decoded, message);
}

TEST(ZFIBHuffman, SingleSymbol) {
  const std::map<float, uint64_t> histogram = {{7.5F, 4}};
  const Huffman coder = Huffman::from_histogram(histogram);
  const std::vector<float> message = {7.5F, 7.5F, 7.5F};
  const Huffman::Encoded encoded = coder.encode(message);
  // A single-symbol code word is empty, so no bits (and no bytes) are emitted.
  EXPECT_TRUE(encoded.bytes.empty());
  EXPECT_EQ(encoded.bit_count, size_t(0));
  const std::vector<float> decoded = coder.decode(encoded.bytes.data(), encoded.bytes.size(), message.size());
  EXPECT_EQ(decoded, message);
}

TEST(ZFIBHuffman, FrequencyTieRoundTrips) {
  // All symbols equally frequent: the storage-order tie-break must still yield a
  //   valid, invertible code.
  const std::map<float, uint64_t> histogram = {{1.0F, 2}, {2.0F, 2}, {3.0F, 2}, {4.0F, 2}};
  const Huffman coder = Huffman::from_histogram(histogram);
  const std::vector<float> message = {4, 3, 2, 1, 1, 2, 3, 4};
  const Huffman::Encoded encoded = coder.encode(message);
  const std::vector<float> decoded = coder.decode(encoded.bytes.data(), encoded.bytes.size(), message.size());
  EXPECT_EQ(decoded, message);
}

// The determinism contract: a coder rebuilt from the stored dictionary produces
//   byte-identical encodings and decodes identically to the writer's coder.
TEST(ZFIBHuffman, HistogramDictionaryEquivalence) {
  const std::map<float, uint64_t> histogram = {{-2.0F, 7}, {0.0F, 11}, {3.5F, 4}, {9.0F, 1}, {12.0F, 6}};
  const Huffman from_hist = Huffman::from_histogram(histogram);
  const Huffman from_dict = Huffman::from_dictionary(from_hist.dictionary());

  const std::vector<float> message = {0, 0, -2, 12, 3.5F, 9, 0, 12, -2, 3.5F, 0, 12};
  const Huffman::Encoded encoded_a = from_hist.encode(message);
  const Huffman::Encoded encoded_b = from_dict.encode(message);
  EXPECT_EQ(encoded_a.bytes, encoded_b.bytes);
  EXPECT_EQ(encoded_a.bit_count, encoded_b.bit_count);

  const std::vector<float> decoded = from_dict.decode(encoded_a.bytes.data(), encoded_a.bytes.size(), message.size());
  EXPECT_EQ(decoded, message);
}
