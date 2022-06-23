/* Copyright (c) 2008-2022 the MRtrix3 contributors.
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

#ifndef __colourmap_h__
#define __colourmap_h__

#include <functional>
#include "types.h"


namespace MR
{
  namespace ColourMap
  {

    class Entry { MEMALIGN(Entry)
      public:

        using basic_map_fn = std::function< Eigen::Array3f (float) >;

        Entry (const std::string& name, const std::string& glsl_mapping, basic_map_fn basic_mapping,
            const std::string& amplitude = std::string(),
            bool special = false, bool is_colour = false, bool is_rgb = false) :
          name (name),
          glsl_mapping (glsl_mapping),
          basic_mapping (basic_mapping),
          amplitude (amplitude.size() ? amplitude : default_amplitude),
          special (special),
          is_colour (is_colour),
          is_rgb (is_rgb) { }

        const std::string name;
        const std::string glsl_mapping;
        basic_map_fn basic_mapping;
        const std::string amplitude;
        bool special, is_colour, is_rgb;

        static const char* default_amplitude;

    };

    extern vector<Entry> maps;
    void initialise ();





    inline size_t num () { return maps.size(); }


    inline size_t num_scalar () {
      size_t n = 0;
      for (const auto& entry : maps) {
        if (!entry.special)
          ++n;
      }
      return n;
    }

    inline size_t index (const std::string& name) {
      for (size_t n = 0; n < maps.size(); ++n)
        if (maps[n].name == name)
          return n;
      assert (false /* should not be here! */);
      return 0;
    }


    inline size_t num_special () {
      size_t n = 0;
      for (const auto& entry : maps) {
        if (entry.special)
          ++n;
      }
      return n;
    }


  }
}

#endif




