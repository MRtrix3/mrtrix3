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

#include <limits>
#include <utility>

#include "types.h"

namespace MR::DWI::Tractography {

// Base class for storing an index alongside either streamline vertex or track scalar data
//
class DataIndex {
public:
  static constexpr size_t invalid = std::numeric_limits<size_t>::max();
  DataIndex() : index(invalid) {}
  DataIndex(const size_t i) : index(i) {}
  DataIndex(const DataIndex &i) : index(i.index) {}
  DataIndex(DataIndex &&i) noexcept : index(i.index) { i.index = invalid; }
  DataIndex &operator=(const DataIndex &i) {
    index = i.index;
    return *this;
  }
  DataIndex &operator=(DataIndex &&i) noexcept {
    index = i.index;
    i.index = invalid;
    return *this;
  }
  void set_index(const size_t i) { index = i; }
  [[nodiscard]] size_t get_index() const { return index; }
  void clear() { index = invalid; }
  bool operator<(const DataIndex &i) const { return index < i.index; }

private:
  size_t index;
};

// A class for track scalars
template <typename ValueType = float> class TrackScalar : public std::vector<ValueType>, public DataIndex {
public:
  using value_type = ValueType;
  using std::vector<ValueType>::vector;
  TrackScalar() = default;
  TrackScalar(const TrackScalar &) = default;
  TrackScalar(TrackScalar &&that) noexcept : std::vector<value_type>(std::move(that)), DataIndex(std::move(that)) {}
  TrackScalar &operator=(const TrackScalar &that) = default;
  void clear() {
    std::vector<ValueType>::clear();
    DataIndex::clear();
  }
};

template <typename ValueType = float>
class Streamline : public std::vector<Eigen::Matrix<ValueType, 3, 1>>, public DataIndex {
public:
  using point_type = Eigen::Matrix<ValueType, 3, 1>;
  using tangent_type = point_type;
  using value_type = ValueType;

  Streamline() : weight(1.0F) {}

  Streamline(size_t size) : std::vector<point_type>(size), weight(value_type(1.0)) {}

  Streamline(size_t size, const point_type &fill) : std::vector<point_type>(size, fill), weight(value_type(1.0)) {}

  Streamline(const Streamline &) = default;
  Streamline &operator=(const Streamline &that) = default;

  Streamline(Streamline &&that) noexcept
      : std::vector<point_type>(std::move(static_cast<std::vector<point_type> &&>(that))),
        DataIndex(std::move(static_cast<DataIndex &&>(that))),
        weight(that.weight) {
    that.weight = std::numeric_limits<float>::quiet_NaN();
  }

  Streamline(const std::vector<point_type> &tck) : std::vector<point_type>(tck), DataIndex(), weight(1.0) {}

  Streamline &operator=(Streamline &&that) noexcept {
    std::vector<point_type>::operator=(std::move(static_cast<std::vector<point_type> &&>(that)));
    DataIndex::operator=(std::move(static_cast<DataIndex &&>(that)));
    weight = that.weight;
    that.weight = 0.0F;
    return *this;
  }

  void clear() {
    std::vector<point_type>::clear();
    DataIndex::clear();
    weight = 1.0;
  }

  float weight;
};

template <typename PointType> typename PointType::Scalar length(const std::vector<PointType> &tck) {
  if (tck.empty())
    return std::numeric_limits<typename PointType::Scalar>::quiet_NaN();
  auto value = typename PointType::Scalar(0);
  for (size_t i = 1; i != tck.size(); ++i)
    value += (tck[i] - tck[i - 1]).norm();
  return value;
}

template <typename PointType> PointType tangent(const std::vector<PointType> &tck, const size_t index) {
  assert(index < tck.size());
  if (tck.size() < 2)
    return PointType::Constant(std::numeric_limits<typename PointType::Scalar>::quiet_NaN());
  if (!index)
    return (tck[1] - tck[0]).normalized();
  if (index == tck.size() - 1)
    return (tck[index] - tck[index - 1]).normalized();
  const PointType offset_prev = tck[index] - tck[index - 1];
  const PointType offset_next = tck[index + 1] - tck[index];
  const typename PointType::Scalar dist_prev = offset_prev.norm();
  const typename PointType::Scalar dist_next = offset_next.norm();
  if (dist_prev == typename PointType::Scalar(0)) {
    return (dist_next == typename PointType::Scalar(0)
                ? PointType::Constant(std::numeric_limits<typename PointType::Scalar>::quiet_NaN())
                : offset_next.normalized());
  }
  if (dist_next == typename PointType::Scalar(0)) {
    return offset_prev.normalized();
  }
  // Greater weight given to the shorter step
  return (dist_next * offset_prev.normalized() + dist_prev * offset_next.normalized()).normalized();
}

} // namespace MR::DWI::Tractography
