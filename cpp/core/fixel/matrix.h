/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include "file/ofstream.h"
#include "fixel/fixel.h"
#include "image.h"
#include "types.h"

namespace MR::Fixel::Matrix {

using index_image_type = uint64_t;
using fixel_index_type = MR::Fixel::index_type;
using count_type = uint32_t;
using connectivity_value_type = float;

class MappedTrack : public std::vector<fixel_index_type> {
public:
  using BaseType = std::vector<fixel_index_type>;
  default_type get_weight() const { return weight; }
  void set_weight(const default_type w) { weight = w; }

private:
  default_type weight;
};

class InitElementBase {
public:
  InitElementBase() : fixel_index(std::numeric_limits<fixel_index_type>::max()) {}
  InitElementBase(const fixel_index_type fixel_index) : fixel_index(fixel_index) {}
  InitElementBase(const InitElementBase &) = default;
  FORCE_INLINE InitElementBase &operator=(const InitElementBase &that) {
    fixel_index = that.fixel_index;
    return *this;
  }
  FORCE_INLINE fixel_index_type index() const { return fixel_index; }
  FORCE_INLINE bool operator<(const InitElementBase &that) const { return fixel_index < that.fixel_index; }

private:
  fixel_index_type fixel_index;
};

class InitElementUnweighted : private InitElementBase {
public:
  using BaseType = InitElementBase;
  using BaseType::operator<;
  using ValueType = count_type;
  InitElementUnweighted() : track_count(0) {}
  InitElementUnweighted(const fixel_index_type fixel_index) : BaseType(fixel_index), track_count(1) {}
  InitElementUnweighted(const fixel_index_type fixel_index, const MappedTrack &all_data)
      : BaseType(fixel_index), track_count(1) {}
  InitElementUnweighted(const InitElementUnweighted &) = default;
  FORCE_INLINE fixel_index_type index() const { return BaseType::index(); }
  FORCE_INLINE InitElementUnweighted &operator++() {
    track_count++;
    return *this;
  }
  FORCE_INLINE InitElementUnweighted &operator=(const InitElementUnweighted &that) {
    BaseType::operator=(that);
    track_count = that.track_count;
    return *this;
  }
  FORCE_INLINE ValueType value() const { return track_count; }

private:
  ValueType track_count;
};

class InitElementWeighted : private InitElementBase {
public:
  using BaseType = InitElementBase;
  using BaseType::operator<;
  using ValueType = connectivity_value_type;
  InitElementWeighted() : sum_weights(ValueType(0)) {}
  InitElementWeighted(const fixel_index_type fixel_index) = delete;
  InitElementWeighted(const fixel_index_type fixel_index, const MappedTrack &all_data)
      : BaseType(fixel_index), sum_weights(all_data.get_weight()) {}
  InitElementWeighted(const InitElementWeighted &) = default;
  FORCE_INLINE fixel_index_type index() const { return BaseType::index(); }
  FORCE_INLINE InitElementWeighted &operator+=(const ValueType increment) {
    sum_weights += increment;
    return *this;
  }
  FORCE_INLINE InitElementWeighted &operator=(const InitElementWeighted &that) {
    BaseType::operator=(that);
    sum_weights = that.sum_weights;
    return *this;
  }
  FORCE_INLINE ValueType value() const { return sum_weights; }

private:
  ValueType sum_weights;
};

template <class ElementType> class InitFixelBase : public std::vector<ElementType> {
public:
  using BaseType = std::vector<ElementType>;
  virtual ~InitFixelBase() {}
  void add(const MappedTrack &mapped_track);
  virtual default_type norm_factor() const = 0;

protected:
  virtual void increment(const MappedTrack &data) = 0;
  virtual void increment(ElementType &element, const MappedTrack &data) = 0;
};

class InitFixelUnweighted : public InitFixelBase<InitElementUnweighted> {
public:
  using BaseType = InitFixelBase<InitElementUnweighted>;
  InitFixelUnweighted() : track_count(0) {}
  default_type norm_factor() const override { return 1.0 / static_cast<default_type>(track_count); }

private:
  count_type track_count;
  void increment(const MappedTrack &data) override { ++track_count; }
  void increment(InitElementUnweighted &element, const MappedTrack &data) override { ++element; }
};

class InitFixelWeighted : public InitFixelBase<InitElementWeighted> {
public:
  using BaseType = InitFixelBase<InitElementWeighted>;
  InitFixelWeighted() : sum_weights(default_type(0)) {}
  default_type norm_factor() const override { return 1.0 / sum_weights; }

private:
  default_type sum_weights;
  void increment(const MappedTrack &data) override { sum_weights += data.get_weight(); }
  void increment(InitElementWeighted &element, const MappedTrack &data) override { element += data.get_weight(); }
};

using InitMatrixUnweighted = std::vector<InitFixelUnweighted>;
using InitMatrixWeighted = std::vector<InitFixelWeighted>;

// A class to store fixel index / connectivity value pairs
//   only after the connectivity matrix has been thresholded / normalised
class NormElement {
public:
  using ValueType = connectivity_value_type;
  NormElement(const fixel_index_type fixel_index, const ValueType connectivity_value)
      : fixel_index(fixel_index), connectivity_value(connectivity_value) {}
  FORCE_INLINE fixel_index_type index() const { return fixel_index; }
  FORCE_INLINE ValueType value() const { return connectivity_value; }
  FORCE_INLINE void exponentiate(const ValueType C) { connectivity_value = std::pow(connectivity_value, C); }
  FORCE_INLINE void normalise(const ValueType norm_factor) { connectivity_value *= norm_factor; }

private:
  fixel_index_type fixel_index;
  ValueType connectivity_value;
};

// With the internally normalised CFE expression, want to store a
//   multiplicative factor per fixel
class NormFixel : public std::vector<NormElement> {
public:
  using ElementType = NormElement;
  using BaseType = std::vector<NormElement>;
  using ValueType = connectivity_value_type;
  NormFixel() : norm_multiplier(ValueType(1)) {}
  NormFixel(const BaseType &i) : BaseType(i), norm_multiplier(ValueType(1)) {}
  NormFixel(BaseType &&i) : BaseType(std::move(i)), norm_multiplier(ValueType(1)) {}
  void normalise() {
    norm_multiplier = ValueType(0);
    for (const auto &c : *this)
      norm_multiplier += c.value();
    norm_multiplier = norm_multiplier ? (ValueType(1) / norm_multiplier) : ValueType(0);
  }
  void normalise(const ValueType sum) { norm_multiplier = sum ? (ValueType(1) / sum) : ValueType(0); }
  ValueType norm_multiplier;
};

// Generate a fixel-fixel connectivity matrix
InitMatrixUnweighted generate_unweighted(const std::string &track_filename,
                                         Image<fixel_index_type> &index_image,
                                         Image<bool> &fixel_mask,
                                         const float angular_threshold);

InitMatrixWeighted generate_weighted(const std::string &track_filename,
                                     Image<fixel_index_type> &index_image,
                                     Image<bool> &fixel_mask,
                                     const float angular_threshold);

template <class MatrixType> class Writer {
public:
  Writer(MatrixType &matrix, const connectivity_value_type threshold) : matrix(matrix), threshold(threshold) {}
  void set_keyvals(KeyValues &kv) { keyvals = kv; }
  void set_count_path(const std::string &path);
  void set_extent_path(const std::string &path);
  void save(const std::string &path) const;

private:
  MatrixType &matrix;
  const connectivity_value_type threshold;
  KeyValues keyvals;
  mutable Image<count_type> count_image;
  mutable Image<connectivity_value_type> extent_image;
};

// Wrapper class for reading the connectivity matrix from the filesystem
class Reader {

public:
  Reader(const std::string &path, const Image<bool> &mask);
  Reader(const std::string &path);

  // TODO Entirely feasible to construct this thing using scratch storage;
  //   would need two passes over the pre-normalised data in order to calculate
  //   the number of fixel-fixel connections, but it could be done
  //
  // It would require restoration of the old Matrix::normalise() function,
  //   but modification to write out to scratch index / fixel / value images
  //   rather than "norm_matrix_type"
  //
  // This would permit usage of fixelcfestats with tractogram as input
  //
  // TODO Could pre-exponentiation of connectivity values be done beforehand using an mrcalc call?
  // Expect fixelcfestats to be provided with a data file, from which it will find the
  //   index & fixel images

  NormFixel operator[](const size_t index) const;

  // TODO Define iteration constructs?

  size_t size() const { return index_image.size(0); }
  size_t size(const size_t) const;

protected:
  const std::string directory;
  // Not to be manipulated directly; need to copy in order to ensure thread-safety
  Image<index_image_type> index_image;
  Image<fixel_index_type> fixel_image;
  Image<connectivity_value_type> value_image;
  Image<bool> mask_image;
};

} // namespace MR::Fixel::Matrix
