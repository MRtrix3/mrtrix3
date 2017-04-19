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

#ifndef __file_mgh_utils_h__
#define __file_mgh_utils_h__

#include "raw.h"
#include "header.h"
#include "file/nifti1_utils.h"

#define MGH_HEADER_SIZE 90
#define MGH_DATA_OFFSET 284
#define MGH_TYPE_UCHAR 0
#define MGH_TYPE_SHORT 4
#define MGH_TYPE_INT   1
#define MGH_TYPE_FLOAT 3

namespace MR
{
  class Header;

  namespace File
  {
    namespace MGH
    {

      int32_t string_to_tag_ID (const std::string& key);
      std::string tag_ID_to_string (const int32_t tag);


      template <typename ValueType, class Input>
        inline ValueType fetch (Input& in) 
        {
          ValueType value;
          in.read (reinterpret_cast<char*> (&value), sizeof (ValueType));
          if (in.eof())
            throw int(1);
          value = ByteOrder::BE (value);
          return value;
        }

      template <typename ValueType, class Output>
        inline void store (ValueType value, Output& out) 
        {
          value = ByteOrder::BE (value);
          out.write (reinterpret_cast<char*> (&value), sizeof (ValueType));
        }



      inline bool check (Header& H, size_t num_axes) 
      {
        if (num_axes < 3) throw Exception ("cannot create MGH image with less than 3 dimensions");
        if (num_axes > 4) throw Exception ("cannot create MGH image with more than 4 dimensions");
        H.ndim() = num_axes;

        if (H.datatype().is_complex())
          throw Exception ("MGH file format does not support complex types");

        switch (H.datatype()() & ( DataType::Type | DataType::Signed )) {
          case DataType::Bit: 
          case DataType::Int8: H.datatype() = DataType::UInt8; break;
          case DataType::UInt16: H.datatype() = DataType::Int16BE; break;
          case DataType::UInt32: 
          case DataType::Int64:
          case DataType::UInt64: H.datatype() = DataType::Int32BE; break;
          case DataType::Float64: H.datatype() = DataType::Float32BE; break;
        }

        if (H.datatype().bytes() > 1) {
          H.datatype().set_flag (DataType::BigEndian);
          H.datatype().unset_flag (DataType::LittleEndian);
        }

        return true;
      }





      template <class Input>
        void read_header (Header& H, Input& in)
        {
          auto version = fetch<int32_t> (in); 
          if (version != 1)
            throw Exception ("image \"" + H.name() + "\" is not in MGH format (version != 1)");

          auto width = fetch<int32_t> (in);
          auto height = fetch<int32_t> (in);
          auto depth = fetch<int32_t> (in);
          auto nframes = fetch<int32_t> (in);
          auto type = fetch<int32_t> (in);
          fetch<int32_t> (in); // dof - ignored
          auto RAS = fetch<int16_t> (in); 


          const size_t ndim = (nframes > 1) ? 4 : 3;
          H.ndim() = ndim;

          H.size (0) = width;
          H.size (1) = height;
          H.size (2) = depth;
          if (ndim == 4)
            H.size (3) = nframes;

          H.spacing (0) = fetch<float32> (in);
          H.spacing (1) = fetch<float32> (in);
          H.spacing (2) = fetch<float32> (in);

          for (size_t i = 0; i != ndim; ++i)
            H.stride (i) = i + 1;

          DataType dtype;
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

          if (RAS) {

            M(0,0) = fetch<float32> (in);
            M(1,0) = fetch<float32> (in);
            M(2,0) = fetch<float32> (in);

            M(0,1) = fetch<float32> (in);
            M(1,1) = fetch<float32> (in);
            M(2,1) = fetch<float32> (in);

            M(0,2) = fetch<float32> (in);
            M(1,2) = fetch<float32> (in);
            M(2,2) = fetch<float32> (in);

            M(0,3) = fetch<float32> (in);
            M(1,3) = fetch<float32> (in);
            M(2,3) = fetch<float32> (in);

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







      template <class Input>
        void read_other (Header& H, Input& in)
        {
          try {
            if (auto tr = fetch<float32> (in))
              add_line (H.keyval()["comments"], "TR: "   + str (tr, 6) + "ms");
            if (auto flip_angle = fetch<float32> (in)) // Radians in MGHO -> degrees in header
              add_line (H.keyval()["comments"], "Flip: " + str (flip_angle * 180.0 / Math::pi, 6) + "deg");
            if (auto te = fetch<float32> (in)) 
              add_line (H.keyval()["comments"], "TE: "   + str (te, 6) + "ms");
            if (auto ti = fetch<float32> (in)) 
              add_line (H.keyval()["comments"], "TI: "   + str (ti, 6) + "ms");

            fetch<float32> (in); // fov - ignored

            do {
              auto id = fetch<int32_t> (in);
              auto size = fetch<int64_t> (in);
              std::string content (size, '\0');
              in.read (reinterpret_cast<char*> (&content[0]), size);
              if (content.size())
                add_line (H.keyval()["comments"], tag_ID_to_string (id) + ": " + content);
            } while (!in.eof());

          } catch (int) { }
        }







      template <class Output>
        void write_header (const Header& H, Output& out)
        {
          const size_t ndim = H.ndim();
          if (ndim > 4)
            throw Exception ("MGH file format does not support images of more than 4 dimensions");

          std::vector<size_t> axes;
          auto M = File::NIfTI::adjust_transform (H, axes);

          store<int32_t> (1, out); // version
          store<int32_t> (H.size (axes[0]), out); // width
          store<int32_t> ((ndim > 1) ? H.size (axes[1]) : 1, out); // height
          store<int32_t> ((ndim > 2) ? H.size (axes[2]) : 1, out); // depth
          store<int32_t> ((ndim > 3) ? H.size (3) : 1, out); // nframes

          int32_t type;
          switch (H.datatype()()) {
            case DataType::UInt8:     type = MGH_TYPE_UCHAR; break;
            case DataType::Int16BE:   type = MGH_TYPE_SHORT; break;
            case DataType::Int32BE:   type = MGH_TYPE_INT;   break;
            case DataType::Float32BE: type = MGH_TYPE_FLOAT; break;
            default: throw Exception ("Error in MGH file format data type parsing: invalid datatype");
          }
          store<int32_t> (type, out); // type
          store<int32_t> (0, out); // dof
          store<int16_t> (1, out); // goodRASFlag

          store<float32> (H.spacing (axes[0]), out); // spacing_x
          store<float32> (H.spacing (axes[1]), out); // spacing_y
          store<float32> (H.spacing (axes[2]), out); // spacing_z

          float32 c[3] = { 0.0f, 0.0f, 0.0f };
          for (size_t i = 0; i != 3; ++i) {
            default_type offset = M(i, 3);
            for (size_t j = 0; j != 3; ++j)
              offset += 0.5 * H.size(axes[j]) * H.spacing(axes[j]) * M(i,j);
            switch (i) {
              case 0: c[0] = offset; break; 
              case 1: c[1] = offset; break; 
              case 2: c[2] = offset; break;
            }
          }

          store<float32> (M(0,0), out); // x_r
          store<float32> (M(1,0), out); // x_a
          store<float32> (M(2,0), out); // x_s

          store<float32> (M(0,1), out); // y_r
          store<float32> (M(1,1), out); // y_a
          store<float32> (M(2,1), out); // y_s

          store<float32> (M(0,2), out); // z_r
          store<float32> (M(1,2), out); // z_a
          store<float32> (M(2,2), out); // z_s

          store<float32> (c[0], out); // c_r
          store<float32> (c[1], out); // c_a
          store<float32> (c[2], out); // c_s
        }








      template <class Output>
        void write_other (const Header& H, Output& out)
        {
          struct Tag
          {
            int32_t id;
            std::string content;
          };

          float32 tr = 0.0f;         /*!< milliseconds */
          float32 flip_angle = 0.0f; /*!< radians */
          float32 te = 0.0f;         /*!< milliseconds */
          float32 ti = 0.0f;         /*!< milliseconds */
          float32 fov = 0.0f;        /*!< IGNORE THIS FIELD (data is inconsistent) */
          std::vector<Tag> tags; /*!< variable length char strings */

          const auto comments = H.keyval().find("comments");
          if (comments != H.keyval().end()) {
            for (const auto i : split_lines (comments->second)) {
              auto pos = i.find_first_of (": ");
              const std::string key = i.substr (0, pos);
              const std::string value = i.substr (pos+2);
              if (key == "TR") tr = to<float32> (value); 
              else if (key == "Flip") flip_angle = to<float32> (value) * Math::pi / 180.0;
              else if (key == "TE") te = to<float32> (value);
              else if (key == "TI") ti = to<float32> (value);
              else {
                auto id = string_to_tag_ID (key);
                if (id) {
                  Tag tag;
                  tag.id = id;
                  tag.content = value;
                  tags.push_back (tag);
                }
              }
            }
          }

          store<float32> (tr, out);
          store<float32> (flip_angle, out);
          store<float32> (te, out);
          store<float32> (ti, out);
          store<float32> (fov, out);
          for (const auto& tag : tags) {
            store<int32_t> (tag.id, out);
            store<int64_t> (tag.content.size(), out);
            out.write (tag.content.c_str(), tag.content.size());
          }
        }

    }
  }
}

#endif

