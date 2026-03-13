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

#include <optional>
#include <vector>

#include "app.h"
#include "axes.h"
#include "datatype.h"
#include "debug.h"
#include "image_helpers.h"
#include "math/math.h"
#include "types.h"

//! Functions to handle the memory layout of images
/*! Strides are typically supplied as a symbolic list of increments,
 * representing the layout of the data in memory. In this symbolic
 * representation, the actual magnitude of the strides is only important
 * in that it defines the ordering of the various axes.
 *
 * For example, the vector of strides [ 3 -1 -2 ] is valid as a symbolic
 * representation of a image stored as a stack of sagittal slices. Each
 * sagittal slice is stored as rows of voxels ordered from anterior to
 * posterior (i.e. negative y: -1), then stacked superior to inferior (i.e.
 * negative z: -2). These slices are then stacked from left to right (i.e.
 * positive x: 3).
 *
 * This representation is symbolic since it does not take into account the
 * size of the Image along each dimension. To be used in practice, these
 * strides must correspond to the number of intensity values to skip
 * between adjacent voxels along the respective axis. For the example
 * above, the image might consists of 128 sagittal slices, each with
 * dimensions 256x256. The dimensions of the image (as returned by size())
 * are therefore [ 128 256 256 ]. The actual strides needed to navigate
 * through the image, given the symbolic strides above, should therefore
 * be [ 65536 -256 -1 ] (since 256x256 = 65536).
 *
 * Note that a stride of zero is treated as undefined or invalid. This can
 * be used in the symbolic representation to specify that the ordering of
 * the corresponding axis is not important. A suitable stride will be
 * allocated to that axis when the image is initialised (this is done
 * with a call to sanitise()).
 *
 * The functions defined in this namespace provide an interface to
 * manipulate the strides and convert symbolic into actual strides. */

namespace MR::Stride::Legacy {

using List = std::vector<ssize_t>;

//! \cond skip
namespace {
template <class HeaderType> class Compare {
public:
  Compare(const HeaderType &header) : S(header) {}
  bool operator()(const size_t a, const size_t b) const {
    if (S.stride(a) == 0)
      return false;
    if (S.stride(b) == 0)
      return true;
    return MR::abs(S.stride(a)) < MR::abs(S.stride(b));
  }

private:
  const HeaderType &S;
};

class Wrapper {
public:
  Wrapper(List &strides) : S(strides) {}
  size_t ndim() const { return S.size(); }
  const ssize_t &stride(size_t axis) const { return S[axis]; }
  ssize_t &stride(size_t axis) { return S[axis]; }

private:
  List &S;
};

template <class HeaderType> class InfoWrapper : public Wrapper {
public:
  InfoWrapper(List &strides, const HeaderType &header) : Wrapper(strides), D(header) { assert(ndim() == D.ndim()); }
  ssize_t size(size_t axis) const { return D.size(axis); }

private:
  const HeaderType &D;
};
} // namespace
//! \endcond

//! return the strides of \a header as a std::vector<ssize_t>
template <class HeaderType> List get(const HeaderType &header) {
  List ret(header.ndim());
  for (size_t i = 0; i < header.ndim(); ++i)
    ret[i] = header.stride(i);
  return ret;
}

//! set the strides of \a header from a std::vector<ssize_t>
template <class HeaderType> void set(HeaderType &header, const List &stride) {
  size_t n = 0;
  for (; n < std::min<size_t>(header.ndim(), stride.size()); ++n)
    header.stride(n) = stride[n];
  for (; n < stride.size(); ++n)
    header.stride(n) = 0;
}

//! set the strides of \a header from another HeaderType
template <class HeaderType, class FromHeaderType> void set(HeaderType &header, const FromHeaderType &from) {
  set(header, get(from));
}

//! sort range of axes with respect to their absolute stride.
/*! \return a vector of indices of the axes in order of increasing
 * absolute stride.
 * \note all strides should be valid (i.e. non-zero). */
template <class HeaderType>
std::vector<size_t>
order(const HeaderType &header, size_t from_axis = 0, size_t to_axis = std::numeric_limits<size_t>::max()) {
  to_axis = std::min<size_t>(to_axis, header.ndim());
  assert(to_axis > from_axis);
  std::vector<size_t> ret(to_axis - from_axis);
  for (size_t i = 0; i < ret.size(); ++i)
    ret[i] = from_axis + i;
  Compare<HeaderType> compare(header);
  std::sort(ret.begin(), ret.end(), compare);
  return ret;
}

//! sort axes with respect to their absolute stride.
/*! \return a vector of indices of the axes in order of increasing
 * absolute stride.
 * \note all strides should be valid (i.e. non-zero). */
template <> inline std::vector<size_t> order<List>(const List &strides, size_t from_axis, size_t to_axis) {
  const Wrapper wrapper(const_cast<List &>(strides));
  return order(wrapper, from_axis, to_axis);
}

//! remove duplicate and invalid strides.
/*! sanitise the strides of HeaderType \a header by identifying invalid (i.e.
 * zero) or duplicate (absolute) strides, and assigning to each a
 * suitable value. The value chosen for each sanitised stride is the
 * lowest number greater than any of the currently valid strides. */
template <class HeaderType> void sanitise(HeaderType &header) {
  // remove duplicates
  for (size_t i = 0; i < header.ndim() - 1; ++i) {
    if (header.size(i) == 1)
      header.stride(i) = 0;
    if (!header.stride(i))
      continue;
    for (size_t j = i + 1; j < header.ndim(); ++j) {
      if (!header.stride(j))
        continue;
      if (MR::abs(header.stride(i)) == MR::abs(header.stride(j)))
        header.stride(j) = 0;
    }
  }

  size_t max = 0;
  for (size_t i = 0; i < header.ndim(); ++i)
    max = std::max(max, static_cast<size_t>(MR::abs(header.stride(i))));

  for (size_t i = 0; i < header.ndim(); ++i) {
    if (header.stride(i))
      continue;
    if (header.size(i) > 1)
      header.stride(i) = ++max;
  }
}

//! remove duplicate and invalid strides.
/*! sanitise the strides of HeaderType \a header by identifying invalid (i.e.
 * zero) or duplicate (absolute) strides, and assigning to each a
 * suitable value. The value chosen for each sanitised stride is the
 * lowest number greater than any of the currently valid strides. */
template <class HeaderType> inline void sanitise(List &strides, const HeaderType &header) {
  InfoWrapper<HeaderType> wrapper(strides, header);
  sanitise(wrapper);
}

//! remove duplicate and invalid strides.
/*! sanitise the strides in \a current by identifying invalid (i.e.
 * zero) or duplicate (absolute) strides, and assigning to each a
 * suitable value. The value chosen for each sanitised stride is the
 * lowest number greater than any of the currently valid strides. */
List &sanitise(List &current, const List &desired, const std::vector<ssize_t> &header);

//! convert strides from symbolic to actual strides
template <class HeaderType> void actualise(HeaderType &header) {
  sanitise(header);
  std::vector<size_t> x(order(header));
  ssize_t skip = 1;
  for (size_t i = 0; i < header.ndim(); ++i) {
    header.stride(x[i]) = header.stride(x[i]) < 0 ? -skip : skip;
    skip *= header.size(x[i]);
  }
}
//! convert strides from symbolic to actual strides
/*! convert strides from symbolic to actual strides, assuming the strides
 * in \a strides and HeaderType dimensions of \a header. */
template <class HeaderType> inline void actualise(List &strides, const HeaderType &header) {
  InfoWrapper<HeaderType> wrapper(strides, header);
  actualise(wrapper);
}

//! get actual strides:
template <class HeaderType> inline List get_actual(HeaderType &header) {
  List strides(get(header));
  actualise(strides, header);
  return strides;
}

//! get actual strides:
template <class HeaderType> inline List get_actual(const List &strides, const HeaderType &header) {
  List out(strides);
  actualise(out, header);
  return out;
}

//! convert strides from actual to symbolic strides
template <class HeaderType> void symbolise(HeaderType &header) {
  std::vector<size_t> p(order(header));
  for (size_t i = 0; i < p.size(); ++i)
    if (header.stride(p[i]) != 0)
      header.stride(p[i]) = header.stride(p[i]) < 0 ? -(i + 1) : i + 1;
}
//! convert strides from actual to symbolic strides
template <> inline void symbolise(List &strides) {
  Wrapper wrapper(strides);
  symbolise(wrapper);
}

//! get symbolic strides:
template <class HeaderType> inline List get_symbolic(const HeaderType &header) {
  List strides(get(header));
  symbolise(strides);
  return strides;
}

//! get symbolic strides:
template <> inline List get_symbolic(const List &list) {
  List strides(list);
  symbolise(strides);
  return strides;
}

//! calculate offset to start of data
/*! this function caculate the offset (in number of voxels) from the start of the data region
 * to the first voxel value (i.e. at voxel [ 0 0 0 ... ]). */
template <class HeaderType> size_t offset(const HeaderType &header) {
  size_t offset = 0;
  for (size_t i = 0; i < header.ndim(); ++i)
    if (header.stride(i) < 0)
      offset += static_cast<size_t>(-header.stride(i)) * (header.size(i) - 1);
  return offset;
}

//! calculate offset to start of data
/*! this function caculate the offset (in number of voxels) from the start of the data region
 * to the first voxel value (i.e. at voxel [ 0 0 0 ... ]), assuming the
 * strides in \a strides and HeaderType dimensions of \a header. */
template <class HeaderType> size_t offset(List &strides, const HeaderType &header) {
  InfoWrapper<HeaderType> wrapper(strides, header);
  return offset(wrapper);
}

//! produce strides from \c current that match those specified in \c desired
/*! The strides in \c desired should be specified as symbolic strides,
 * and any zero strides will be ignored and replaced with sensible values
 * if needed.  Essentially, this function checks whether the symbolic
 * strides in \c current already match those specified in \c desired. If so,
 * these will be used as-is, otherwise a new set of strides based on \c
 * desired will be produced, as follows. First, non-zero strides in \c
 * desired are used as-is, then the remaining strides are taken from \c
 * current where specified and used with higher values, followed by those
 * strides not specified in either.
 *
 * Note that strides are considered matching even if the differ in their
 * sign - this purpose of this function is to ensure contiguity in RAM
 * along the desired axes, and a reversal in the direction of traversal
 * is not considered to affect this.
 *
 * Examples:
 * - \c current: [ 1 2 3 4 ], \c desired: [ 0 0 0 1 ] => [ 2 3 4 1 ]
 * - \c current: [ 3 -2 4 1 ], \c desired: [ 0 0 0 1 ] => [ 3 -2 4 1 ]
 * - \c current: [ -2 4 -3 1 ], \c desired: [ 1 2 3 0 ] => [ 1 2 3 4 ]
 * - \c current: [ -1 2 -3 4 ], \c desired: [ 1 2 3 0 ] => [ -1 2 -3 4 ]
 *   */
template <class HeaderType> List get_nearest_match(const HeaderType &current, const List &desired) {
  List in(get_symbolic(current)), out(desired);
  out.resize(in.size(), 0);

  std::vector<ssize_t> dims(current.ndim());
  for (size_t n = 0; n < dims.size(); ++n)
    dims[n] = current.size(n);

  for (size_t i = 0; i < out.size(); ++i)
    if (out[i])
      if (MR::abs(out[i]) != MR::abs(in[i]))
        return sanitise(in, out, dims);

  sanitise(in, current);
  return in;
}

//! convenience function to get volume-contiguous strides
inline List contiguous_along_axis(size_t axis) {
  List strides(axis + 1, 0);
  strides[axis] = 1;
  return strides;
}

//! convenience function to get volume-contiguous strides
template <class HeaderType> inline List contiguous_along_axis(size_t axis, const HeaderType &header) {
  return get_nearest_match(header, contiguous_along_axis(axis));
}

//! convenience function to get spatially contiguous strides
template <class HeaderType> inline List contiguous_along_spatial_axes(const HeaderType &header) {
  List strides = get(header);
  for (size_t n = 3; n < strides.size(); ++n)
    strides[n] = 0;
  return strides;
}

List __from_command_line(const List &current);

template <class HeaderType>
inline void set_from_command_line(HeaderType &header, const List &default_strides = List()) {
  auto cmdline_strides = __from_command_line(get(header));
  if (cmdline_strides.size())
    set(header, cmdline_strides);
  else if (!default_strides.empty())
    set(header, default_strides);
}

} // namespace MR::Stride::Legacy

namespace MR::Stride {

extern const App::OptionGroup Options;

template <typename ValueType> class Base {
public:
  using value_type = ValueType;
  using vector_type = std::vector<ValueType>;
  Base(const vector_type &data) : data_(data) { assert(size() <= 5); }
  Base(const size_t num_axes, const ValueType invalid_value) : data_(num_axes, invalid_value) { assert(size() <= 5); }
  Base(const Base &) = default;
  Base() = default;

  bool empty() const { return data_.empty(); }
  size_t size() const { return data_.size(); }

  operator const vector_type &() const { return data_; }
  ValueType operator[](const ArrayIndex index) const {
    assert(index >= 0 && index < size());
    return data_[index];
  }
  bool operator==(const Base &that) const { return that.data_ == data_; }
  bool operator!=(const Base &that) const { return that.data_ != data_; }
  Base &operator=(const Base &that) {
    data_ = that.data_;
    return *this;
  }

  virtual bool is_canonical() const = 0;
  virtual bool is_degenerate() const = 0;
  virtual bool is_sanitised() const = 0;
  virtual void sanitise() = 0;
  virtual bool valid() const = 0;

protected:
  vector_type data_;
};

namespace {
template <class, typename Fallback, typename = void> struct type_or_default {
  using type = Fallback;
};
template <class C, typename F> struct type_or_default<C, F, std::void_t<typename C::type>> {
  using type = typename C::type;
};
} // namespace

template <typename ValueType> std::ostream &operator<<(std::ostream &stream, const Base<ValueType> &base) {
  if (base.empty()) {
    stream << "<NULL>";
    return stream;
  }
  // Explicitly cast Symbolic's int8_t to > 1 byte
  //   so as to not attempt to print as a character
  using IntType = typename type_or_default<ValueType, int>::type;
  stream << static_cast<IntType>(base[0]);
  for (ArrayIndex axis = 1; axis != static_cast<ArrayIndex>(base.size()); ++axis)
    stream << "," << static_cast<IntType>(base[axis]);
  return stream;
}

// Forward definition of classes and explanation:
// Imagine a DICOM dataset (LPS) that has been converted to have volume contiguous in memory
// "Order": Contains a sorted list of axis indices,
//   where from head to tail contains the axis indices from smallest to largest absolute stride
//   In the above example: [3 0 1 2]
//   (ie. first value being "3" means that the axis for which data are contiguous is index 3, or the fourth axis)
class Order;
// "Permutation": Contains for each axis its relative position in that order
//   In the above example: [1 2 3 0]
class Permutation;
// "Symbolic": The presentation of strides as many are accustomed to seeing them,
//   encoding for each axis both its relative ordering and sign
//   In the above example: [-2 -3 4 1]
class Symbolic;
// "Actual": The number of elements that must be traversed within the binary data
//   in order to traverse the image by one index along a respective axis
//   In the above example (assuming a 32x32x20x10 dataset): [-10 -320 10240 1]
class Actual;

class Order : public Base<ArrayIndex> {
public:
  using Base<ArrayIndex>::value_type;
  using Base<ArrayIndex>::vector_type;
  static constexpr value_type invalid = -1;

  explicit Order(const Permutation &permutation);
  explicit Order(const vector_type &data);
  Order(const Order &) = default;

  using Base::operator=;
  operator Axes::Subset() const;

  bool is_canonical() const override;
  bool is_degenerate() const override { return false; }
  bool is_sanitised() const override;
  void sanitise() override;
  bool valid() const override;

  Axes::Subset head(const size_t num_axes) const;
  Permutation permutation() const;
  Axes::Subset subset(const Axes::Subset &axes) const;
  Axes::Subset subset(const ArrayIndex from_axis, const ArrayIndex to_axis = -1) const;
  Axes::Subset tail(const size_t num_axes) const;

  static Order canonical(const size_t num_axes);
};

class Permutation : public Base<ArrayIndex> {
public:
  using Base<ArrayIndex>::value_type;
  using Base<ArrayIndex>::vector_type;
  static constexpr value_type invalid = -1;

  explicit Permutation(const vector_type &data);
  explicit Permutation(const Order &order);
  explicit Permutation(const Symbolic &symbolic);
  explicit Permutation(const Actual &actual);
  Permutation(const Permutation &) = default;
  explicit Permutation() {}
  template <class HeaderType> explicit Permutation(const HeaderType &header) : Permutation(Actual(header)) {}

  using Base::operator=;

  bool is_canonical() const override;
  bool is_degenerate() const override;
  bool is_sanitised() const override;
  void sanitise() override;
  bool valid() const override;

  Permutation head(const size_t num_axes) const;
  Order order() const;
  void resize(const size_t num_axes);
  Permutation sanitised() const;

  static Permutation axis_range(const ArrayIndex from, const ArrayIndex to);
  static Permutation canonical(const size_t num_axes);
  static Permutation contiguous_along_axis(const size_t num_axes, const ArrayIndex axis);
  static Permutation contiguous_along_spatial_axes(const size_t num_axes);
  static const Permutation volume_contiguous;

  friend class Actual;
};

class Symbolic : public Base<int8_t> {
public:
  using Base<int8_t>::value_type;
  using Base<int8_t>::vector_type;
  static constexpr value_type invalid = 0;

  explicit Symbolic(const vector_type &in);
  template <class HeaderType> explicit Symbolic(const HeaderType &header);
  explicit Symbolic(const Actual &actual);
  Symbolic(const Symbolic &) = default;
  explicit Symbolic() {}

  using Base::operator=;

  bool is_canonical() const override;
  bool is_degenerate() const override;
  bool is_sanitised() const override;
  void sanitise() override;
  bool valid() const override;

  template <class HeaderType> Actual actualise(HeaderType &header) const;
  Symbolic block(const ArrayIndex from_axis, const ArrayIndex to_axis = -1) const;
  void conform(const Symbolic &in);
  Symbolic conformed(const Symbolic &in) const;
  void extend(const size_t num_axes);
  Symbolic head(const ArrayIndex num_axes) const;
  Order order() const;
  Permutation permutation() const;
  void resize(const size_t num_axes);
  Symbolic resized(const size_t num_axes) const;
  void reorder(const Permutation &order);
  Symbolic reordered(const Permutation &order) const;
  Symbolic sanitised() const;

  static Symbolic canonical(const size_t num_axes);
};

class Actual : public Base<MemIndex> {
public:
  using Base<MemIndex>::value_type;
  using Base<MemIndex>::vector_type;
  static constexpr value_type invalid = 0;

  explicit Actual(const vector_type &data);
  Actual(const Actual &) = default;
  Actual() = delete;
  template <class HeaderType> Actual(const HeaderType &header);
  Actual(const Symbolic &symbolic, const std::vector<size_t> &sizes);

  using Base::operator=;

  // It is possible for an axity with an axis of unity size
  //   to have two axes with actual strides of equal absolute value;
  //   this is however not actually a problem
  bool is_canonical() const override;
  bool is_degenerate() const override { return false; }
  bool is_sanitised() const override { return valid(); }
  void sanitise() override { assert(false); }
  bool valid() const override;

  template <class HeaderType> bool match(const HeaderType &) const;
  Order order() const;
  Permutation permutation() const;
  Symbolic symbolic() const;

  template <class HeaderType> void to(HeaderType &header) const;
};

// When the header is a template,
//   have no way of knowing whether the input is actual or symbolic;
//   best compatibility is achieved by assuming the latter
template <class HeaderType> Symbolic::Symbolic(const HeaderType &header) : Base(header.ndim(), Symbolic::invalid) {
  Actual::vector_type strides(header.ndim(), Actual::invalid);
  for (ArrayIndex axis = 0; axis != static_cast<ArrayIndex>(header.ndim()); ++axis)
    strides[axis] = header.stride(axis);
  *this = Actual(strides).symbolic();
}

template <class HeaderType> Actual Symbolic::actualise(HeaderType &header) const {
  std::vector<size_t> sizes(header.ndim());
  for (ArrayIndex axis = 0; axis != static_cast<ArrayIndex>(header.ndim()); ++axis)
    sizes[axis] = header.size(axis);
  return Actual(sanitised(), sizes);
}

template <class HeaderType> Actual::Actual(const HeaderType &header) : Base(Symbolic(header).actualise(header)) {}

template <class HeaderType> bool Actual::match(const HeaderType &header) const {
  return *this == Symbolic(header).actualise(header);
}

template <class HeaderType> void Actual::to(HeaderType &header) const {
  assert(header.ndim() >= size());
  ArrayIndex n = 0;
  for (; n < std::min(static_cast<size_t>(header.ndim()), size()); ++n)
    header.stride(n) = data_[n];
  for (; n < header.ndim(); ++n)
    header.stride(n) = Actual::invalid;
}

template <class HeaderType> MemIndex offset(const Stride::Actual &strides, const HeaderType &header) {
  assert(strides.size() == header.ndim());
  MemIndex offset = 0;
  for (ArrayIndex axis = 0; axis < static_cast<ArrayIndex>(header.ndim()); ++axis)
    if (strides[axis] < 0)
      offset += static_cast<MemIndex>(-strides[axis]) * (header.size(axis) - 1);
  return offset;
}

template <class HeaderType> MemIndex offset(const HeaderType &header) {
  // Don't compute a data offset for a HeaderType
  //   for which the strides have not been actualised
  assert(Actual(header).match(header));
  MemIndex offset = 0;
  for (ArrayIndex axis = 0; axis != static_cast<ArrayIndex>(header.ndim()); ++axis)
    if (header.stride(axis) < 0)
      offset -= header.stride(axis) * (header.size(axis) - 1);
  return offset;
}

std::optional<Symbolic> __from_command_line(const Symbolic &current);
std::optional<Symbolic> __from_command_line(const Symbolic &user_specification, const Symbolic &current);

template <class HeaderType>
inline void set_from_command_line(HeaderType &header, const Permutation &default_order = Permutation()) {
  auto cmdline_strides = __from_command_line(Symbolic(header));
  if (cmdline_strides.has_value())
    cmdline_strides->actualise(header);
  else if (!default_order.empty())
    Stride::Symbolic(header).reordered(default_order).actualise(header);
}

} // namespace MR::Stride
