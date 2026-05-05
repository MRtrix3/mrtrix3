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

#include "image.h"
#include "image_helpers.h"

#include "fixel/correspondence/correspondence.h"

namespace MR::Fixel::Correspondence {

// Information to be stored for each fixel that will be useful during correspondence
class Fixel {
public:
  Fixel(const Helper::ConstRow<Image<float>> &direction, const float density)
      : _dir(dir_t{direction[0], direction[1], direction[2]}.normalized()), _density(density) {}
  Fixel(const dir_t &direction, const float density) : _dir(direction), _density(density) {}
  const dir_t &dir() const { return _dir; }
  float density() const { return _density; }
  float dot(const Fixel &i) const { return dir().dot(i.dir()); }
  float dot(const dir_t &d) const { return dir().dot(d); }
  float absdot(const Fixel &i) const { return std::fabs(dir().dot(i.dir())); }
  float absdot(const dir_t &d) const { return std::fabs(dir().dot(d)); }

protected:
  const dir_t _dir;
  const float _density;
};

} // namespace MR::Fixel::Correspondence
