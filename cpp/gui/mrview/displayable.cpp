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

#include "mrview/displayable.h"

namespace MR::GUI::MRView {

Displayable::Displayable(std::string_view filename)
    : QAction(nullptr),
      lessthan(NaNF),
      greaterthan(NaNF),
      display_midpoint(NaNF),
      display_range(NaNF),
      transparent_intensity(NaNF),
      opaque_intensity(NaNF),
      alpha(NaNF),
      colourmap(0),
      show(true),
      show_colour_bar(true),
      filename(filename),
      value_min(NaNF),
      value_max(NaNF),
      flags_(0x00000000) {
  colour[0] = colour[1] = 255;
  colour[2] = 0;
}

Displayable::~Displayable() {}

bool Displayable::Shader::need_update(const Displayable &object) const {
  return flags != object.flags() || colourmap != object.colourmap;
}

void Displayable::Shader::update(const Displayable &object) {
  flags = object.flags();
  colourmap = object.colourmap;
}

} // namespace MR::GUI::MRView
