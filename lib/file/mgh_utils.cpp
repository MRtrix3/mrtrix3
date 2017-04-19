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

#include <cctype>

#include "header.h"
#include "file/ofstream.h"
#include "file/mgh_utils.h"
#include "file/nifti1_utils.h"

namespace MR
{
  namespace File
  {
    namespace MGH
    {



      namespace {

        std::string tag_ID_to_string (const int32_t tag)
        {
          switch (tag) {
            case 1: return "MGH_TAG_OLD_COLORTABLE";
            case 2: return "MGH_TAG_OLD_USEREALRAS";
            case 3: return "MGH_TAG_CMDLINE";
            case 4: return "MGH_TAG_USEREALRAS";
            case 5: return "MGH_TAG_COLORTABLE";

            case 10: return "MGH_TAG_GCAMORPH_GEOM";
            case 11: return "MGH_TAG_GCAMORPH_TYPE";
            case 12: return "MGH_TAG_GCAMORPH_LABELS";

            case 20: return "MGH_TAG_OLD_SURF_GEOM";
            case 21: return "MGH_TAG_SURF_GEOM";

            case 30: return "MGH_TAG_OLD_MGH_XFORM";
            case 31: return "MGH_TAG_MGH_XFORM";
            case 32: return "MGH_TAG_GROUP_AVG_SURFACE_AREA";
            case 33: return "MGH_TAG_AUTO_ALIGN";

            case 40: return "MGH_TAG_SCALAR_DOUBLE";
            case 41: return "MGH_TAG_PEDIR";
            case 42: return "MGH_TAG_MRI_FRAME";
            case 43: return "MGH_TAG_FIELDSTRENGTH";

            default: break;
          }
          return "MGH_TAG_" + str(tag);
        }



        int32_t string_to_tag_ID (const std::string& key)
        {
          if (key.compare (0, 8, "MGH_TAG_") == 0) {

            auto id = key.substr (8);

            if (id == "OLD_COLORTABLE") return 1;
            if (id == "OLD_USEREALRAS") return 2;
            if (id == "CMDLINE") return 3;
            if (id == "USEREALRAS") return 4;
            if (id == "COLORTABLE") return 5;

            if (id == "GCAMORPH_GEOM") return 10;
            if (id == "GCAMORPH_TYPE") return 11;
            if (id == "GCAMORPH_LABELS") return 12;

            if (id == "OLD_SURF_GEOM") return 20;
            if (id == "SURF_GEOM") return 21;

            if (id == "OLD_MGH_XFORM") return 30;
            if (id == "MGH_XFORM") return 31;
            if (id == "GROUP_AVG_SURFACE_AREA") return 32;
            if (id == "AUTO_ALIGN") return 33;

            if (id == "SCALAR_DOUBLE") return 40;
            if (id == "PEDIR") return 41;
            if (id == "MRI_FRAME") return 42;
            if (id == "FIELDSTRENGTH") return 43;
          }

          return 0;
        }

      }








      void read_header (Header& H, const mgh_header& MGHH)
      {
        if (Raw::fetch_BE<int32_t> (&MGHH.version) != 1)
          throw Exception ("image \"" + H.name() + "\" is not in MGH format (version != 1)");

        const size_t ndim = (ByteOrder::BE (MGHH.nframes) > 1) ? 4 : 3;
        H.ndim() = ndim;
        H.size (0) = ByteOrder::BE (MGHH.width);
        H.size (1) = ByteOrder::BE (MGHH.height);
        H.size (2) = ByteOrder::BE (MGHH.depth);
        if (ndim == 4)
          H.size (3) = ByteOrder::BE (MGHH.nframes);

        H.spacing (0) = ByteOrder::BE (MGHH.spacing_x);
        H.spacing (1) = ByteOrder::BE (MGHH.spacing_y);
        H.spacing (2) = ByteOrder::BE (MGHH.spacing_z);

        for (size_t i = 0; i != ndim; ++i)
          H.stride (i) = i + 1;

        DataType dtype;
        int32_t type = ByteOrder::BE (MGHH.type);
        switch (type) {
          case MGH_TYPE_UCHAR: dtype = DataType::UInt8;     break;
          case MGH_TYPE_SHORT: dtype = DataType::Int16BE;   break;
          case MGH_TYPE_INT:   dtype = DataType::Int32BE;   break;
          case MGH_TYPE_FLOAT: dtype = DataType::Float32BE; break;
          default: throw Exception ("unknown data type for MGH image \"" + H.name() + "\" (" + str (type) + ")");
        }
        H.datatype() = dtype;
        H.reset_intensity_scaling();

        transform_type& M (H.transform());

        const int16_t RAS = ByteOrder::BE (MGHH.goodRASFlag);
        if (RAS) {

          M(0,0) = ByteOrder::BE (MGHH.x_r);
          M(0,1) = ByteOrder::BE (MGHH.y_r);
          M(0,2) = ByteOrder::BE (MGHH.z_r);

          M(1,0) = ByteOrder::BE (MGHH.x_a);
          M(1,1) = ByteOrder::BE (MGHH.y_a);
          M(1,2) = ByteOrder::BE (MGHH.z_a);

          M(2,0) = ByteOrder::BE (MGHH.x_s);
          M(2,1) = ByteOrder::BE (MGHH.y_s);
          M(2,2) = ByteOrder::BE (MGHH.z_s);

          M(0,3) = ByteOrder::BE (MGHH.c_r);
          M(1,3) = ByteOrder::BE (MGHH.c_a);
          M(2,3) = ByteOrder::BE (MGHH.c_s);

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

      }







      void read_other (Header& H, const mgh_other& MGHO)
      {
        if (ByteOrder::BE (MGHO.tr) != 0.0f)
          add_line (H.keyval()["comments"], "TR: "   + str (ByteOrder::BE (MGHO.tr), 6) + "ms");
        if (ByteOrder::BE (MGHO.flip_angle) != 0.0f) // Radians in MGHO -> degrees in header
          add_line (H.keyval()["comments"], "Flip: " + str (ByteOrder::BE (MGHO.flip_angle) * 180.0 / Math::pi, 6) + "deg");
        if (ByteOrder::BE (MGHO.te) != 0.0f)
          add_line (H.keyval()["comments"], "TE: "   + str (ByteOrder::BE (MGHO.te), 6) + "ms");
        if (ByteOrder::BE (MGHO.ti) != 0.0f)
          add_line (H.keyval()["comments"], "TI: "   + str (ByteOrder::BE (MGHO.ti), 6) + "ms");
      }








      void read_tag (Header& H, const mgh_tag& tag)
      {
        if (tag.content.size())
          add_line (H.keyval()["comments"], tag_ID_to_string (ByteOrder::BE (tag.id)) + ": " + tag.content);
      }







      void write_header (mgh_header& MGHH, const Header& H)
      {
        const size_t ndim = H.ndim();
        if (ndim > 4)
          throw Exception ("MGH file format does not support images of more than 4 dimensions");

        std::vector<size_t> axes;
        auto M = File::NIfTI::adjust_transform (H, axes);

        Raw::store_BE<int32_t> (1, &MGHH.version);
        Raw::store_BE<int32_t> (H.size (axes[0]), &MGHH.width);
        Raw::store_BE<int32_t> ((ndim > 1) ? H.size (axes[1]) : 1, &MGHH.height);
        Raw::store_BE<int32_t> ((ndim > 2) ? H.size (axes[2]) : 1, &MGHH.depth);
        Raw::store_BE<int32_t> ((ndim > 3) ? H.size (3) : 1, &MGHH.nframes);

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
        Raw::store_BE<int32_t> (type, &MGHH.type);

        Raw::store_BE<int32_t> (0, &MGHH.dof);
        Raw::store_BE<int16_t> (1, &MGHH.goodRASFlag);
        Raw::store_BE<float32> (H.spacing (axes[0]), &MGHH.spacing_x);
        Raw::store_BE<float32> (H.spacing (axes[1]), &MGHH.spacing_y);
        Raw::store_BE<float32> (H.spacing (axes[2]), &MGHH.spacing_z);

        //const Math::Matrix<float>& M (H.transform());
        Raw::store_BE<float32> (M(0,0), &MGHH.x_r);
        Raw::store_BE<float32> (M(0,1), &MGHH.y_r);
        Raw::store_BE<float32> (M(0,2), &MGHH.z_r);

        Raw::store_BE<float32> (M(1,0), &MGHH.x_a);
        Raw::store_BE<float32> (M(1,1), &MGHH.y_a);
        Raw::store_BE<float32> (M(1,2), &MGHH.z_a);

        Raw::store_BE<float32> (M(2,0), &MGHH.x_s);
        Raw::store_BE<float32> (M(2,1), &MGHH.y_s);
        Raw::store_BE<float32> (M(2,2), &MGHH.z_s);

        for (size_t i = 0; i != 3; ++i) {
          default_type offset = M(i, 3);
          for (size_t j = 0; j != 3; ++j)
            offset += 0.5 * H.size(axes[j]) * H.spacing(axes[j]) * M(i,j);
          switch (i) {
            case 0: Raw::store_BE<float32> (offset, &MGHH.c_r); break;
            case 1: Raw::store_BE<float32> (offset, &MGHH.c_a); break;
            case 2: Raw::store_BE<float32> (offset, &MGHH.c_s); break;
          }
        }

      }








      void write_other (mgh_other& MGHO, const Header& H)
      {
        MGHO.tags.clear();

        const auto comments = H.keyval().find("comments");
        if (comments != H.keyval().end()) {
          for (const auto i : split_lines (comments->second)) {
            auto pos = i.find_first_of (": ");
            const std::string key = i.substr (0, pos);
            const std::string value = i.substr (pos+2);
            if (key == "TR")
              Raw::store_BE<float32> (to<float32> (value), &MGHO.tr);
            else if (key == "Flip") // Degrees in header -> radians in MGHO
              Raw::store_BE<float32> (to<float32> (value) * Math::pi / 180.0, &MGHO.flip_angle);
            else if (key == "TE")
              Raw::store_BE<float32> (to<float32> (value), &MGHO.te);
            else if (key == "TI")
              Raw::store_BE<float32> (to<float32> (value), &MGHO.ti);
            else {
              auto id = string_to_tag_ID (key);
              if (id) {
                mgh_tag tag;
                tag.id = ByteOrder::BE (id);
                tag.size = ByteOrder::BE (value.size());
                tag.content = value;
                MGHO.tags.push_back (tag);
              }
            }
          }
        }

        Raw::store_BE<float32> (0.0, &MGHO.fov);
      }

    }
  }
}


