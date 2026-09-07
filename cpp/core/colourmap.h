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

#include "exception.h"
#include "mrtrix.h"
#include "types.h"
#include <functional>

namespace MR::ColourMap {

class Entry {
public:
  using basic_map_fn = std::function<Eigen::Array3f(float)>;

  Entry(std::string_view name,
        std::string_view glsl_mapping,
        basic_map_fn basic_mapping,
        const std::string amplitude = "",
        bool special = false,
        bool is_colour = false,
        bool is_rgb = false)
      : name(name),
        glsl_mapping(glsl_mapping),
        basic_mapping(basic_mapping),
        amplitude(amplitude.empty() ? default_amplitude : amplitude),
        special(special),
        is_colour(is_colour),
        is_rgb(is_rgb) {}

  const std::string name;
  const std::string glsl_mapping;
  basic_map_fn basic_mapping;
  const std::string amplitude;
  bool special, is_colour, is_rgb;

  static const std::string default_amplitude;
};

extern const std::vector<Entry> maps;

//! enumeration of the colour maps that can be selected on the command-line
/*! The lowercase enumerator names define the set of valid choices for
 *  colour-map command-line options. Each enumerator name matches the \c name
 *  field of the corresponding entry in the \c maps table, so a selection can be
 *  resolved to a \c maps entry via index() without relying on the enumerator's
 *  underlying integer value. The "Complex" map is deliberately omitted, as it is
 *  not offered as a command-line selection. */
enum class Choice { Gray, Hot, Cool, Jet, Inferno, Viridis, PET, Colour, RGB };

inline size_t num() { return maps.size(); }

inline size_t num_scalar() {
  return std::count_if(maps.begin(), maps.end(), [](const Entry &map) { return map.special; });
}

inline size_t index(std::string_view name) {
  auto it = std::find_if(maps.begin(), maps.end(), [&name](const Entry &map) { return map.name == name; });

  if (it == maps.end())
    throw MR::Exception("Colour map \"" + name + "\" not found");
  return std::distance(maps.begin(), it);
}

inline size_t num_special() {
  return std::count_if(maps.begin(), maps.end(), [](const Entry &map) { return map.special; });
}

} // namespace MR::ColourMap
