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

#include "file/path.h"
#include "image/mapper.h"
#include "image/misc.h"
#include "image/format/list.h"
#include "image/name_parser.h"
#include "get_set.h"

namespace MR {
  namespace Image {
    namespace Format {

      namespace {

        const char* FormatBFloat = "XDS (floating point)";
        const char* FormatBShort = "XDS (integer)";

      }






      bool XDS::read (Mapper& dmap, Header& H) const
      {
        if (!Path::has_suffix (H.name, ".bfloat") && !Path::has_suffix (H.name, ".bshort")) return (false);

        H.axes.resize (4);
        int BE;

        std::string name (H.name);
        name.replace (name.size()-6, 6, "hdr");

        std::ifstream in (name.c_str());
        if (!in) throw Exception ("error reading header file \"" + name + "\": " + strerror (errno));
        in >> H.axes[1].dim >> H.axes[0].dim >> H.axes[3].dim >> BE;
        in.close();

        if (Path::has_suffix (H.name, ".bfloat")) {
          H.data_type = DataType::Float32;
          H.format = FormatBFloat;
        }
        else {
          H.data_type = DataType::UInt16;
          H.format = FormatBShort;
        }

        if (BE) H.data_type.set_flag (DataType::LittleEndian);
        else H.data_type.set_flag (DataType::BigEndian);

        H.axes[2].dim = 1;

        H.axes[0].vox = H.axes[1].vox = 3.0;
        H.axes[2].vox = 10.0;
        H.axes[3].vox = 1.0;

        H.axes[0].order = 0;                H.axes[0].forward = false;
        H.axes[1].order = 1;                H.axes[1].forward = false;
        H.axes[2].order = Axis::undefined;  H.axes[2].forward = true;
        H.axes[3].order = 2;                H.axes[3].forward = true;

        H.axes[0].desc = Axis::left_to_right;
        H.axes[1].desc = Axis::posterior_to_anterior;
        H.axes[2].desc = Axis::inferior_to_superior;
        H.axes[3].desc = Axis::time;

        H.axes[0].units = Axis::millimeters; 
        H.axes[1].units = Axis::millimeters; 
        H.axes[2].units = Axis::millimeters;
        H.axes[3].units = Axis::milliseconds;

        dmap.add (H.name, 0);

        return (true);
      }







      bool XDS::check (Header& H, int num_axes) const
      {
        if (!Path::has_suffix (H.name, ".bfloat") && !Path::has_suffix (H.name, ".bshort")) return (false);
        if (num_axes > 4) throw Exception ("cannot create XDS image with more than 4 dimensions");
        if (num_axes == 4 && H.axes[2].dim > 1) throw Exception ("cannot create multi-slice XDS image with a single file");
        if (num_axes < 2) throw Exception ("cannot create XDS image with less than 2 dimensions");

        H.axes.resize (4);

        H.axes[2].dim = 1;
        for (size_t n = 0; n < 4; n++)
          if (H.axes[n].dim < 1) H.axes[n].dim = 1;

        H.axes[0].vox = H.axes[1].vox = 3.0;
        H.axes[2].vox = 10.0;
        H.axes[3].vox = 1.0;

        H.axes[0].order = 0;                H.axes[0].forward = false;
        H.axes[1].order = 1;                H.axes[1].forward = false;
        H.axes[2].order = Axis::undefined;  H.axes[2].forward = true;
        H.axes[3].order = 2;                H.axes[3].forward = true;

        H.axes[0].desc = Axis::left_to_right;
        H.axes[1].desc = Axis::posterior_to_anterior;
        H.axes[2].desc = Axis::inferior_to_superior;
        H.axes[3].desc = Axis::time;

        H.axes[0].units = Axis::millimeters; 
        H.axes[1].units = Axis::millimeters; 
        H.axes[2].units = Axis::millimeters;
        H.axes[3].units = Axis::milliseconds;

        bool is_BE = H.data_type.is_big_endian();

        if (Path::has_suffix (H.name, ".bfloat")) {
          H.data_type = DataType::Float32;
          H.format = FormatBFloat;
        }
        else {
          H.data_type = DataType::UInt16;
          H.format = FormatBShort;
        }

        if (is_BE) H.data_type.set_flag (DataType::BigEndian);
        else H.data_type.set_flag (DataType::LittleEndian);

        return (true);
      }




      void XDS::create (Mapper& dmap, const Header& H) const
      {
        off64_t msize = memory_footprint (H.data_type, voxel_count (H.axes, "11 1"));

        std::string header_name (H.name);
        header_name.replace (header_name.size()-6, 6, "hdr");

        std::ofstream out(header_name.c_str());
        if (!out) throw Exception ("error writing header file \"" + header_name + "\": " + strerror (errno));

        out << H.axes[1].dim << " " << H.axes[0].dim << " " << H.axes[3].dim 
            << " " << ( H.data_type.is_little_endian() ? 1 : 0 ) << "\n";
        out.close();

        dmap.add (H.name, 0, msize);
      }

    } 
  }  
}

