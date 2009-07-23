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


    08-09-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * fix handling of mosaic slice ordering (using SliceNormalVector entry in CSA header)

*/

#include "file/path.h"
#include "file/dicom/image.h"
#include "file/dicom/series.h"
#include "file/dicom/study.h"
#include "file/dicom/patient.h"
#include "file/dicom/csa_entry.h"

namespace MR {
  namespace File {
    namespace Dicom {

      void Image::parse_item (Element& item, const std::string& dirname)
      {
        switch (item.group) {
          case 0x0004U: 
            if (item.element == 0x1500U) {
              assert (dirname.size());
              filename = dirname;
              std::vector<std::string> V (item.get_string());
              for (uint n = 0; n < V.size(); n++) 
                filename = Path::join (filename, V[n]);
            }
            break;
          case 0x0018U: 
            switch (item.element) {
              case 0x0050U: slice_thickness = item.get_float()[0]; return;
              case 0x1310U: acq_dim[0] = item.get_uint()[0];
                            acq_dim[1] = item.get_uint()[3];
                            return;
              case 0x0024U: sequence_name = item.get_string()[0];
                            if (!sequence_name.size()) return;
                            int c = sequence_name.size()-1;
                            while (c >= 0 && isdigit (sequence_name[c])) c--;
                            c++;
                            sequence = to<uint> (sequence_name.substr (c));
                            return;
            }
            return;
          case 0x0020U: 
            switch (item.element) {
              case 0x0012U: acq = item.get_uint()[0]; return;
              case 0x0013U: instance = item.get_uint()[0]; return;
              case 0x0032U: position_vector[0] = item.get_float()[0];
                            position_vector[1] = item.get_float()[1];
                            position_vector[2] = item.get_float()[2];
                            return;
              case 0x0037U: orientation_x[0] = item.get_float()[0];
                            orientation_x[1] = item.get_float()[1];
                            orientation_x[2] = item.get_float()[2];
                            orientation_y[0] = item.get_float()[3];
                            orientation_y[1] = item.get_float()[4];
                            orientation_y[2] = item.get_float()[5];
                            Math::normalise (orientation_x);
                            Math::normalise (orientation_y);
                            return;
            }
            return;
          case 0x0028U:
            switch (item.element) {
              case 0x0010U: dim[1] = item.get_uint()[0]; return;
              case 0x0011U: dim[0] = item.get_uint()[0]; return;
              case 0x0030U: pixel_size[0] = item.get_float()[0];
                            pixel_size[1] = item.get_float()[1]; 
                            return;
              case 0x0100U: bits_alloc = item.get_uint()[0]; return;
              case 0x1052U: scale_intercept = item.get_float()[0]; return;
              case 0x1053U: scale_slope = item.get_float()[0]; return;
            }
            return;
          case 0x0029U:
            if (item.element == 0x1010U || item.element == 0x1020U) {
              decode_csa (item.data, item.data + item.size);
              return;
            }
            else return;
          case 0x7FE0U: 
            if (item.element == 0x0010U) {
              data = item.offset (item.data);
              data_size = item.size;
              is_BE = item.is_big_endian();
              return;
            }
            return;
        }

        return;
      }






      void Image::read ()
      {
        Element item;
        item.set (filename);

        while (item.read()) 
          if (item.item_number.size() == 0)
            parse_item (item);

        calc_distance();
      }





      bool Image::operator< (const Image& ima) const
      {
        if (acq != ima.acq) return (acq < ima.acq);
        assert (!isnan(distance));
        assert (!isnan(ima.distance));
        if (distance != ima.distance) return (distance < ima.distance);
        if (sequence != ima.sequence) return (sequence < ima.sequence);
        if (instance != ima.instance) return (instance < ima.instance);
        return (false);
      }






      void Image::print_fields (bool dcm, bool csa) const
      {
        if (!filename.size()) return;

        Element item;
        item.set (filename);

        fprintf (stdout, 
            "**********************************************************\n"\
            "  %s\n" \
            "**********************************************************\n", 
            filename.c_str());

        while (item.read()) {
          if (dcm) item.print();
          if (csa && item.group == 0x0029U) {
            if (item.element == 0x1010U || item.element == 0x1020U) {
              CSAEntry entry (item.data, item.data + item.size, true);
              while (entry.parse() == 0);
            }
          }
        }
      }






      void Image::decode_csa (const uint8_t* start, const uint8_t* end)
      {
        CSAEntry entry (start, end);

        while (entry.parse()) {
          if (strcmp ("B_value", entry.key()) == 0) bvalue = entry.get_float();
          else if (strcmp ("DiffusionGradientDirection", entry.key()) == 0) entry.get_float (G);
          else if (strcmp ("NumberOfImagesInMosaic", entry.key()) == 0) images_in_mosaic = entry.get_int();
          else if (strcmp ("SliceNormalVector", entry.key()) == 0) entry.get_float (orientation_z);
        }

        if (G[0] && bvalue)
          if (Math::abs(G[0]) > 1.0 && Math::abs(G[1]) > 1.0 && Math::abs(G[2]) > 1.0)
            bvalue = G[0] = G[1] = G[2] = 0.0;
      }






      std::ostream& operator<< (std::ostream& stream, const Image& item)
      {
        stream << "            " << ( item.instance == UINT_MAX ? 0 : item.instance ) << "#" 
          << ( item.acq == UINT_MAX ? 0 : item.acq) << ":"
          << ( item.sequence == UINT_MAX ? 0 : item.sequence ) << " (" 
          << ( item.sequence_name.size() ? item.sequence_name : "?" ) << "), "
          << item.dim[0] << "x" << item.dim[1] << ", "
          << item.pixel_size[0] << "x" << item.pixel_size[1] << " x " 
          << item.slice_thickness << " mm, [ "
          << item.position_vector[0] << " " << item.position_vector[1] << " " << item.position_vector[2] << " ] [ "
          << item.orientation_x[0] << " " << item.orientation_x[1] << " " << item.orientation_x[2] << " ] [ "
          << item.orientation_y[0] << " " << item.orientation_y[1] << " " << item.orientation_y[2] << " ] "
          << ( item.filename.size() ? item.filename : "" ) << "\n";
             
        return (stream);
      }




    }
  }
}

