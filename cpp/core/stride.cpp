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

#include "stride.h"

#include <limits>
#include <map>
#include <set>

#include "header.h"

namespace MR::Stride {

using namespace App;

// clang-format off
const OptionGroup Options = OptionGroup("Stride options")
    + Option("strides",
             "specify the strides of the output data in memory;"
             " either as a comma-separated list of (signed) integers,"
             " or as a template image from which the strides shall be extracted and used."
             " The actual strides produced will depend on whether"
             " the output image format can support it.")
      + Argument("spec").type_sequence_int().type_image_in();
// clang-format on

} // namespace MR::Stride

namespace MR::Stride::Legacy {

List &sanitise(List &current, const List &desired, const std::vector<ssize_t> &dims) {
  // remove duplicates
  for (size_t i = 0; i < current.size() - 1; ++i) {
    if (dims[i] == 1)
      current[i] = 0;
    if (!current[i])
      continue;
    for (size_t j = i + 1; j < current.size(); ++j) {
      if (!current[j])
        continue;
      if (MR::abs(current[i]) == MR::abs(current[j]))
        current[j] = 0;
    }
  }

  ssize_t desired_max = 0;
  for (size_t i = 0; i < desired.size(); ++i)
    if (MR::abs(desired[i]) > desired_max)
      desired_max = MR::abs(desired[i]);

  ssize_t in_max = 0;
  for (size_t i = 0; i < current.size(); ++i)
    if (MR::abs(current[i]) > in_max)
      in_max = MR::abs(current[i]);
  in_max += desired_max + 1;

  for (size_t i = 0; i < current.size(); ++i)
    if (dims[i] > 1 && desired[i])
      current[i] = desired[i];
    else if (current[i])
      current[i] += current[i] < 0 ? -desired_max : desired_max;
    else
      current[i] = in_max++;

  symbolise(current);
  return current;
}

List __from_command_line(const List &current) {
  List strides;
  auto opt = App::get_options("strides");
  if (opt.empty())
    return strides;

  try {
    auto header = Header::open(std::string(opt[0][0]));
    strides = get_symbolic(header);
  } catch (Exception &E) {
    E.display(3);
    try {
      auto tmp = parse_ints<int>(opt[0][0]);
      for (auto x : tmp)
        strides.push_back(x);
    } catch (Exception &E) {
      E.display(3);
      throw Exception("argument \"" + std::string(opt[0][0]) +
                      "\" to option \"-strides\" is not a list of strides or an image");
    }
  }

  if (strides.size() > current.size())
    WARN("too many axes supplied to -strides option - ignoring remaining strides");
  strides.resize(current.size(), 0);

  for (const auto x : strides)
    if (MR::abs(x) > static_cast<int>(current.size()))
      throw Exception("strides specified exceed image dimensions: got " + str(opt[0][0]) + ", but image has " +
                      str(current.size()) + " axes");

  for (size_t i = 0; i < strides.size() - 1; ++i) {
    if (!strides[1])
      continue;
    for (size_t j = i + 1; j < strides.size(); ++j)
      if (MR::abs(strides[i]) == MR::abs(strides[j]))
        throw Exception("duplicate entries provided to \"-strides\" option: " + str(opt[0][0]));
  }

  List prev = get_symbolic(current);

  for (size_t i = 0; i < strides.size(); ++i)
    if (strides[i] != 0)
      prev[i] = 0;

  prev = get_symbolic(prev);
  ssize_t max_remaining = 0;
  for (const auto x : prev)
    if (MR::abs(x) > max_remaining)
      max_remaining = MR::abs(x);

  struct FindStride {
    FindStride(List::value_type value) : x(MR::abs(value)) {}
    bool operator()(List::value_type a) { return MR::abs(a) == x; }
    const List::value_type x;
  };

  ssize_t next_avail = 0;
  for (ssize_t next = 1; next <= max_remaining; ++next) {
    auto p = std::find_if(prev.begin(), prev.end(), FindStride(next));
    assert(p != prev.end());
    List::value_type s;
    while (1) {
      s = *p + (*p > 0 ? next_avail : -next_avail);
      if (std::find_if(strides.begin(), strides.end(), FindStride(s)) == strides.end())
        break;
      ++next_avail;
    }
    strides[std::distance(prev.begin(), p)] = s;
  }

  return strides;
}

} // namespace MR::Stride::Legacy

namespace MR::Stride {

std::ostream &operator<<(std::ostream &stream, const Base &base) {
  if (base.empty()) {
    stream << "<NULL>";
    return stream;
  }
  stream << base[0];
  for (Eigen::Index axis = 1; axis != base.size(); ++axis)
    stream << "," << base[axis];
  return stream;
}

Order::Order(const Permutation &permutation) : Base(permutation.size(), Order::invalid) {
  assert(permutation.valid());
  for (Eigen::Index axis = 0; axis != permutation.size(); ++axis)
    data_[permutation[axis]] = axis;
}

Order::Order(const ListType &data) : Base(data) {}

Order::operator Axes::Subset() const { return Axes::Subset(data_.begin(), data_.end()); }

bool Order::is_canonical() const {
  for (Eigen::Index axis = 0; axis != size(); ++axis) {
    if (data_[axis] != axis)
      return false;
  }
  return true;
}

bool Order::is_sanitised() const {
  if (!valid())
    return false;
  if (*std::min_element(data_.begin(), data_.end()) != 0)
    return false;
  if (*std::max_element(data_.begin(), data_.end()) != size() - 1)
    return false;
  return true;
}

void Order::sanitise() {
  std::multimap<Eigen::Index, Eigen::Index> sorter;
  for (Eigen::Index axis = 0; axis != size(); ++axis)
    sorter.insert({data_[axis], axis});
  data_.clear();
  for (const auto &item : sorter)
    data_.push_back(item.second);
  assert(is_sanitised());
}

bool Order::valid() const {
  if (empty())
    return false;
  if (*std::min_element(data_.begin(), data_.end()) < 0)
    return false;
  if (std::set<Eigen::Index>(data_.begin(), data_.end()).size() != size())
    return false;
  return true;
}

// Axes::Subset Order::block(const Eigen::Index from_axis, const Eigen::Index to_axis) const {
//   assert(from_axis >= 0);
//   assert(from_axis < to_axis);
//   return Axes::Subset(data_.begin() + from_axis, data_.begin() + std::min(size(), to_axis));
// }

Axes::Subset Order::head(const Eigen::Index num_axes) const {
  assert(num_axes >= 0);
  assert(num_axes <= size());
  return Axes::Subset(data_.begin(), data_.begin() + num_axes);
}

Permutation Order::permutation() const { return Permutation(*this); }

Axes::Subset Order::subset(const Axes::Subset &axes) const {
  Axes::Subset result;
  for (const auto axis : data_) {
    if (std::find(axes.begin(), axes.end(), axis) != axes.end())
      result.push_back(axis);
  }
  return result;
}

Axes::Subset Order::subset(const Eigen::Index from_axis, const Eigen::Index to_axis) const {
  assert(from_axis >= 0);
  assert(to_axis == -1 || to_axis > from_axis);
  Axes::Subset result;
  for (const auto axis : data_) {
    if (axis >= from_axis && (to_axis == -1 || axis < to_axis))
      result.push_back(axis);
  }
  return result;
}

Axes::Subset Order::tail(const Eigen::Index num_axes) const {
  assert(num_axes >= 0);
  assert(num_axes <= size());
  return Axes::Subset(ListType(data_.begin() + (size() - num_axes), data_.end()));
}

Order Order::canonical(const Eigen::Index num_axes) {
  ListType order(num_axes);
  for (Eigen::Index axis = 0; axis != num_axes; ++axis)
    order[axis] = axis;
  Order result(order);
  assert(result.is_sanitised());
  return result;
}

Permutation::Permutation(const ListType &permutation) : Base(permutation) {
  // TODO From user input?
  // May include "-1" values indicating insertion of an axis?
}

Permutation::Permutation(const Order &order)
    : Base(1 + *std::max_element(static_cast<ListType>(order).begin(), static_cast<ListType>(order).end()),
           Permutation::invalid) {
  for (Eigen::Index index = 0; index != order.size(); ++index)
    data_[order[index]] = index;
  assert(!is_degenerate());
}

// TODO Note that the origin function took from_axis and to_axis arguments;
//   it may however be preferable to force an initial extraction of those axes
//   prior to establishing their order,
//   rather than doing it within the ordering function?
// TODO Confirm that this preserves duplicate symbolic strides
//   (irrespective of sign) as duplicates in order?
// TODO Think that zeroes need to be detected here,
//   and assigned a value one larger than the maximal absolute non-zero symbolic
Permutation::Permutation(const Symbolic &symbolic) : Base(symbolic.size(), Permutation::invalid) {
  assert(symbolic.valid());
  // TODO Update type following CRTP transition
  std::multimap<Eigen::Index, Eigen::Index> sorter;
  for (Eigen::Index axis = 0; axis != size(); ++axis)
    sorter.insert({MR::abs(symbolic[axis]), axis});
  // TODO Preserve ties in the conversion?
  Eigen::Index perm_index = 0;
  Eigen::Index last_symbolic = Symbolic::invalid;
  for (const auto &item : sorter) {
    if (item.first != last_symbolic) {
      if (last_symbolic != Symbolic::invalid)
        ++perm_index;
      last_symbolic = item.first;
    }
    data_[item.second] = perm_index;
  }
  DEBUG("From symbolic strides [" + str(symbolic) + "]," + " constructed permutation [" + str(*this) + "]");
}

Permutation::Permutation(const Actual &actual) : Permutation(Symbolic(actual)) {}

bool Permutation::is_canonical() const {
  for (Eigen::Index axis = 0; axis != size(); ++axis) {
    if (data_[axis] != axis)
      return false;
  }
  return true;
}

bool Permutation::is_degenerate() const {
  std::set<Eigen::Index> valid_indices;
  for (Eigen::Index axis = 0; axis != size(); ++axis) {
    if (data_[axis] != invalid) {
      if (valid_indices.find(data_[axis]) != valid_indices.end())
        return true;
      valid_indices.insert(data_[axis]);
    }
  }
  return false;
}

bool Permutation::is_sanitised() const {
  if (!valid())
    return false;
  // First axis must be indexed as zero
  if (*std::min_element(data_.begin(), data_.end()) != 0)
    return false;
  // Should not be any values beyond the number of dimensions
  if (*std::max_element(data_.begin(), data_.end()) != data_.size() - 1)
    return false;
  // No duplicates
  if (is_degenerate())
    return false;
  return true;
}

void Permutation::sanitise() {
  std::multimap<Eigen::Index, Eigen::Index> sorter;
  for (Eigen::Index axis = 0; axis != size(); ++axis)
    sorter.insert({data_[axis], axis});
  Eigen::Index axis = 0;
  std::ostringstream oss;
  oss << "Sanitised permutation [" << *this << "]";
  for (const auto &sorted : sorter)
    data_[sorted.second] = axis++;
  oss << " as [" << *this << "]";
  DEBUG(oss.str());
  assert(is_sanitised());
}

bool Permutation::valid() const {
  // If empty, not a valid permutation
  if (empty())
    return false;
  // No negative axis indices
  if (*std::min_element(data_.begin(), data_.end()) < 0)
    return false;
  return true;
}

Permutation Permutation::head(const Eigen::Index num_axes) const {
  assert(num_axes >= 0);
  assert(num_axes <= size());
  Permutation result(*this);
  result.resize(num_axes);
  return result;
}

Order Permutation::order() const { return Order(*this); }

void Permutation::resize(const Eigen::Index num_axes) { data_.resize(num_axes, Permutation::invalid); }

Permutation Permutation::sanitised() const {
  Permutation result(*this);
  result.sanitise();
  return result;
}

Permutation Permutation::axis_range(const Eigen::Index from, const Eigen::Index to) {
  assert(to > from);
  ListType permutation(to - from, Permutation::invalid);
  for (Eigen::Index i = 0; i != permutation.size(); ++i)
    permutation[i] = from + i;
  const Permutation result(permutation);
  assert(result.is_sanitised());
  return result;
}

Permutation Permutation::canonical(const Eigen::Index num_axes) {
  ListType permutation(num_axes);
  for (Eigen::Index axis = 0; axis != num_axes; ++axis)
    permutation[axis] = axis;
  const Permutation result(permutation);
  assert(result.is_sanitised());
  return result;
}

Permutation Permutation::contiguous_along_axis(const Eigen::Index num_axes, const Eigen::Index axis) {
  assert(axis >= 0);
  assert(axis < num_axes);
  ListType permutation(num_axes, 1);
  permutation[axis] = 0;
  return Permutation(permutation);
}

const Permutation Permutation::volume_contiguous({1, 1, 1, 0});

Symbolic::Symbolic(const ListType &in) : Base(in) { sanitise(); }

Symbolic::Symbolic(const Actual &actual) : Base(actual.size(), Symbolic::invalid) {
  // TODO Would prefer to be able to restore this assertion
  // ... But this might be getting called based on an Actual initialised from a template input,
  //   in which case there's no guarantee that those data are in fact valid Actual...
  // assert(actual.valid());
  std::multimap<Eigen::Index, Eigen::Index> sorter;
  // Where actual strides are tied,
  //   order axes from last to first
  // TODO This should be an arbitrary preference;
  //   code shouldn't break based on this change... (awaiting to see if it fixes current issue)
  // TODO I think that elements that are zero here
  //   may need to be put at the end of the list rather than the start?
  for (Eigen::Index axis = size() - 1; axis >= 0; --axis)
    sorter.insert({actual[axis] == 0 ? std::numeric_limits<Eigen::Index>::max() : MR::abs(actual[axis]), axis});
  Eigen::Index index = 1;
  std::ostringstream oss;
  oss << "From actual strides [" << actual << "],";
  for (const auto &item : sorter)
    data_[item.second] = index++ * (actual[item.second] < 0 ? -1 : 1);
  oss << " constructed symbolic strides [" << *this << "]";
  DEBUG(oss.str());
  assert(is_sanitised());
}

bool Symbolic::is_canonical() const {
  for (Eigen::Index axis = 0; axis != size(); ++axis) {
    if (data_[axis] != axis + 1)
      return false;
  }
  return true;
}

bool Symbolic::is_degenerate() const {
  std::set<Eigen::Index> nonzero_magnitudes;
  for (Eigen::Index axis = 0; axis != size(); ++axis) {
    if (data_[axis] != invalid) {
      if (nonzero_magnitudes.find(MR::abs(data_[axis])) != nonzero_magnitudes.end())
        return true;
      nonzero_magnitudes.insert(MR::abs(data_[axis]));
    }
  }
  return false;
}

bool Symbolic::is_sanitised() const {
  if (!valid())
    return false;
  if (is_degenerate())
    return false;
  std::set<Eigen::Index> magnitudes;
  for (Eigen::Index axis = 0; axis != size(); ++axis)
    magnitudes.insert(MR::abs(data_[axis]));
  if (*std::min_element(magnitudes.begin(), magnitudes.end()) != 1)
    return false;
  if (*std::max_element(magnitudes.begin(), magnitudes.end()) != size())
    return false;
  return true;
}

bool Symbolic::valid() const {
  if (empty())
    return false;
  if (std::any_of(data_.begin(), data_.end(), [](const Eigen::Index value) { return value == invalid; }))
    return false;
  return true;
}

Symbolic Symbolic::block(const Eigen::Index from_axis, const Eigen::Index to_axis) const {
  assert(from_axis >= 0);
  assert(from_axis < size());
  if (to_axis == -1)
    return Symbolic(ListType(data_.begin() + from_axis, data_.end()));
  assert(from_axis < to_axis);
  return Symbolic(ListType(data_.begin() + from_axis, data_.begin() + std::min(size(), to_axis)));
}

void Symbolic::conform(const Symbolic &in) {
  // TODO Technically might be possible to relax
  assert(in.size() == size());
  class Data {
  public:
    Data(const Eigen::Index axis_index, const Eigen::Index current_stride, const Eigen::Index requested_stride)
        : axis_index(axis_index), current_stride(current_stride), requested_stride(requested_stride) {}
    Eigen::Index axis_index;
    Eigen::Index current_stride;   // TODO Update to reflect CRTP
    Eigen::Index requested_stride; // TODO Update to reflect CRTP
    bool operator<(const Data &in) const {
      if (requested_stride != 0 && in.requested_stride == 0)
        return true;
      if (requested_stride == 0 && in.requested_stride != 0)
        return false;
      if (MR::abs(requested_stride) != MR::abs(in.requested_stride))
        return MR::abs(requested_stride) < MR::abs(in.requested_stride);
      if (MR::abs(current_stride) != MR::abs(in.current_stride))
        return MR::abs(current_stride) < MR::abs(in.current_stride);
      assert(axis_index != in.axis_index);
      return axis_index < in.axis_index;
    }
  };
  std::vector<Data> sorter;
  for (Eigen::Index axis = 0; axis != size(); ++axis)
    sorter.emplace_back(Data(axis, data_[axis], in[axis]));
  std::sort(sorter.begin(), sorter.end());
  std::ostringstream oss;
  oss << "Initial symbolic strides [" << *this << "],"
      << " conformed to symbolic stride request [" << in << "]";
  for (Eigen::Index index = 0; index != size(); ++index)
    data_[sorter[index].axis_index] = (index + 1) * (data_[sorter[index].axis_index] < 0 ? -1 : 1);
  oss << " is [" << *this << "]";
  DEBUG(oss.str());
  assert(is_sanitised());
}

Symbolic Symbolic::conformed(const Symbolic &in) const {
  Symbolic result(*this);
  result.conform(in);
  return result;
}

Order Symbolic::order() const { return permutation().order(); }

//! remove duplicate and invalid strides.
/*! sanitise the strides of HeaderType \a header by identifying invalid (i.e.
 * zero) or duplicate (absolute) strides, and assigning to each a
 * suitable value. The value chosen for each sanitised stride is the
 * lowest number greater than any of the currently valid strides. */
// TODO The origin of this function included checks for image axes of size 1;
//   perhaps rather than being integrated here,
//   it would be preferable to instead define a separate explicit function
//   that shuffles unity-size axes to the (smallest or largest) strides
// void Symbolic::sanitise() {
//   // remove duplicates
//   for (size_t i = 0; i < size() - 1; ++i) {
//     if (data_[i] == ssize_t(0))
//       continue;
//     for (size_t j = i + 1; j < size(); ++j) {
//       if (!data_[j])
//         continue;
//       if (MR::abs(data_[i]) == MR::abs(data_[j]))
//         data_[j] = 0;
//     }
//   }

//   ssize_t max = 0;
//   for (size_t i = 0; i < size(); ++i)
//     max = std::max(max, MR::abs(data_[i]));

//   for (size_t i = 0; i < size(); ++i) {
//     if (data_[i] == 0)
//       data_[i] = ++max;
//   }
// }

Permutation Symbolic::permutation() const { return Permutation(*this); }

void Symbolic::resize(const Eigen::Index num_axes) { data_.resize(num_axes, Symbolic::invalid); }

Symbolic Symbolic::resized(const Eigen::Index num_axes) const {
  Symbolic result(*this);
  result.resize(num_axes);
  return result;
}

void Symbolic::sanitise() {
  // Key is absolute value of current stride:
  //   - If currently set to zero,
  //     then axis will be promoted to as contiguous as possible
  //     (contingent on earlier axes being the same)
  //   - If currently non-zero,
  //     they will be ordered
  std::multimap<Eigen::Index, Eigen::Index> sorter;
  for (Eigen::Index axis = 0; axis != size(); ++axis)
    sorter.insert({MR::abs(data_[axis]), axis});
  ListType result(size());
  Eigen::Index axis = 1;
  // TODO Double-check
  for (const auto &item : sorter)
    result[item.second] = axis++ * (data_[item.second] < 0 ? -1 : 1);
  if (result != data_) {
    std::ostringstream oss;
    oss << "Symbolic strides [" << *this << "]";
    data_ = result;
    oss << " sanitised as [" << *this << "]";
    DEBUG(oss.str());
  }
  assert(is_sanitised());
}

// TODO By the revised definitions,
//   this is no longer reordering the symbolic strides based on a specified "order";
//   rather, it is specifying a desired permutation

// TODO This may not be doing as intended
// What is actually needed is:
// - For each unique permutation index in incrementing order:
//   - Sort axes within that group by existing symbolic strides
//   - Set symbolic strides
void Symbolic::reorder(const Permutation &permutation) {

  // TODO Revise to
  assert(permutation.size() == size());

  class Data {
  public:
    Data(const Eigen::Index axis_index, const Eigen::Index current_symbolic, const Eigen::Index permutation_group)
        : axis_index(axis_index), current_symbolic(current_symbolic), permutation_group(permutation_group) {}
    Eigen::Index axis_index;
    Eigen::Index current_symbolic; // TODO Update type for CRTP
    Eigen::Index permutation_group;
    bool operator<(const Data &that) const {
      if (permutation_group != that.permutation_group)
        return permutation_group < that.permutation_group;
      if (MR::abs(current_symbolic) != MR::abs(that.current_symbolic))
        return MR::abs(current_symbolic) < MR::abs(that.current_symbolic);
      assert(axis_index != that.axis_index);
      return axis_index < that.axis_index;
    }
  };
  std::vector<Data> sorted;
  for (Eigen::Index axis = 0; axis != size(); ++axis)
    sorted.emplace_back(Data(axis, data_[axis], permutation[axis]));
  std::sort(sorted.begin(), sorted.end());
  std::ostringstream oss;
  oss << "Symbolic strides [" << *this << "]"
      << " following application of permutation [" << permutation << "]";
  for (Eigen::Index order_indexed = 0; order_indexed != size(); ++order_indexed)
    data_[sorted[order_indexed].axis_index] =
        (order_indexed + 1) * (sorted[order_indexed].current_symbolic < 0 ? -1 : 1);
  oss << " are [" << *this << "]";
  DEBUG(oss.str());
  assert(valid());
}

// void Symbolic::reorder(const Permutation &permutation) {
//   assert(permutation.size() == size());
//   // TODO Based on a requested reording of axes,
//   //   derive a new set of symbolic strides
//   // TODO Ensure tests are present that if this is provided with a Stride::Order
//   //   that was derived from itself,
//   //   there is no effect on the symbolic strides
//   // TODO Should be possible to provide this with something like Order::contiguous_along_axis(),
//   //   which relies on specifying zeroes to say "relative order of these axes should remain unchanged"
//   //   -> NO IT DOESN'T!
//   // Does this need to be a duplicate of "get_nearest_match()"?
//   // -> Not convinced that the logic there is correct anyways...
//   // -> This might take as input a (potentially invalid) set of symbolic strides,
//   //    and include zero values specified by the user
//   ListType result(size(), Symbolic::invalid);
//   // Map requested permutation to input axis;
//   //   possible that there may be entries in the input requested permutation that are equal but non-zero,
//   //   in which case they will just be sorted by axis index
//   // TODO Change type once CRTP'd
//   std::multimap<Eigen::Index, Eigen::Index> sort_nonzero;
//   for (Eigen::Index axis = 0; axis != size(); ++axis) {
//     // Permutation doesn't need to be completely valid,
//     //   but it certainly can't contain negative values
//     assert(permutation[axis] >= 0);
//     if (permutation[axis] != 0)
//       sort_nonzero.insert({data_[axis], axis});
//   }
//   Eigen::Index nonzero_index = 0;
//   for (const auto &nonzero : sort_nonzero)
//     result[nonzero.second] = nonzero_index++ * (data_[nonzero.second] < 0 ? -1 : 1);
//   // Now for the axes that were specified as zero in the input requested permutation:
//   //   they should be preserved in relative order to one another,
//   //   and preserve their original sign,
//   //   but *after* all of the axes that were non-zero in the input requested order
//   for (Eigen::Index axis = 0; axis != size(); ++axis) {
//     if (permutation[axis] == 0)
//       result[axis] = (MR::abs(data_[axis]) + sort_nonzero.size()) * (data_[axis] < 0 ? -1 : 1);
//   }
//   data_ = result;
//   assert(valid());
// }

Symbolic Symbolic::reordered(const Permutation &order) const {
  Symbolic result(*this);
  result.reorder(order);
  return result;
}

Symbolic Symbolic::sanitised() const {
  Symbolic result(*this);
  result.sanitise();
  return result;
}

// Duplicate of previous MR::Header::<anonymous>::check_strides_match()
// TODO Confirm whether this is suitable as a generic function
bool Symbolic::operator==(const Symbolic &that) const {
  Eigen::Index n = 0;
  for (; n < std::min(size(), that.size()); ++n)
    if ((*this)[n] != that[n])
      return false;
  for (Eigen::Index i = n; i < size(); ++i)
    if ((*this)[i] > 1)
      return false;
  for (Eigen::Index i = n; i < that.size(); ++i)
    if (that[i] > 1)
      return false;
  return true;
}

bool Symbolic::operator!=(const Symbolic &that) const { return !operator==(that); }

Symbolic Symbolic::canonical(const Eigen::Index axes) {
  ListType result(axes);
  for (Eigen::Index axis = 0; axis != axes; ++axis)
    result[axis] = axis + 1;
  const Symbolic symbolic(result);
  assert(symbolic.valid());
  return symbolic;
}

// TODO Consider moving function
// Maybe should be a constructor for Actual that takes as input Symbolic and sizes
Actual Symbolic::actualise_from_sizes(const std::vector<Eigen::Index> &sizes) const {
  assert(is_sanitised());
  assert(size() == sizes.size());
  const Order x(order());
  ListType actual(size(), Actual::invalid);
  Eigen::Index skip = 1;
  for (Eigen::Index i = 0; i < size(); ++i) {
    actual[x[i]] = data_[x[i]] < 0 ? -skip : skip;
    skip *= sizes[x[i]];
  }
  const Actual result(actual);
  DEBUG("Symbolic strides [" + str(*this) + "]" + //
        " actualised using sizes " + str(sizes) + //
        " as [" + str(result) + "]");             //
  return result;
}

Actual::Actual(const ListType data) : Base(data) {}

bool Actual::is_canonical() const {
  Eigen::Index last = 0;
  for (Eigen::Index axis = 0; axis != size(); ++axis) {
    // Permissive of equality:
    //   axis of unity size will result in two sequential axes having equal actual strides
    if (data_[axis] < last)
      return false;
    last = data_[axis];
  }
  return true;
}

bool Actual::valid() const {
  if (empty())
    return false;
  const std::set<Eigen::Index> data_as_set(data_.begin(), data_.end());
  // Duplicates present -> invalid
  if (data_as_set.size() != data_.size())
    return false;
  // At least one stride of zero present -> invalid
  if (data_as_set.find(Eigen::Index(0)) != data_as_set.end())
    return false;
  return true;
}

Order Actual::order() const { return permutation().order(); }

Permutation Actual::permutation() const { return symbolic().permutation(); }

Symbolic Actual::symbolic() const { return Symbolic(*this); }

std::optional<Symbolic> __from_command_line(const Symbolic &current) {

  auto opt = App::get_options("strides");
  if (opt.empty())
    return std::nullopt;

  Symbolic user_specification;
  try {
    user_specification = Symbolic(Header::open(opt[0][0]));
  } catch (Exception &E) {
    E.display(3);
    try {
      user_specification = Symbolic(parse_ints<Eigen::Index>(opt[0][0]));
    } catch (Exception &E) {
      E.display(3);
      throw Exception("argument \"" + static_cast<std::string>(opt[0][0]) + "\"" +
                      " to option \"-strides\" is not a list of strides or an image");
    }
    if (user_specification.size() > current.size()) {
      INFO("template image provided to -strides option has more axes than the image being modified;"
           " ignoring excess strides");
      user_specification.resize(current.size());
    }
  }

  return __from_command_line(user_specification, current);
}

std::optional<Symbolic> __from_command_line(const Symbolic &user_specification, const Symbolic &current) {

  if (user_specification.size() > current.size())
    throw Exception("too many axes supplied to -strides option");

  for (Eigen::Index axis = 0; axis != user_specification.size(); ++axis) {
    if (MR::abs(const_cast<const Symbolic *const>(&user_specification)->operator[](axis)) > current.size())
      throw Exception(std::string("strides specified exceed image dimensions:") + //
                      " got " + str(user_specification) + "," +                   //
                      " but image has " + str(current.size()) + " axes");         //
  }

  if (user_specification.is_degenerate())
    throw Exception("duplicate entries provided to \"-strides\" option: " + str(user_specification));

  // TODO From here need to figure out preceding logic
  // In particular do we need to consider the prospect that what the user provides at the command-line
  //   could technically be *either* symbolic strides *or* an order?
  // Think that rather than replicating the steps of the code as-is,
  //   derive appropriate logic from scratch:
  // 1. For all non-zero entries in the user specification,
  //    place in sequential order,
  //    preserving sign
  // 2. For all zero entries in the user specification,
  //    place in sequential order atop step 1,
  //    preserving sign
  ListType new_symbolic(current.size(), Symbolic::invalid);
  std::multimap<Eigen::Index, Eigen::Index> order_userspec;
  std::multimap<Eigen::Index, Eigen::Index> order_current;
  for (Eigen::Index axis = 0; axis != current.size(); ++axis) {
    if (const_cast<const Symbolic *const>(&user_specification)->operator[](axis) == 0)
      order_current.insert({MR::abs(current[axis]), axis});
    else
      order_userspec.insert({MR::abs(const_cast<const Symbolic *const>(&user_specification)->operator[](axis)), axis});
  }
  Eigen::Index out_absstride = 1;
  for (const auto &item : order_userspec)
    new_symbolic[item.second] = out_absstride++ * (user_specification[item.second] < 0 ? -1 : 1);
  for (const auto &item : order_current)
    new_symbolic[item.second] = out_absstride++ * (current[item.second] < 0 ? -1 : 1);
  const Symbolic result(new_symbolic);
  assert(result.valid());
  DEBUG("Current symbolic strides [" + str(current) + "]" + " conformed to user specification [" +
        str(user_specification) + "]" + " as [" + str(result) + "]");
  return {result};
}

} // namespace MR::Stride
