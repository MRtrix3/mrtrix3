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

Order::Order(const Permutation &permutation) : Base(permutation.size(), Order::invalid) {
  assert(permutation.valid());
  for (ArrayIndex axis = 0; axis != permutation.size(); ++axis)
    data_[permutation[axis]] = axis;
}

Order::Order(const Symbolic &symbolic) : Order(Permutation(symbolic)) {}

Order::Order(const std::vector<Order::value_type> &data) : Base(data) {}

Order::operator Axes::Subset() const { return Axes::Subset(data_.begin(), data_.end()); }

bool Order::is_canonical() const {
  for (ArrayIndex axis = 0; axis != size(); ++axis) {
    if (data_[axis] != axis)
      return false;
  }
  return true;
}

bool Order::is_sanitised() const { return (valid()); }

void Order::sanitise() { throw Exception("Cannot apply sanitise() operation to Stride::Order"); }

bool Order::valid() const {
  if (empty())
    return false;
  if (*std::min_element(data_.begin(), data_.end()) != 0)
    return false;
  if (*std::max_element(data_.begin(), data_.end()) != size() - 1)
    return false;
  if (std::set<ArrayIndex>(data_.begin(), data_.end()).size() != size())
    return false;
  return true;
}

Axes::Subset Order::head(const size_t num_axes) const {
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

Axes::Subset Order::subset(const ArrayIndex from_axis, const ArrayIndex to_axis) const {
  assert(from_axis >= 0);
  if (to_axis != Order::invalid) {
    assert(to_axis > from_axis);
    assert(to_axis <= size());
  }
  Axes::Subset result;
  for (const auto axis : data_) {
    if (axis >= from_axis && (to_axis == -1 || axis < to_axis))
      result.push_back(axis);
  }
  return result;
}

Axes::Subset Order::tail(const size_t num_axes) const {
  assert(num_axes >= 0);
  assert(num_axes <= size());
  return Axes::Subset(std::vector<value_type>(data_.begin() + (size() - num_axes), data_.end()));
}

Order Order::canonical(const size_t num_axes) {
  vector_type order(num_axes);
  for (ArrayIndex axis = 0; axis != static_cast<ArrayIndex>(num_axes); ++axis)
    order[axis] = axis;
  Order result(order);
  assert(result.is_sanitised());
  return result;
}

Permutation::Permutation(const Permutation::vector_type &permutation) : Base(permutation) {}

Permutation::Permutation(const Order &order) {
  for (ArrayIndex index = 0; index != static_cast<ArrayIndex>(order.size()); ++index) {
    if (order[index] >= size())
      data_.resize(order[index] + 1, Permutation::invalid);
    data_[order[index]] = index;
  }
  assert(valid());
  assert(!is_degenerate());
}

Permutation::Permutation(const Symbolic &symbolic) : Base(symbolic.size(), Permutation::invalid) {
  std::multimap<Symbolic::value_type, ArrayIndex> sorter;
  for (ArrayIndex axis = 0; axis != size(); ++axis) {
    if (symbolic[axis] != Symbolic::invalid)
      sorter.insert({MR::abs(symbolic[axis]), axis});
  }
  ArrayIndex perm_index = 0;
  Symbolic::value_type last_symbolic = Symbolic::invalid;
  for (const auto &item : sorter) {
    if (item.first != last_symbolic) {
      if (last_symbolic != Symbolic::invalid)
        ++perm_index;
      last_symbolic = item.first;
    }
    data_[item.second] = perm_index;
  }
  ++perm_index;
  for (ArrayIndex axis = 0; axis != size(); ++axis) {
    if (symbolic[axis] == Symbolic::invalid)
      data_[axis] = perm_index;
  }
  DEBUG("From symbolic strides [" + str(symbolic) + "]," + " constructed permutation [" + str(*this) + "]");
}

Permutation::Permutation(const Actual &actual) : Permutation(Symbolic(actual)) {}

bool Permutation::is_canonical() const {
  for (ArrayIndex axis = 0; axis != size(); ++axis) {
    if (data_[axis] != axis)
      return false;
  }
  return true;
}

bool Permutation::is_degenerate() const {
  std::set<ArrayIndex> valid_indices;
  for (ArrayIndex axis = 0; axis != size(); ++axis) {
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
  std::multimap<value_type, ArrayIndex> sorter;
  for (ArrayIndex index = 0; index != size(); ++index) {
    if (data_[index] == Permutation::invalid)
      throw Exception("Cannot sanitise a stride permutation that contains invalid values");
    sorter.insert({data_[index], index});
  }
  std::ostringstream oss;
  oss << "Sanitised permutation [" << *this << "]";
  value_type axis = 0;
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

Permutation Permutation::head(const size_t num_axes) const {
  assert(num_axes <= size());
  Permutation::vector_type result(*this);
  result.resize(num_axes);
  return Permutation(result);
}

Order Permutation::order() const { return Order(*this); }

Permutation Permutation::sanitised() const {
  Permutation result(*this);
  result.sanitise();
  return result;
}

Symbolic Permutation::symbolic() const { return Symbolic(*this); }

Permutation Permutation::axis_range(const ArrayIndex from, const ArrayIndex to) {
  assert(to > from);
  vector_type permutation(to - from, Permutation::invalid);
  for (ArrayIndex i = 0; i != permutation.size(); ++i)
    permutation[i] = from + i;
  const Permutation result(permutation);
  assert(result.valid());
  if (from == 0) {
    assert(result.is_sanitised());
  }
  return result;
}

Permutation Permutation::canonical(const size_t num_axes) {
  vector_type permutation(num_axes);
  for (ArrayIndex axis = 0; axis != num_axes; ++axis)
    permutation[axis] = axis;
  const Permutation result(permutation);
  assert(result.is_sanitised());
  return result;
}

Permutation Permutation::contiguous_along_axis(const size_t num_axes, const ArrayIndex axis) {
  assert(axis >= 0);
  assert(axis < num_axes);
  vector_type permutation(num_axes, 1);
  permutation[axis] = 0;
  return Permutation(permutation);
}

Permutation Permutation::contiguous_along_spatial_axes(const size_t num_axes) {
  Permutation::vector_type permutation(num_axes, 0);
  for (ArrayIndex nonspatial_axis = 3; nonspatial_axis < static_cast<ArrayIndex>(num_axes); ++nonspatial_axis)
    permutation[nonspatial_axis] = 1;
  return Permutation(permutation);
}

const Permutation Permutation::volume_contiguous({1, 1, 1, 0});

Symbolic::Symbolic(const Symbolic::vector_type &in) : Base(in) {}

Symbolic::Symbolic(const Permutation &in) {
  resize(in.size());
  for (ArrayIndex axis = 0; axis != size(); ++axis)
    data_[axis] = static_cast<Symbolic::value_type>(in[axis]) + 1;
}

Symbolic::Symbolic(const Header &header) : Symbolic(header.strides()) {}

Symbolic::Symbolic(const Actual &actual) : Base(actual.size(), Symbolic::invalid) {
  std::multimap<Actual::value_type, ArrayIndex> sorter;
  for (ArrayIndex axis = size() - 1; axis >= 0; --axis)
    sorter.insert(
        {actual[axis] == Actual::invalid ? std::numeric_limits<Actual::value_type>::max() : MR::abs(actual[axis]),
         axis});
  value_type index = 1;
  value_type counter = 0;
  Actual::value_type prev_actual = Actual::invalid;
  std::ostringstream oss;
  oss << "From actual strides [" << actual << "],";
  for (const auto &item : sorter) {
    if (item.first == prev_actual) {
      data_[item.second] = index * (actual[item.second] < 0 ? -1 : 1);
      ++counter;
    } else {
      data_[item.second] = ++counter * (actual[item.second] < 0 ? -1 : 1);
      prev_actual = item.first;
      index = counter;
    }
  }
  oss << " constructed symbolic strides [" << *this << "]";
  DEBUG(oss.str());
  // If Actual contains ties, then Symbolic will no longer be sanitised
  // assert(is_sanitised());
  assert(valid());
}

bool Symbolic::is_canonical() const {
  for (ArrayIndex axis = 0; axis != size(); ++axis) {
    if (data_[axis] != axis + 1)
      return false;
  }
  return true;
}

bool Symbolic::is_degenerate() const {
  std::set<ArrayIndex> nonzero_magnitudes;
  for (ArrayIndex axis = 0; axis != size(); ++axis) {
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
  std::set<ArrayIndex> magnitudes;
  for (ArrayIndex axis = 0; axis != size(); ++axis)
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
  if (std::any_of(data_.begin(), data_.end(), [](const value_type value) { return value == invalid; }))
    return false;
  return true;
}

Symbolic Symbolic::block(const ArrayIndex from_axis, const ArrayIndex to_axis) const {
  assert(from_axis >= 0);
  assert(from_axis < size());
  if (to_axis == -1)
    return Symbolic(vector_type(data_.begin() + from_axis, data_.end()));
  assert(from_axis < to_axis);
  assert(to_axis < size());
  return Symbolic(
      vector_type(data_.begin() + from_axis, data_.begin() + std::min(static_cast<ArrayIndex>(size()), to_axis)));
}

void Symbolic::conform(const Symbolic &in) {
  assert(in.size() == size());
  const Stride::Permutation perm_in(in);
  const Symbolic reordered_this = reordered(perm_in);
  if (reordered_this == *this)
    return;
  // "First, non-zero strides in desired are used as-is"
  vector_type result(in);
  // "then the remaining strides are taken from current where specified and used with higher values"
  value_type index_out = *std::max_element(result.begin(), result.end());
  for (ArrayIndex axis = 0; axis != size(); ++axis) {
    if ((*this)[axis] != Symbolic::invalid && in[axis] == Symbolic::invalid)
      result[axis] = ++index_out * ((*this)[axis] < 0 ? -1 : 1);
  }
  // "followed by those strides not specified in either."
  for (ArrayIndex axis = 0; axis != size(); ++axis) {
    if ((*this)[axis] == Symbolic::invalid && in[axis] == Symbolic::invalid)
      result[axis] = ++index_out;
  }
  data_ = result;
}

Symbolic Symbolic::conformed(const Symbolic &in) const {
  Symbolic result(*this);
  result.conform(in);
  return result;
}

void Symbolic::flip(const ArrayIndex axis) {
  assert(axis >= 0 && axis < size());
  data_[axis] *= -1;
}

Symbolic Symbolic::head(const ArrayIndex num_axes) const {
  assert(num_axes <= size());
  return Symbolic(vector_type(data_.begin(), data_.begin() + num_axes));
}

void Symbolic::invalidate(const ArrayIndex axis) {
  assert(axis >= 0 && axis < size());
  data_[axis] = Symbolic::invalid;
}

Order Symbolic::order() const { return permutation().order(); }

Permutation Symbolic::permutation() const { return Permutation(*this); }

void Symbolic::resize(const size_t num_axes) { data_.resize(num_axes, Symbolic::invalid); }

Symbolic Symbolic::resized(const size_t num_axes) const {
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
  std::multimap<value_type, ArrayIndex> sorter;
  for (ArrayIndex axis = 0; axis != size(); ++axis) {
    if (data_[axis] != Symbolic::invalid)
      sorter.insert({MR::abs(data_[axis]), axis});
  }
  vector_type result(size());
  ArrayIndex counter = 0;
  for (const auto &item : sorter)
    result[item.second] = ++counter * (data_[item.second] < 0 ? -1 : 1);
  for (ArrayIndex axis = 0; axis != size(); ++axis) {
    if (data_[axis] == Symbolic::invalid)
      result[axis] = ++counter;
  }
  if (result != data_) {
    std::ostringstream oss;
    oss << "Symbolic strides [" << *this << "]";
    data_ = result;
    oss << " sanitised as [" << *this << "]";
    DEBUG(oss.str());
  }
  assert(is_sanitised());
}

void Symbolic::reorder(const Permutation &permutation) {

  assert(permutation.size() == size());

  class Data {
  public:
    Data(const ArrayIndex axis_index, const value_type current_symbolic, const ArrayIndex permutation_group)
        : axis_index(axis_index), current_symbolic(current_symbolic), permutation_group(permutation_group) {}
    ArrayIndex axis_index;
    value_type current_symbolic;
    ArrayIndex permutation_group;
    bool operator<(const Data &that) const {
      if (permutation_group != that.permutation_group)
        return permutation_group < that.permutation_group;
      if (MR::abs(current_symbolic) != MR::abs(that.current_symbolic)) {
        if (that.current_symbolic == Symbolic::invalid)
          return true;
        if (current_symbolic == Symbolic::invalid)
          return false;
        return MR::abs(current_symbolic) < MR::abs(that.current_symbolic);
      }
      assert(axis_index != that.axis_index);
      return axis_index < that.axis_index;
    }
  };
  std::vector<Data> sorted;
  for (ArrayIndex axis = 0; axis != size(); ++axis)
    sorted.emplace_back(Data(axis, data_[axis], permutation[axis]));
  std::sort(sorted.begin(), sorted.end());
  std::ostringstream oss;
  oss << "Symbolic strides [" << *this << "]"
      << " following application of permutation [" << permutation << "]";
  for (ArrayIndex order_indexed = 0; order_indexed != size(); ++order_indexed)
    data_[sorted[order_indexed].axis_index] =
        (order_indexed + 1) * (sorted[order_indexed].current_symbolic < 0 ? -1 : 1);
  oss << " are [" << *this << "]";
  DEBUG(oss.str());
  assert(valid());
}

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

Symbolic Symbolic::canonical(const size_t num_axes) {
  vector_type result(num_axes);
  for (ArrayIndex axis = 0; axis != static_cast<ArrayIndex>(num_axes); ++axis)
    result[axis] = axis + 1;
  const Symbolic symbolic(result);
  assert(symbolic.valid());
  return symbolic;
}

Actual::Actual(const Actual::vector_type &data) : Base(data) {}

Actual::Actual(const Symbolic &symbolic, const std::vector<VoxelIndex> &sizes)
    : Base<value_type>(symbolic.size(), invalid) {
  assert(symbolic.is_sanitised());
  assert(symbolic.size() == sizes.size());
  const Order x(symbolic.order());
  value_type skip = 1;
  for (ArrayIndex i = 0; i < size(); ++i) {
    data_[x[i]] = symbolic[x[i]] < 0 ? -skip : skip;
    skip *= sizes[x[i]];
  }
  DEBUG("Symbolic strides [" + str(symbolic) + "]" + //
        " actualised using sizes " + str(sizes) +    //
        " as [" + str(*this) + "]");                 //
}

bool Actual::is_canonical() const {
  if (!valid())
    return false;
  value_type last = 0;
  for (ArrayIndex axis = 0; axis != size(); ++axis) {
    // Permissive of equality:
    //   axis of unity size will result in two sequential axes having equal actual strides
    if (data_[axis] < last)
      return false;
    last = data_[axis];
  }
  return true;
}

// It is possible for an image with an axis of unity size
//   to have two axes with actual strides of equal absolute value;
//   this is however not actually a problem
bool Actual::is_degenerate() const {
  std::set<value_type> set_abs;
  for (auto value : data_) {
    const value_type abs = MR::abs(value);
    if (set_abs.find(abs) != set_abs.end())
      return true;
    set_abs.insert(abs);
  }
  return false;
}

bool Actual::valid() const {
  if (empty())
    return false;
  return (!std::any_of(data_.begin(), data_.end(), [](value_type value) { return value == Actual::invalid; }));
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
  } catch (Exception &E_asimage) {
    E_asimage.display(3);
    try {
      const auto intseq = parse_ints<MR::App::ParsedArgument::IntType>(opt[0][0]);
      if (*std::min_element(intseq.begin(), intseq.end()) < std::numeric_limits<Symbolic::value_type>::min() ||
          *std::max_element(intseq.begin(), intseq.end()) > std::numeric_limits<Symbolic::value_type>::max())
        throw Exception("Integer sequence \"" + str(intseq) +
                        "\" exceeds permissible range for symbolic strides"
                        " ([" +
                        str(std::numeric_limits<Symbolic::value_type>::min()) + " - " +
                        str(std::numeric_limits<Symbolic::value_type>::max()) + "])");
      const Symbolic::vector_type temp = MR::container_cast<Symbolic::vector_type>(intseq);
      user_specification = Symbolic(temp);
    } catch (Exception &E_asstrides) {
      Exception e("Argument \"" + static_cast<std::string>(opt[0][0]) + "\"" +
                  " to option \"-strides\" could not be properly interpreted");
      e.push_back("Error when attempting to interpret as image:");
      for (ArrayIndex i = 0; i != E_asimage.num(); ++i)
        e.push_back(E_asimage[i]);
      e.push_back("Error when attempting to interpret as sequence of integers:");
      for (ArrayIndex i = 0; i != E_asstrides.num(); ++i)
        e.push_back(E_asstrides[i]);
      throw e;
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

  for (ArrayIndex axis = 0; axis != user_specification.size(); ++axis) {
    if (MR::abs(user_specification[axis]) > current.size())
      throw Exception(std::string("strides specified exceed image dimensions:") + //
                      " got " + str(user_specification) + "," +                   //
                      " but image has " + str(current.size()) + " axes");         //
  }

  if (user_specification.is_degenerate())
    throw Exception("duplicate entries provided to \"-strides\" option: " + str(user_specification));

  // 1. For all non-zero entries in the user specification,
  //    place in sequential order,
  //    preserving sign
  // 2. For all zero entries in the user specification,
  //    place in sequential order atop step 1,
  //    preserving sign from current image attributes
  Symbolic::vector_type new_symbolic(current.size(), Symbolic::invalid);
  std::multimap<Symbolic::value_type, Symbolic::value_type> order_userspec;
  std::multimap<Symbolic::value_type, Symbolic::value_type> order_current;
  for (ArrayIndex axis = 0; axis != static_cast<ArrayIndex>(current.size()); ++axis) {
    if (user_specification[axis] == 0)
      order_current.insert({MR::abs(current[axis]), axis});
    else
      order_userspec.insert({MR::abs(user_specification[axis]), axis});
  }
  Symbolic::value_type out_absstride = 1;
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

void set_from_command_line(Header &header, const Permutation &default_order) {
  auto cmdline_strides = __from_command_line(header.strides());
  if (cmdline_strides.has_value())
    header.strides() = *cmdline_strides;
  else if (!default_order.empty())
    header.strides() = header.strides().reordered(default_order);
}

} // namespace MR::Stride
