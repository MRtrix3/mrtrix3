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

#ifndef __file_mgh_h__
#define __file_mgh_h__

#include "raw.h"
#include "header.h"
#include "file/nifti_utils.h"

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

        switch (H.datatype()() & (DataType::Type | DataType::Signed)) {
          case DataType::Bit: 
          case DataType::UInt8:
            H.datatype() = DataType::UInt8; break;
          case DataType::Int8:
          case DataType::UInt16:
          case DataType::Int16:
            H.datatype() = DataType::Int16BE; break;
          case DataType::UInt32:
          case DataType::Int32:
          case DataType::UInt64:
          case DataType::Int64:
            H.datatype() = DataType::Int32BE; break;
          case DataType::Float32:
          case DataType::Float64:
            H.datatype() = DataType::Float32BE; break;
          default: throw Exception ("Unsupported data type for MGH format (" + std::string(H.datatype().specifier()) + ")");
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
            // FreeSurfer file mriio.c indicates that reading the first five
            //   single-precision floating-point numbers should terminate as
            //   soon as one of them is null...
            // However this is inconsistent with FreeSurfer's own MGH read
            //   code, which always writes five floats regardless of their values...
            if (auto tr = fetch<float32> (in))
              H.keyval()["MGH_TR"] = str(tr, 6);
            if (auto flip_angle = fetch<float32> (in)) // Radians in MGHO -> degrees in header
              H.keyval()["MGH_flip"] = str(flip_angle * 180.0 / Math::pi, 6);
            if (auto te = fetch<float32> (in))
              H.keyval()["MGH_TE"] = str(te, 6);
            if (auto ti = fetch<float32> (in))
              H.keyval()["MGH_TI"] = str(ti, 6);

            fetch<float32> (in); // fov - ignored

            do {
              auto id = fetch<int32_t> (in);
              int64_t size;
              if (id == 30) // MGH_TAG_OLD_MGH_XFORM
                size = fetch<int32_t> (in) - 1;
              else if (id == 20 || id == 4 || id == 1) // MGH_TAG_OLD_SURF_GEOM, MGH_TAG_OLD_USEREALRAS, MGH_TAG_OLD_COLORTABLE
                size = 0;
              else
                size = fetch<int64_t> (in);
              std::string content (size+1, '\0');
              in.read (const_cast<char*> (content.data()), size);
              if (content.size() > 1) {
                if (id == 3) { // MGH_TAG_CMDLINE
                  for (auto line : split_lines (content))
                    add_line (H.keyval()["command_history"], line);
                } else {
                  H.keyval()[tag_ID_to_string(id)] = content;
                }
              }
            } while (!in.eof());

          } catch (int) { }
        }







      template <class Output>
        void write_header (const Header& H, Output& out)
        {
          const size_t ndim = H.ndim();
          if (ndim > 4)
            throw Exception ("MGH file format does not support images of more than 4 dimensions");

          vector<size_t> axes;
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
            default: throw Exception ("Error in MGH file format header write: invalid datatype (" + std::string(H.datatype().specifier()) + ")");
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
          class Tag
          {
            public:
              Tag() : id (0), content() { }
              Tag (const int32_t i, const std::string& s) : id (i), content (s) { }
              void set (const int32_t i, const std::string& s) { id = i; content = s; }
              int32_t id;
              std::string content;
          };

          float32 tr = 0.0f;         /*!< milliseconds */
          float32 flip_angle = 0.0f; /*!< radians */
          float32 te = 0.0f;         /*!< milliseconds */
          float32 ti = 0.0f;         /*!< milliseconds */
          float32 fov = 0.0f;        /*!< IGNORE THIS FIELD (data is inconsistent) */
          Tag transform_tag;
          vector<Tag> tags;          /*!< variable length char strings */
          Tag cmdline_tag;

          for (auto entry : H.keyval()) {
            if (entry.first == "command_history")
              cmdline_tag.set (3, entry.second); // MGH_TAG_CMDLINE
            else if (entry.first.size() < 5 || entry.first.substr(0, 4) != "MGH_")
              continue;
            if (entry.first == "MGH_TR") {
              tr = to<float32> (entry.second);
            } else if (entry.first == "MGH_flip") {
              flip_angle = to<float32> (entry.second) * Math::pi / 180.0;
            } else if (entry.first == "MGH_TE") {
              te = to<float32> (entry.second);
            } else if (entry.first == "MGH_TI") {
              ti = to<float32> (entry.second);
            } else {
              const auto id = string_to_tag_ID (entry.first);
              if (id == 31) // MGH_TAG_MGH_XFORM
                transform_tag.set (31, entry.second);
              else if (id)
                tags.push_back (Tag (id, entry.second));
            }
          }

          store<float32> (tr, out);
          store<float32> (flip_angle, out);
          store<float32> (te, out);
          store<float32> (ti, out);
          store<float32> (fov, out);
          if (transform_tag.content.size()) {
            store<int32_t> (transform_tag.id, out);
            store<int64_t> (transform_tag.content.size(), out);
            out.write (transform_tag.content.c_str(), transform_tag.content.size());
          }
          // FreeSurfer appears to write all other tag data in a single batch...
          //   Not sure how it is prepared though.
          // Nevertheless, their referenced code seems to write a single size field,
          //   then a large batch of "tag data" all together; which is different to
          //   what we're doing here. What I don't understand is how, if the size
          //   of this "tag data" is the next thing written (and is only written if
          //   non-zero), a reader function can distinguish this size entry from a
          //   tag ID.
          for (const auto& tag : tags) {
            store<int32_t> (tag.id, out);
            store<int64_t> (tag.content.size(), out);
            out.write (tag.content.c_str(), tag.content.size());
          }
          if (cmdline_tag.content.size()) {
            store<int32_t> (cmdline_tag.id, out);
            store<int64_t> (cmdline_tag.content.size(), out);
            out.write (cmdline_tag.content.c_str(), cmdline_tag.content.size());
          }
        }

    }
  }
}

#endif

