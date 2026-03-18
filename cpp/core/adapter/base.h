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

#include <vector>

#include "image_helpers.h"
#include "stride.h"
#include "types.h"

namespace MR {
class Header;

namespace Adapter {

template <template <class ImageType> class AdapterType, class ImageType, typename... Args>
inline AdapterType<ImageType> make(const ImageType &parent, Args &&...args) {
  return AdapterType<ImageType>(parent, std::forward<Args>(args)...);
}

template <class AdapterType, class ImageType>
class Base : public ImageBase<AdapterType, typename ImageType::value_type> {
public:
  using value_type = typename ImageType::value_type;

  Base(const ImageType &parent) : parent_(parent) {}

  template <class U> const Base &operator=(const U &V) { return parent_ = V; }

  FORCE_INLINE ImageType &parent() { return parent_; }
  FORCE_INLINE bool operator!() const { return !valid(); }
  FORCE_INLINE const ImageType &parent() const { return parent_; }

  FORCE_INLINE virtual std::string_view name() const { return parent_.name(); }
  FORCE_INLINE virtual bool valid() const { return parent_.valid(); }

  FORCE_INLINE virtual size_t ndim() const { return parent_.ndim(); }
  FORCE_INLINE virtual VoxelIndex size(const ArrayIndex axis) const { return parent_.size(axis); }
  FORCE_INLINE virtual default_type spacing(const ArrayIndex axis) const { return parent_.spacing(axis); }
  FORCE_INLINE virtual Stride::Actual::value_type stride(const ArrayIndex axis) const { return parent_.stride(axis); }
  FORCE_INLINE virtual const transform_type &transform() const { return parent_.transform(); }
  FORCE_INLINE virtual const KeyValues &keyval() const { return parent_.keyval(); }

  FORCE_INLINE virtual VoxelIndex get_index(const ArrayIndex axis) const { return parent_.index(axis); }
  FORCE_INLINE virtual void move_index(const ArrayIndex axis, const VoxelIndex increment) {
    parent_.index(axis) += increment;
  }

  FORCE_INLINE virtual value_type get_value() const { return parent_.value(); }
  FORCE_INLINE virtual void set_value(value_type val) { parent_.value() = val; }

  FORCE_INLINE virtual void reset() { parent_.reset(); }

  friend std::ostream &operator<<(std::ostream &stream, const Base &V) {
    stream << "image adapter \"" << V.name() << "\", datatype " << MR::DataType::from<value_type>().specifier()
           << ", position [ ";
    for (StdIndex n = 0; n < V.ndim(); ++n)
      stream << V[n] << " ";
    stream << "], value = " << V.value();
    return stream;
  }

protected:
  ImageType parent_;
};

} // namespace Adapter
} // namespace MR
