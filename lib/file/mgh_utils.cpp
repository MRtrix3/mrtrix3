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

#include "header.h"
#include "raw.h"
#include "file/mgh_utils.h"
#include "file/nifti_utils.h"
#include "file/ofstream.h"

namespace MR
{
  namespace File
  {
    namespace MGH
    {


      bool read_header (Header& H, const mgh_header& MGHH)
      {
        bool is_BE = false;
        if (Raw::fetch_<int32_t> (&MGHH.version, is_BE) != 1) {
          is_BE = true;
          if (Raw::fetch_<int32_t> (&MGHH.version, is_BE) != 1)
            throw Exception ("image \"" + H.name() + "\" is not in MGH format (version != 1)");
        }

        const size_t ndim = (Raw::fetch_<int32_t> (&MGHH.nframes, is_BE) > 1) ? 4 : 3;
        H.ndim() = ndim;
        H.size (0) = Raw::fetch_<int32_t> (&MGHH.width, is_BE);
        H.size (1) = Raw::fetch_<int32_t> (&MGHH.height, is_BE);
        H.size (2) = Raw::fetch_<int32_t> (&MGHH.depth, is_BE);
        if (ndim == 4)
          H.size (3) = Raw::fetch_<int32_t> (&MGHH.nframes, is_BE);

        H.spacing (0) = Raw::fetch_<float> (&MGHH.spacing_x, is_BE);
        H.spacing (1) = Raw::fetch_<float> (&MGHH.spacing_y, is_BE);
        H.spacing (2) = Raw::fetch_<float> (&MGHH.spacing_z, is_BE);

        for (size_t i = 0; i != ndim; ++i)
          H.stride (i) = i + 1;

        DataType dtype;
        int32_t type = Raw::fetch_<int32_t> (&MGHH.type, is_BE);
        switch (type) {
          case MGH_TYPE_UCHAR: dtype = DataType::UInt8;   break;
          case MGH_TYPE_SHORT: dtype = DataType::Int16;   break;
          case MGH_TYPE_INT:   dtype = DataType::Int32;   break;
          case MGH_TYPE_FLOAT: dtype = DataType::Float32; break;
          default: throw Exception ("unknown data type for MGH image \"" + H.name() + "\" (" + str (type) + ")");
        }
        if (dtype != DataType::UInt8) {
          if (is_BE)
            dtype.set_flag (DataType::BigEndian);
          else
            dtype.set_flag (DataType::LittleEndian);
        }
        H.datatype() = dtype;
        H.reset_intensity_scaling();

        transform_type& M (H.transform());

        const int16_t RAS = Raw::fetch_<int16_t> (&MGHH.goodRASFlag, is_BE);
        if (RAS) {

          M(0,0) = Raw::fetch_<float> (&MGHH.x_r, is_BE);
          M(0,1) = Raw::fetch_<float> (&MGHH.y_r, is_BE);
          M(0,2) = Raw::fetch_<float> (&MGHH.z_r, is_BE);

          M(1,0) = Raw::fetch_<float> (&MGHH.x_a, is_BE);
          M(1,1) = Raw::fetch_<float> (&MGHH.y_a, is_BE);
          M(1,2) = Raw::fetch_<float> (&MGHH.z_a, is_BE);

          M(2,0) = Raw::fetch_<float> (&MGHH.x_s, is_BE);
          M(2,1) = Raw::fetch_<float> (&MGHH.y_s, is_BE);
          M(2,2) = Raw::fetch_<float> (&MGHH.z_s, is_BE);

          M(0,3) = Raw::fetch_<float> (&MGHH.c_r, is_BE);
          M(1,3) = Raw::fetch_<float> (&MGHH.c_a, is_BE);
          M(2,3) = Raw::fetch_<float> (&MGHH.c_s, is_BE);

          for (size_t i = 0; i < 3; ++i) {
            for (size_t j = 0; j < 3; ++j)
              M(i,3) -= 0.5 * H.size(j) * H.spacing(j) * M(i,j);
          }


        } else {

          // Default transformation matrix, assumes coronal orientation
          M(0,0) = -1.0; M(0,1) =  0.0; M(0,2) =  0.0; M(0,3) = 0.0;
          M(1,0) =  0.0; M(1,1) =  0.0; M(1,2) = -1.0; M(1,3) = 0.0;
          M(2,0) =  0.0; M(2,1) = +1.0; M(2,2) =  0.0; M(2,3) = 0.0;

        }

        return is_BE;

      }



      void read_other (Header& H, const mgh_other& MGHO, const bool is_BE) {

        if (Raw::fetch_<float> (&MGHO.tr, is_BE) != 0.0f)
          add_line (H.keyval()["comments"], "TR: "   + str (Raw::fetch_<float> (&MGHO.tr, is_BE)) + "ms");
        if (Raw::fetch_<float> (&MGHO.flip_angle, is_BE) != 0.0f)
          add_line (H.keyval()["comments"], "Flip: " + str (Raw::fetch_<float> (&MGHO.flip_angle, is_BE) * 180.0 / Math::pi) + "deg");
        if (Raw::fetch_<float> (&MGHO.te, is_BE) != 0.0f)
          add_line (H.keyval()["comments"], "TE: "   + str (Raw::fetch_<float> (&MGHO.te, is_BE)) + "ms");
        if (Raw::fetch_<float> (&MGHO.ti, is_BE) != 0.0f)
          add_line (H.keyval()["comments"], "TI: "   + str (Raw::fetch_<float> (&MGHO.ti, is_BE)) + "ms");

        // Ignore FoV field

        for (const auto i : MGHO.tags) 
          add_line (H.keyval()["comments"], i);

      }





      void write_header (mgh_header& MGHH, const Header& H)
      {
        bool is_BE = H.datatype().is_big_endian();

        const size_t ndim = H.ndim();
        if (ndim > 4)
          throw Exception ("MGH file format does not support images of more than 4 dimensions");

        vector<size_t> axes;
        auto M = File::NIfTI::adjust_transform (H, axes);

        Raw::store<int32_t> (1, &MGHH.version, is_BE);
        Raw::store<int32_t> (H.size (axes[0]), &MGHH.width, is_BE);
        Raw::store<int32_t> ((ndim > 1) ? H.size (axes[1]) : 1, &MGHH.height, is_BE);
        Raw::store<int32_t> ((ndim > 2) ? H.size (axes[2]) : 1, &MGHH.depth, is_BE);
        Raw::store<int32_t> ((ndim > 3) ? H.size (3) : 1, &MGHH.nframes, is_BE);

        const DataType& dt = H.datatype();
        if (dt.is_complex())
          throw Exception ("MGH file format does not support complex types");
        if ((dt() & DataType::Type) == DataType::Bit)
          throw Exception ("MGH file format does not support bit type");
        if (((dt() & DataType::Type) == DataType::UInt8) && dt.is_signed())
          throw Exception ("MGH file format does not support signed 8-bit integers; unsigned only");
        if (((dt() & DataType::Type) == DataType::UInt16) && !dt.is_signed())
          throw Exception ("MGH file format does not support unsigned 16-bit integers; signed only");
        if (((dt() & DataType::Type) == DataType::UInt32) && !dt.is_signed())
          throw Exception ("MGH file format does not support unsigned 32-bit integers; signed only");
        if ((dt() & DataType::Type) == DataType::UInt64)
          throw Exception ("MGH file format does not support 64-bit integer data");
        if ((dt() & DataType::Type) == DataType::Float64)
          throw Exception ("MGH file format does not support 64-bit floating-point data");
        int32_t type;
        switch (dt()) {
          case DataType::UInt8: type = MGH_TYPE_UCHAR; break;
          case DataType::Int16:   case DataType::Int16BE:   case DataType::Int16LE:   type = MGH_TYPE_SHORT; break;
          case DataType::Int32:   case DataType::Int32BE:   case DataType::Int32LE:   type = MGH_TYPE_INT;   break;
          case DataType::Float32: case DataType::Float32BE: case DataType::Float32LE: type = MGH_TYPE_FLOAT; break;
          default: throw Exception ("Error in MGH file format data type parsing");
        }
        Raw::store<int32_t> (type, &MGHH.type, is_BE);

        Raw::store<int32_t> (0, &MGHH.dof, is_BE);
        Raw::store<int16_t> (1, &MGHH.goodRASFlag, is_BE);
        Raw::store<float> (H.spacing (axes[0]), &MGHH.spacing_x, is_BE);
        Raw::store<float> (H.spacing (axes[1]), &MGHH.spacing_y, is_BE);
        Raw::store<float> (H.spacing (axes[2]), &MGHH.spacing_z, is_BE);

        //const Math::Matrix<float>& M (H.transform());
        Raw::store<float> (M(0,0), &MGHH.x_r, is_BE); 
        Raw::store<float> (M(0,1), &MGHH.y_r, is_BE); 
        Raw::store<float> (M(0,2), &MGHH.z_r, is_BE);

        Raw::store<float> (M(1,0), &MGHH.x_a, is_BE); 
        Raw::store<float> (M(1,1), &MGHH.y_a, is_BE); 
        Raw::store<float> (M(1,2), &MGHH.z_a, is_BE);
        
        Raw::store<float> (M(2,0), &MGHH.x_s, is_BE); 
        Raw::store<float> (M(2,1), &MGHH.y_s, is_BE); 
        Raw::store<float> (M(2,2), &MGHH.z_s, is_BE);

        for (size_t i = 0; i != 3; ++i) {
          default_type offset = M(i, 3);
          for (size_t j = 0; j != 3; ++j)
            offset += 0.5 * H.size(axes[j]) * H.spacing(axes[j]) * M(i,j);
          switch (i) {
            case 0: Raw::store<float> (offset, &MGHH.c_r, is_BE); break;
            case 1: Raw::store<float> (offset, &MGHH.c_a, is_BE); break;
            case 2: Raw::store<float> (offset, &MGHH.c_s, is_BE); break;
          }
        }

      }




      void write_other (mgh_other& MGHO, const Header& H)
      {

        bool is_BE = H.datatype().is_big_endian();

        const auto comments = H.keyval().find("comments");
        if (comments != H.keyval().end()) {
          for (const auto i : split_lines (comments->second)) {
            const std::string key = i.substr (0, i.find_first_of (':'));
            if (key == "TR")
              Raw::store<float> (to<float> (i.substr (3)), &MGHO.tr, is_BE);
            else if (key == "Flip")
              Raw::store<float> (to<float> (i.substr (5)), &MGHO.flip_angle, is_BE);
            else if (key == "TE")
              Raw::store<float> (to<float> (i.substr (3)), &MGHO.te, is_BE);
            else if (key == "TI")
              Raw::store<float> (to<float> (i.substr (3)), &MGHO.ti, is_BE);
            else
              MGHO.tags.push_back (i);
          }
        }
        Raw::store<float> (0.0, &MGHO.fov, is_BE);

      }




      void write_other_to_file (const std::string& path, const mgh_other& MGHO)
      {
        File::OFStream out (path, std::ios_base::out | std::ios_base::app);
        out.write ((char*) &MGHO, 5 * sizeof (float));
        for (vector<std::string>::const_iterator i = MGHO.tags.begin(); i != MGHO.tags.end(); ++i)
          out.write (i->c_str(), i->size() + 1);
        out.close();
      }




    }
  }
}


