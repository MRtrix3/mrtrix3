/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2013.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

 */



#ifndef __dwi_tractography_connectomics_lut_h__
#define __dwi_tractography_connectomics_lut_h__


#include "app.h"
#include "args.h"
#include "point.h"

#include "dwi/tractography/connectomics/connectomics.h"

#include <map>
#include <string>


namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectomics {



class Node_info;
typedef std::map<node_t, Node_info> Node_map;
extern const App::OptionGroup LookupTableOption;
void load_lookup_table (Node_map&);





// Class for storing any useful information regarding a parcellation node that
//   may be imported from a lookup table
class Node_info
{

  public:
    Node_info (const std::string& n) :
      name (n),
      colour (Point<uint8_t> (0, 0, 0)),
      alpha (255) { }

    Node_info (const std::string& n, const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a = 255) :
      name (n),
      colour (Point<uint8_t> (r, g, b)),
      alpha (a) { }

    Node_info (const std::string& n, const Point<uint8_t>& rgb, const uint8_t a = 255) :
      name (n),
      colour (rgb),
      alpha (a) { }


    void set_colour (const uint8_t r, const uint8_t g, const uint8_t b) { colour = Point<uint8_t> (r,g,b); }
    void set_colour (const Point<uint8_t> rgb) { colour = rgb; }
    void set_alpha  (const uint8_t a) { alpha = a; }


    const std::string&    get_name()   const { return name; }
    const Point<uint8_t>& get_colour() const { return colour; }
    uint8_t               get_alpha()  const { return alpha; }


  private:
    std::string name;
    Point<uint8_t> colour;
    uint8_t alpha;

};





}
}
}
}


#endif

