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

#include "surface/mesh.h"

namespace MR::Surface {

class Scalar : public Eigen::Array<default_type, Eigen::Dynamic, 1> {

public:
  using Base = Eigen::Array<default_type, Eigen::Dynamic, 1>;

  Scalar(std::string_view, const Mesh &);

  Scalar(const Scalar &that) = default;

  Scalar(Scalar &&that) : Base(std::move(that)), name(std::move(that.name)) {}

  Scalar() {}

  Scalar &operator=(Scalar &&that) {
    Base::operator=(std::move(that));
    name = std::move(that.name);
    return *this;
  }

  Scalar &operator=(const Scalar &that) {
    Base::operator=(that);
    name = that.name;
    return *this;
  }

  void clear() {
    Base::resize(0);
    name.clear();
  }

  void save(std::string_view) const;

  std::string_view get_name() const { return name; }
  void set_name(std::string_view s) { name = s; }

private:
  std::string name;

  void load_fs_w(std::string_view, const Mesh &);
  void load_fs_curv(std::string_view, const Mesh &);
};

} // namespace MR::Surface
