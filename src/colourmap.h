/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#include "exception.h"
#include "types.h"
#include <functional>

namespace MR::ColourMap {

class Entry {
public:
  using basic_map_fn = std::function<Eigen::Array3f(float)>;

  Entry(const char *name,
        const char *glsl_mapping,
        basic_map_fn basic_mapping,
        const char *amplitude = NULL,
        bool special = false,
        bool is_colour = false,
        bool is_rgb = false)
      : name(name),
        glsl_mapping(glsl_mapping),
        basic_mapping(basic_mapping),
        amplitude(amplitude ? amplitude : default_amplitude),
        special(special),
        is_colour(is_colour),
        is_rgb(is_rgb) {}

  const char *name;
  const char *glsl_mapping;
  basic_map_fn basic_mapping;
  const char *amplitude;
  bool special, is_colour, is_rgb;

  static const char *default_amplitude;
};

extern const std::vector<Entry> maps;

inline size_t num() { return maps.size(); }

inline size_t num_scalar() {
  return std::count_if(maps.begin(), maps.end(), [](const Entry &map) { return map.special; });
}

inline size_t index(const std::string &name) {
  auto it = std::find_if(maps.begin(), maps.end(), [&name](const Entry &map) { return map.name == name; });

  if (it == maps.end())
    throw MR::Exception("Colour map \"" + name + "\" not found");
  return std::distance(maps.begin(), it);
}

inline size_t num_special() {
  return std::count_if(maps.begin(), maps.end(), [](const Entry &map) { return map.special; });
}

} // namespace MR::ColourMap
