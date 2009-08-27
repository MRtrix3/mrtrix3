/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#include <fstream>

#include "file/misc.h"
#include "file/path.h"
#include "file/entry.h"
#include "image/misc.h"
#include "image/header.h"
#include "image/format/list.h"
#include "get_set.h"

namespace MR {
  namespace Image {
    namespace Format {

      bool XDS::read (Header& H) const
      {
        if (!Path::has_suffix (H.name(), ".bfloat") && !Path::has_suffix (H.name(), ".bshort")) return (false);

        H.axes.ndim() = 4;
        int BE;

        std::string name (H.name());
        name.replace (name.size()-6, 6, "hdr");

        std::ifstream in (name.c_str());
        if (!in) throw Exception ("error reading header file \"" + name + "\": " + strerror (errno));
        in >> H.axes.dim(1) >> H.axes.dim(0) >> H.axes.dim(3) >> BE;
        in.close();

        H.datatype() = ( Path::has_suffix (H.name(), ".bfloat") ? DataType::Float32 : DataType::UInt16 );

        if (BE) H.datatype().set_flag (DataType::LittleEndian);
        else H.datatype().set_flag (DataType::BigEndian);

        H.axes.dim(2) = 1;

        H.axes.vox(0) = H.axes.vox(1) = 3.0;
        H.axes.vox(2) = 10.0;
        H.axes.vox(3) = 1.0;

        H.axes.order(0) = 0;                H.axes.forward(0) = false;
        H.axes.order(1) = 1;                H.axes.forward(1) = false;
        H.axes.order(2) = Axes::undefined;  H.axes.forward(2) = true;
        H.axes.order(3) = 2;                H.axes.forward(3) = true;

        H.axes.description(0) = Axes::left_to_right;
        H.axes.description(1) = Axes::posterior_to_anterior;
        H.axes.description(2) = Axes::inferior_to_superior;
        H.axes.description(3) = Axes::time;

        H.axes.units(0) = Axes::millimeters; 
        H.axes.units(1) = Axes::millimeters; 
        H.axes.units(2) = Axes::millimeters;
        H.axes.units(3) = Axes::milliseconds;

        H.files.push_back (File::Entry (H.name()));

        return (true);
      }







      bool XDS::check (Header& H, int num_axes) const
      {
        if (!Path::has_suffix (H.name(), ".bfloat") && !Path::has_suffix (H.name(), ".bshort")) return (false);
        if (num_axes > 4) throw Exception ("cannot create XDS image with more than 4 dimensions");
        if (num_axes == 4 && H.axes.dim(2) > 1) throw Exception ("cannot create multi-slice XDS image with a single file");
        if (num_axes < 2) throw Exception ("cannot create XDS image with less than 2 dimensions");

        H.axes.ndim() = 4;

        H.axes.dim(2) = 1;
        for (size_t n = 0; n < 4; n++)
          if (H.axes.dim(n) < 1) H.axes.dim(n) = 1;

        H.axes.vox(0) = H.axes.vox(1) = 3.0;
        H.axes.vox(2) = 10.0;
        H.axes.vox(3) = 1.0;

        H.axes.order(0) = 0;                H.axes.forward(0) = false;
        H.axes.order(1) = 1;                H.axes.forward(1) = false;
        H.axes.order(2) = Axes::undefined;  H.axes.forward(2) = true;
        H.axes.order(3) = 2;                H.axes.forward(3) = true;

        H.axes.description(0) = Axes::left_to_right;
        H.axes.description(1) = Axes::posterior_to_anterior;
        H.axes.description(2) = Axes::inferior_to_superior;
        H.axes.description(3) = Axes::time;

        H.axes.units(0) = Axes::millimeters; 
        H.axes.units(1) = Axes::millimeters; 
        H.axes.units(2) = Axes::millimeters;
        H.axes.units(3) = Axes::milliseconds;

        bool is_BE = H.datatype().is_big_endian();

        H.datatype() = ( Path::has_suffix (H.name(), ".bfloat") ? DataType::Float32 : DataType::UInt16 );

        if (is_BE) H.datatype().set_flag (DataType::BigEndian);
        else H.datatype().set_flag (DataType::LittleEndian);

        return (true);
      }




      void XDS::create (Header& H) const
      {
        std::string header_name (H.name());
        header_name.replace (header_name.size()-6, 6, "hdr");

        std::ofstream out(header_name.c_str());
        if (!out) throw Exception ("error writing header file \"" + header_name + "\": " + strerror (errno));

        out << H.axes.dim(1) << " " << H.axes.dim(0) << " " << H.axes.dim(3) 
            << " " << ( H.datatype().is_little_endian() ? 1 : 0 ) << "\n";
        out.close();

        File::create (H.name(), memory_footprint (H, "11 1"));
        H.files.push_back (File::Entry (H.name()));
      }

    } 
  }  
}

