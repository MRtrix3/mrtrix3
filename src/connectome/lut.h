/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */



#ifndef __connectome_lut_h__
#define __connectome_lut_h__


#include "app.h"

#include "connectome/connectome.h"

#include <map>
#include <string>


namespace MR {
namespace Connectome {





enum lut_format { LUT_NONE, LUT_BASIC, LUT_FREESURFER, LUT_AAL, LUT_ITKSNAP };
extern const char* lut_format_strings[];

extern const App::OptionGroup LookupTableOption;
class Node_map;
void load_lut_from_cmdline (Node_map&);

typedef Eigen::Array<uint8_t, 3, 1> RGB;



// Class for storing any useful information regarding a parcellation node that
//   may be imported from a lookup table
class Node_info
{

  public:
    Node_info (const std::string& n) :
      name (n),
      colour (0, 0, 0),
      alpha (255) { }

    Node_info (const std::string& n, const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a = 255) :
      name (n),
      colour (r, g, b),
      alpha (a) { }

    Node_info (const std::string& n, const RGB& rgb, const uint8_t a = 255) :
      name (n),
      colour (rgb),
      alpha (a) { }


    void set_colour (const uint8_t r, const uint8_t g, const uint8_t b) { colour = RGB {r,g,b}; }
    void set_colour (const RGB& rgb) { colour = rgb; }
    void set_alpha  (const uint8_t a) { alpha = a; }


    const std::string& get_name()   const { return name; }
    const RGB&         get_colour() const { return colour; }
    uint8_t            get_alpha()  const { return alpha; }


  private:
    std::string name;
    RGB colour;
    uint8_t alpha;

};




class Node_map : public std::map<node_t, Node_info>
{
  public:
    void load (const std::string&, const lut_format);
  private:
    void parse_line_basic      (const std::string&);
    void parse_line_freesurfer (const std::string&);
    void parse_line_aal        (const std::string&);
    void parse_line_itksnap    (const std::string&);

};



}
}


#endif

