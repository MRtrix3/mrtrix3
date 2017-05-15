/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __connectome_lut_h__
#define __connectome_lut_h__


#include "app.h"

#include "connectome/connectome.h"

#include <map>
#include <string>
#include <vector>


namespace MR {
namespace Connectome {




// Class for storing any useful information regarding a parcellation node that
//   may be imported from a lookup table
class LUT_node
{ MEMALIGN(LUT_node)

  public:

    using RGB = Eigen::Array<uint8_t, 3, 1>;

    LUT_node (const std::string& n) :
      name (n),
      colour (0, 0, 0),
      alpha (255) { }

    LUT_node (const std::string& n, const std::string& sn) :
      name (n),
      short_name (sn),
      colour (0, 0, 0),
      alpha (255) { }

    LUT_node (const std::string& n, const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a = 255) :
      name (n),
      colour (r, g, b),
      alpha (a) { }

    LUT_node (const std::string& n, const std::string& sn, const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a = 255) :
      name (n),
      short_name (sn),
      colour (r, g, b),
      alpha (a) { }

    LUT_node (const std::string& n, const RGB& rgb, const uint8_t a = 255) :
      name (n),
      colour (rgb),
      alpha (a) { }


    void set_colour (const uint8_t r, const uint8_t g, const uint8_t b) { colour = RGB {r,g,b}; }
    void set_colour (const RGB& rgb) { colour = rgb; }
    void set_alpha  (const uint8_t a) { alpha = a; }


    const std::string& get_name()       const { return name; }
    const std::string& get_short_name() const { return short_name.size() ? short_name : name; }
    const RGB&         get_colour()     const { return colour; }
    uint8_t            get_alpha()      const { return alpha; }


  private:
    std::string name, short_name;
    RGB colour;
    uint8_t alpha;

};




class LUT : public std::multimap<node_t, LUT_node>
{ MEMALIGN(LUT)
    enum file_format { LUT_NONE, LUT_BASIC, LUT_FREESURFER, LUT_AAL, LUT_ITKSNAP, LUT_MRTRIX };
  public:
    using map_type = std::multimap<node_t, LUT_node>;
    LUT () : exclusive (true) { }
    LUT (const std::string&);
    void load (const std::string&);
    bool is_exclusive() const { return exclusive; }
  private:
    bool exclusive;

    file_format guess_file_format (const std::string&);

    void parse_line_basic      (const std::string&);
    void parse_line_freesurfer (const std::string&);
    void parse_line_aal        (const std::string&);
    void parse_line_itksnap    (const std::string&);
    void parse_line_mrtrix     (const std::string&);

    void check_and_insert (const node_t, const LUT_node&);

};




// Convenience function for constructing a mapping from one lookup table to another
// NOTE: If the TARGET LUT contains multiple entries for a particular index, and a
//   mapping TO that index is required, the conversion is ill-formed.
vector<node_t> get_lut_mapping (const LUT&, const LUT&);




}
}


#endif

