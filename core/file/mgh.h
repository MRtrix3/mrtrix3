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


#ifndef __file_mgh_h__
#define __file_mgh_h__

#include <sstream>

#include "header.h"
#include "raw.h"
#include "file/gz.h"
#include "file/nifti_utils.h"

#define MGH_HEADER_SIZE 90
#define MGH_DATA_OFFSET 284

#define MGH_TYPE_UCHAR 0
#define MGH_TYPE_SHORT 4
#define MGH_TYPE_INT   1
#define MGH_TYPE_FLOAT 3


#define MGH_TAG_OLD_COLORTABLE 1
#define MGH_TAG_OLD_USEREALRAS 2
#define MGH_TAG_CMDLINE 3
#define MGH_TAG_USEREALRAS 4
#define MGH_TAG_COLORTABLE 5

#define MGH_TAG_GCAMORPH_GEOM 10
#define MGH_TAG_GCAMORPH_TYPE 11
#define MGH_TAG_GCAMORPH_LABELS 12

#define MGH_TAG_OLD_SURF_GEOM 20
#define MGH_TAG_SURF_GEOM 21

#define MGH_TAG_OLD_MGH_XFORM 30
#define MGH_TAG_MGH_XFORM 31
#define MGH_TAG_GROUP_AVG_SURFACE_AREA 32
#define MGH_TAG_AUTO_ALIGN 33

#define MGH_TAG_SCALAR_DOUBLE 40
#define MGH_TAG_PEDIR 41
#define MGH_TAG_MRI_FRAME 42
#define MGH_TAG_FIELDSTRENGTH 43

#define MGH_STRLEN 1024
#define MGH_MATRIX_STRLEN (4*4*100)

#define MGH_FRAME_TYPE_ORIGINAL 0
#define MGH_FRAME_TYPE_DIFFUSION_AUGMENTED 1


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




      typedef struct
      {
        int32_t type;             // code for what is stored in this frame
        float32 TE;               // echo time
        float32 TR;               // recovery time
        float32 flip;             // flip angle
        float32 TI;               // time-to-inversion
        float32 TD;               // delay time
        // Note that in the import / export code, TM is loaded with a float32 here, even though
        //   TM actually appears in the augmented diffusion section
        int32_t sequence_type;    // see SEQUENCE* constants
        float32 echo_spacing;
        float32 echo_train_len;   // length of the echo train
        float32 read_dir[3];      // read-out direction in RAS coords
        float32 pe_dir[3];        // phase-encode direction in RAS coords
        float32 slice_dir[3];     // slice direction in RAS coords
        int32_t label;            // index into CLUT
        char    name[MGH_STRLEN]; // human-readable description of frame contents
        int32_t dof;              // for stat maps (e.g. # of subjects)

        Eigen::Matrix<default_type, 4, 4>* m_ras2vox;
        float32 thresh;
        int32_t units;            // e.g. UNITS_PPM,  UNITS_RAD_PER_SEC, ...

        // for Herr Dr. Prof. Dr. Dr. Witzel
        // directions: maybe best in both reference frames
        // or just 3 coordinates and a switch which frame it is?
        float64 DX, DY, DZ, DR, DP, DS;

        // B-value
        float64 bvalue;

        // Mixing time
        float64 TM;

        // What kind of diffusion scan is this (can be an enum)
        // stejskal-tanner,trse,steam etc....
        int64_t diffusion_type;

        // Gradient values
        int64_t D1_ramp, D1_flat; float64 D1_amp;
        int64_t D2_ramp, D2_flat; float64 D2_amp;
        int64_t D3_ramp, D3_flat; float64 D3_amp;
        int64_t D4_ramp, D4_flat; float64 D4_amp;
      } mri_frame ;





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
          // First, encapsulate some functionalities for cleanliness...
          // TODO These may well be shared with the FreeSurfer surface file formats


          auto read_matrix = [] (Input& in)
          {
            char buffer[MGH_MATRIX_STRLEN];
            in.read (buffer, MGH_MATRIX_STRLEN);
            // There's also a string before the 16 floating-point values, the FreeSurfer code
            //   discards it immediately
            char ch[100];
            Eigen::Matrix<default_type, 4, 4> M;
            sscanf (buffer, "%s %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf",
                    ch,
                    &M(0,0), &M(0,1), &M(0,2), &M(0,3),
                    &M(1,0), &M(1,1), &M(1,2), &M(1,3),
                    &M(2,0), &M(2,1), &M(2,2), &M(2,3),
                    &M(3,0), &M(3,1), &M(3,2), &M(3,3));
            return M;
          };



          auto read_mri_frame = [&] (Input& in, const int64_t len)
          {
            const int64_t fstart = in.tellg();
            const size_t nframes = H.ndim() == 4 ? H.size(3) : 1;
            std::string table;
            Eigen::IOFormat format (Eigen::StreamPrecision, Eigen::DontAlignCols, " ", " ", "", "", "", "");
            for (size_t frame_index = 0; frame_index != nframes; ++frame_index) {
              mri_frame frame;
              frame.type = fetch<int32_t> (in);
              frame.TE = fetch<float32> (in);
              frame.TR = fetch<float32> (in);
              frame.flip = fetch<float32> (in);
              frame.TI = fetch<float32> (in);
              frame.TD = fetch<float32> (in);
              // FreeSurfer loads TM with a float32 here, even though TM is in the diffusion section and is a float64...
              fetch<float32> (in);
              frame.TM = 0.0;
              frame.sequence_type = fetch<int32_t> (in);
              frame.echo_spacing = fetch<float32> (in);
              frame.echo_train_len = fetch<float32> (in);
              for (size_t i = 0; i != 3; i++)
                frame.read_dir[i] = fetch<float32> (in);
              for (size_t i = 0; i != 3; i++)
                frame.pe_dir[i] = fetch<float32> (in);
              for (size_t i = 0; i != 3; i++)
                frame.slice_dir[i] = fetch<float32> (in);
              frame.label = fetch<int32_t> (in);
              in.read (frame.name, MGH_STRLEN);
              frame.dof = fetch<int32_t> (in);
              // Single matrix is read here; use the same function as that for importing MGH_TAG_AUTO_ALIGN
              fetch<int32_t> (in); // Skip the tag ID absorbed by znzTAGreadStart()
              fetch<int64_t> (in); // Also skip the length
              frame.m_ras2vox = new Eigen::Matrix<default_type, 4, 4> (read_matrix (in));
              frame.thresh = fetch<float32> (in);
              frame.units = fetch<int32_t> (in);

              std::string line = str(frame.type) + "," + str(frame.TE) + "," + str(frame.TR) + "," + str(frame.flip) + ","
                               + str(frame.TI) + "," + str(frame.TD) + ","
                               + str(frame.sequence_type) + "," + str(frame.echo_spacing) + "," + str(frame.echo_train_len) + ","
                               + str(frame.read_dir[0]) + "," + str(frame.read_dir[1]) + "," + str(frame.read_dir[2]) + ","
                               + str(frame.pe_dir[0]) + "," + str(frame.pe_dir[1]) + "," + str(frame.pe_dir[2]) + ","
                               + str(frame.slice_dir[0]) + "," + str(frame.slice_dir[1]) + "," + str(frame.slice_dir[2]) + ","
                               + str(frame.label) + "," + frame.name + "," + str(frame.dof) + ","
                               + str(frame.m_ras2vox->format (format)) + ","
                               + str(frame.thresh) + "," + str(frame.units);

              delete frame.m_ras2vox; frame.m_ras2vox = nullptr;

              if (frame.type == MGH_FRAME_TYPE_DIFFUSION_AUGMENTED) {
                frame.DX = fetch<float64> (in);
                frame.DY = fetch<float64> (in);
                frame.DZ = fetch<float64> (in);
                frame.DR = fetch<float64> (in);
                frame.DP = fetch<float64> (in);
                frame.DS = fetch<float64> (in);
                frame.bvalue = fetch<float64> (in);
                frame.TM = fetch<float64> (in);

                frame.D1_ramp = fetch<int64_t> (in);
                frame.D1_flat = fetch<int64_t> (in);
                frame.D1_amp = fetch<float64> (in);

                frame.D2_ramp = fetch<int64_t> (in);
                frame.D2_flat = fetch<int64_t> (in);
                frame.D2_amp = fetch<float64> (in);

                frame.D3_ramp = fetch<int64_t> (in);
                frame.D3_flat = fetch<int64_t> (in);
                frame.D3_amp = fetch<float64> (in);

                frame.D4_ramp = fetch<int64_t> (in);
                frame.D4_flat = fetch<int64_t> (in);
                frame.D4_amp = fetch<float64> (in);

                line += "," + str(frame.DX) + "," + str(frame.DY) + "," + str(frame.DZ) + ","
                            + str(frame.DR) + "," + str(frame.DP) + "," + str(frame.DS) + ","
                            + str(frame.bvalue) + "," + str(frame.TM) + ","
                            + str(frame.D1_ramp) + "," + str(frame.D1_flat) + "," + str(frame.D1_amp) + ","
                            + str(frame.D2_ramp) + "," + str(frame.D2_flat) + "," + str(frame.D2_amp) + ","
                            + str(frame.D3_ramp) + "," + str(frame.D3_flat) + "," + str(frame.D3_amp) + ","
                            + str(frame.D4_ramp) + "," + str(frame.D4_flat) + "," + str(frame.D4_amp);
              }

              add_line (table, line);
            }

            // Test to see if the correct amount of data has been read
            //   (the expected length of the field is reported as part of the tag)
            const int64_t fend = in.tellg();
            const int64_t empty_space_len = len - (fend - fstart);
            if (empty_space_len > 0) {
              char buffer[empty_space_len];
              in.read (buffer, empty_space_len);
            }

            return table;
          };



          auto read_colourtable_V1 = [&] (Input& in, const int32_t nentries)
          {
            if (!nentries)
              throw Exception ("Error reading colour table from file \"" + H.name() + "\": No entries");
            std::string table;
            const int32_t filename_length = fetch<int32_t> (in);
            std::string filename (filename_length + 1, '\0');
            in.read (const_cast<char*> (filename.data()), filename_length);
            for (size_t structure = 0; structure != nentries; ++structure) {
              const int32_t structurename_length = fetch<int32_t> (in);
              if (structurename_length < 0)
                throw Exception ("Error reading colour table from file \"" + H.name() + "\": Negative structure name length");
              std::string structurename (structurename_length + 1, '\0');
              in.read (const_cast<char*> (structurename.data()), structurename_length);
              const int32_t r = fetch<int32_t> (in);
              const int32_t g = fetch<int32_t> (in);
              const int32_t b = fetch<int32_t> (in);
              const int32_t t = fetch<int32_t> (in);
              const int32_t a = 255 - t; // Alpha = 255 - transparency
              add_line (table, structurename + "," + str(r) + "," + str(g) + "," + str(b) + "," + str(a));
            }
            return table;
          };



          auto read_colourtable_V2 = [&] (Input& in)
          {
            const int32_t nentries = fetch<int32_t> (in);
            if (!nentries)
              throw Exception ("Error reading colour table from file \"" + H.name() + "\": No entries");
            vector<std::string> table;
            const int32_t filename_length = fetch<int32_t> (in);
            std::string filename (filename_length + 1, '\0');
            in.read (const_cast<char*> (filename.data()), filename_length);
            const int32_t num_entries_to_read = fetch<int32_t> (in);
            for (size_t i = 0; i != num_entries_to_read; ++i) {
              const int32_t structure = fetch<int32_t> (in);
              if (structure < 0)
                throw Exception ("Error reading colour table from file \"" + H.name() + "\": Negative structure index (" + str(structure) + ")");
              if (structure < table.size() && table[structure].size())
                throw Exception ("Error reading colour table from file \"" + H.name() + "\": Duplicate structure index (" + str(structure) + ")");
              else if (structure >= table.size())
                table.resize (structure + 1, std::string());
              const int32_t structurename_length = fetch<int32_t> (in);
              if (structurename_length < 0)
                throw Exception ("Error reading colour table from file \"" + H.name() + "\": Negative structure name length");
              std::string structurename (structurename_length + 1, '\0');
              in.read (const_cast<char*> (structurename.data()), structurename_length);
              const int32_t r = fetch<int32_t> (in);
              const int32_t g = fetch<int32_t> (in);
              const int32_t b = fetch<int32_t> (in);
              const int32_t t = fetch<int32_t> (in);
              const int32_t a = 255 - t; // Alpha = 255 - transparency
              table[structure] = structurename + "," + str(r) + "," + str(g) + "," + str(b) + "," + str(a);
            }
            std::string result;
            for (size_t index = 0; index != table.size(); ++index) {
              if (table[index].size())
                add_line (result, str(index) + "," + table[index]);
            }
            return result;
          };






          // Start the function read_other() proper
          try {
            // fetch() will throw an int(1) straight away if these data don't exist
            H.keyval()["MGH_TR"] = str(fetch<float32> (in), 6);
            H.keyval()["MGH_flip"] = str(fetch<float32> (in) * 180.0 / Math::pi, 6); // Radians in MGHO -> degrees in header
            H.keyval()["MGH_TE"] = str(fetch<float32> (in), 6);
            H.keyval()["MGH_TI"] = str(fetch<float32> (in), 6);
            fetch<float32> (in); // fov - ignored

            do {
              auto id = fetch<int32_t> (in);
              int64_t size;
              if (id == MGH_TAG_OLD_MGH_XFORM)
                size = fetch<int32_t> (in) - 1;
              else if (id == MGH_TAG_OLD_SURF_GEOM
                    || id == MGH_TAG_OLD_USEREALRAS
                    || id == MGH_TAG_OLD_COLORTABLE)
                size = 0;
              else
                size = fetch<int64_t> (in);
              std::string content (size+1, '\0');
              switch (id) {
                case MGH_TAG_MRI_FRAME:
                  H.keyval()[tag_ID_to_string(id)] = read_mri_frame (in, size);
                  break;
                case MGH_TAG_OLD_COLORTABLE:
                  {
                    const int32_t version = fetch<int32_t> (in);
                    if (version > 0) {
                      const int32_t nentries = version;
                      H.keyval()[tag_ID_to_string(id)] = read_colourtable_V1 (in, nentries);
                    } else if (version == -2) {
                      H.keyval()[tag_ID_to_string(id)] = read_colourtable_V2 (in);
                    } else {
                      throw Exception ("Error reading colour table from file \"" + H.name() + "\": Unknown version (" + str(version) + ")");
                    }
                  }
                  break;
                case MGH_TAG_OLD_MGH_XFORM:
                case MGH_TAG_MGH_XFORM:
                  in.read (const_cast<char*> (content.data()), size);
                  H.keyval()[tag_ID_to_string(id)] = content;
                  // Don't care whether or not we can actually access this file...
                  break;
                case MGH_TAG_CMDLINE:
                  in.read (const_cast<char*> (content.data()), size);
                  // Should only appear one line at a time
                  add_line (H.keyval()["command_history"], content);
                  break;
                case MGH_TAG_AUTO_ALIGN:
                  // Imports data into a 4x4 matrix, and stores in header by converting to string
                  H.keyval()[tag_ID_to_string(id)] = str(read_matrix (in));
                  break;
                case MGH_TAG_PEDIR:
                  in.read (const_cast<char*> (content.data()), size);
                  H.keyval()[tag_ID_to_string(id)] = content;
                  break;
                case MGH_TAG_FIELDSTRENGTH: {
                    // This field is written with TAGwrite() rather than znzwriteFloat()
                    // Therefore, the byte order is NOT forced to big-endian
                    // As a consequence, this import will only work if the system that
                    //   created the file, and the one reading the file, have the
                    //   same endianness
                    float32 field_strength = fetch<float32>(in);
#ifndef MRTRIX_IS_BIG_ENDIAN
                    ByteOrder::swap (field_strength);
#endif
                    H.keyval()[tag_ID_to_string(id)] = str(field_strength);
                  }
                  break;
                default: // FreeSurfer doesn't actually perform any import of any other fields
                  in.read (const_cast<char*> (content.data()), size);
                  break;
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



          // Function znzWriteMatrix() is used for both MGH_TAG_AUTO_ALIGN tag entries,
          //   and in znzTAGwriteMRIframes() for the VOX2RAS matrix.
          auto write_matrix = [] (const Eigen::Matrix<default_type, 4, 4>& M, Output& out)
          {
            char buffer[MGH_MATRIX_STRLEN];
            memset (buffer, 0x00, MGH_MATRIX_STRLEN);
            sprintf (buffer, "AutoAlign %10lf %10lf %10lf %10lf %10lf %10lf %10lf %10lf %10lf %10lf %10lf %10lf %10lf %10lf %10lf %10lf",
                    M(0,0), M(0,1), M(0,2), M(0,3),
                    M(1,0), M(1,1), M(1,2), M(1,3),
                    M(2,0), M(2,1), M(2,2), M(2,3),
                    M(3,0), M(3,1), M(3,2), M(3,3));
            store<int32_t> (MGH_TAG_AUTO_ALIGN, out);
            store<int64_t> (MGH_MATRIX_STRLEN, out);
            out.write (buffer, MGH_MATRIX_STRLEN);
          };



          auto write_mri_frames = [&] (const std::string& table, Output& out)
          {
            const size_t nframes = H.ndim() == 4 ? H.size(3) : 1;
            const auto lines = split_lines (table);
            if (lines.size() != nframes) {
              WARN ("Error writing MRI frame data to output image (image has " + str(nframes) + " volumes, frame data tables has " + str(lines.size()) + " rows); omitting information from output image");
              return;
            }
            vector<mri_frame> frames (nframes);
            for (size_t frame_index = 0; frame_index != nframes; ++frame_index) {
              const auto entries = split (lines[frame_index], ",", false);
              if (entries.size() != 24 && entries.size() != 45) {
                WARN ("Error writing MRI frame data to output image (frame data table has line with " + str(entries.size()) + " entries, expected 24 or 45); omitting information from output image");
                return;
              }
              mri_frame& frame (frames[frame_index]);
              frame.type = to<int32_t> (entries[0]);
              frame.TE = to<float32> (entries[1]);
              frame.TR = to<float32> (entries[2]);
              frame.flip = to<float32> (entries[3]);
              frame.TI = to<float32> (entries[4]);
              frame.TD = to<float32> (entries[5]);
              // When stored in an MRtrix3 header key:value entry, TM is NOT included in the list at this point
              frame.sequence_type = to<int32_t> (entries[6]);
              frame.echo_spacing = to<float32> (entries[7]);
              frame.echo_train_len = to<float32> (entries[8]);
              for (size_t i = 0; i != 3; ++i)
                frame.read_dir[i] = to<float32> (entries[9+i]);
              for (size_t i = 0; i != 3; ++i)
                frame.pe_dir[i] = to<float32> (entries[12+i]);
              for (size_t i = 0; i != 3; ++i)
                frame.slice_dir[i] = to<float32> (entries[15+i]);
              frame.label = to<int32_t> (entries[18]);
              strcpy (frame.name, entries[19].c_str());
              frame.dof = to<int32_t> (entries[20]);

              frame.m_ras2vox = new Eigen::Matrix<default_type, 4, 4> (Eigen::Matrix<default_type, 4, 4>::Zero());
              const auto M = split (entries[21], " ");
              if (M.size() != 16) {
                WARN ("Error writing MRI frame data to output image (expected RAS2vox matrix with 16 entries, read " + str(M.size()) + "); omitting information from output image");
                return;
              }
              (*frame.m_ras2vox)(0,0) = to<default_type> (M[0]);
              (*frame.m_ras2vox)(0,1) = to<default_type> (M[1]);
              (*frame.m_ras2vox)(0,2) = to<default_type> (M[2]);
              (*frame.m_ras2vox)(0,3) = to<default_type> (M[3]);
              (*frame.m_ras2vox)(1,0) = to<default_type> (M[4]);
              (*frame.m_ras2vox)(1,1) = to<default_type> (M[5]);
              (*frame.m_ras2vox)(1,2) = to<default_type> (M[6]);
              (*frame.m_ras2vox)(1,3) = to<default_type> (M[7]);
              (*frame.m_ras2vox)(2,0) = to<default_type> (M[8]);
              (*frame.m_ras2vox)(2,1) = to<default_type> (M[9]);
              (*frame.m_ras2vox)(2,2) = to<default_type> (M[10]);
              (*frame.m_ras2vox)(2,3) = to<default_type> (M[11]);
              (*frame.m_ras2vox)(3,0) = to<default_type> (M[12]);
              (*frame.m_ras2vox)(3,1) = to<default_type> (M[13]);
              (*frame.m_ras2vox)(3,2) = to<default_type> (M[14]);
              (*frame.m_ras2vox)(3,3) = to<default_type> (M[15]);

              frame.thresh = to<float32> (entries[22]);
              frame.units = to<int32_t> (entries[23]);

              if (frame.type == MGH_FRAME_TYPE_DIFFUSION_AUGMENTED) {
                if (entries.size() != 45) {
                  WARN ("Error writing MRI frame data to output image (frame indicated as diffusion-augmented, but does not have sufficient data); omitting information from output image");
                  return;
                }
                frame.DX = to<float64> (entries[25]);
                frame.DY = to<float64> (entries[26]);
                frame.DZ = to<float64> (entries[27]);
                frame.DR = to<float64> (entries[28]);
                frame.DP = to<float64> (entries[29]);
                frame.DS = to<float64> (entries[30]);
                frame.bvalue = to<float64> (entries[31]);
                frame.TM = to<float64> (entries[32]);
                frame.diffusion_type = to<int64_t> (entries[33]);
                frame.D1_ramp = to<int64_t> (entries[34]);
                frame.D1_flat = to<int64_t> (entries[35]);
                frame.D1_amp = to<float64> (entries[36]);
                frame.D2_ramp = to<int64_t> (entries[37]);
                frame.D2_flat = to<int64_t> (entries[38]);
                frame.D2_amp = to<float64> (entries[39]);
                frame.D3_ramp = to<int64_t> (entries[40]);
                frame.D3_flat = to<int64_t> (entries[41]);
                frame.D3_amp = to<float64> (entries[42]);
                frame.D4_ramp = to<int64_t> (entries[43]);
                frame.D4_flat = to<int64_t> (entries[44]);
                frame.D4_amp = to<float64> (entries[45]);
              }
            }
            // That's right: The size allocated is based on the size of a class, which
            //   contains a pointer to a matrix; hence probably why they had to add
            //   a safety factor...
            // TODO See if this can be reduced
            const int64_t len = 10 * nframes * sizeof(mri_frame);
            store<int32_t> (MGH_TAG_MRI_FRAME, out);
            store<int64_t> (len, out);
            const int64_t fstart = out.tellp();
            for (auto frame : frames) {
              store<int32_t> (frame.type, out);
              store<float32> (frame.TE, out);
              store<float32> (frame.TR, out);
              store<float32> (frame.flip, out);
              store<float32> (frame.TI, out);
              store<float32> (frame.TD, out);
              // Once again, have to account for the fact that FreeSurfer reads / writes a single-precision
              //   floating-point value for TM here, even though TM is a double and resides in the
              //   augmented diffusion section
              store<float32> (0.0f, out);
              store<int32_t> (frame.sequence_type, out);
              store<float32> (frame.echo_spacing, out);
              store<float32> (frame.echo_train_len, out);
              for (size_t i = 0; i != 3; ++i)
                store<float32> (frame.read_dir[i], out);
              for (size_t i = 0; i != 3; ++i)
                store<float32> (frame.pe_dir[i], out);
              for (size_t i = 0; i != 3; ++i)
                store<float32> (frame.slice_dir[i], out);
              store<int32_t> (frame.label, out);
              out.write (frame.name, MGH_STRLEN);
              store<int32_t> (frame.dof, out);
              write_matrix (*frame.m_ras2vox, out); delete frame.m_ras2vox; frame.m_ras2vox = nullptr;
              store<float32> (frame.thresh, out);
              store<int32_t> (frame.units, out);
              if (frame.type == MGH_FRAME_TYPE_DIFFUSION_AUGMENTED) {
                store<float64> (frame.DX, out);
                store<float64> (frame.DY, out);
                store<float64> (frame.DZ, out);
                store<float64> (frame.DR, out);
                store<float64> (frame.DP, out);
                store<float64> (frame.DS, out);
                store<float64> (frame.bvalue, out);
                store<float64> (frame.TM, out);
                store<int64_t> (frame.diffusion_type, out);
                store<int64_t> (frame.D1_ramp, out);
                store<int64_t> (frame.D1_flat, out);
                store<float64> (frame.D1_amp, out);
                store<int64_t> (frame.D2_ramp, out);
                store<int64_t> (frame.D2_flat, out);
                store<float64> (frame.D2_amp, out);
                store<int64_t> (frame.D3_ramp, out);
                store<int64_t> (frame.D3_flat, out);
                store<float64> (frame.D3_amp, out);
                store<int64_t> (frame.D4_ramp, out);
                store<int64_t> (frame.D4_flat, out);
                store<float64> (frame.D4_amp, out);
              }
            }

            const int64_t fend = out.tellp();
            const int64_t extra_space_len = len - (fend - fstart);
            if (extra_space_len > 0) {
              char buffer[extra_space_len];
              memset (buffer, 0x00, extra_space_len);
              out.write (buffer, extra_space_len);
            }
          };



          auto write_colourtable_V1 = [] (const std::string& table, Output& out)
          {
            const auto lines = split_lines (table);
            if (!lines.size())
              return;
            store<int32_t> (MGH_TAG_OLD_COLORTABLE, out);
            store<int32_t> (lines.size(), out);
            const std::string filename = "INTERNAL";
            store<int32_t> (filename.size()+1, out);
            out.write (filename.c_str(), filename.size()+1);
            for (auto line : lines) {
              const auto entries = split (line, ",", true);
              if (entries.size() != 5)
                throw Exception ("Error writing colour table to file: Line has " + str(entries.size()) + " fields, expected 5");
              // Name,Red,Green,Blue,Transparency
              store<int32_t> (entries[0].size()+1, out);
              out.write (entries[0].c_str(), entries[0].size()+1);
              store<int32_t> (to<int32_t> (entries[1]), out);
              store<int32_t> (to<int32_t> (entries[2]), out);
              store<int32_t> (to<int32_t> (entries[3]), out);
              store<int32_t> (255 - to<int32_t> (entries[4]), out);
            }
          };



          auto write_colourtable_V2 = [] (const std::string& table, Output& out)
          {
            store<int32_t> (-2, out);
            // Need to find out the maximum node index
            const auto lines = split_lines (table);
            int32_t max_index = 0;
            for (auto line : lines) {
              const auto entries = split (line, ",", true);
              if (entries.size() != 6)
                throw Exception ("Error writing colour table to file: Line has " + str(entries.size()) + " fields, expected 6");
              const int32_t index = to<int32_t> (entries[0]);
              max_index = std::max (max_index, index);
            }
            store<int32_t> (max_index, out);
            const std::string filename = "INTERNAL";
            store<int32_t> (filename.size()+1, out);
            out.write (filename.c_str(), filename.size()+1);
            // Actual number of entries in the table
            store<int32_t> (lines.size(), out);
            for (auto line : lines) {
              const auto entries = split (line, ",", true);
              // Index,Name,Red,Green,Blue,Transparency
              store<int32_t> (to<int32_t> (entries[0]), out);
              store<int32_t> (entries[1].size()+1, out);
              out.write (entries[1].c_str(), entries[1].size()+1);
              store<int32_t> (to<int32_t> (entries[2]), out);
              store<int32_t> (to<int32_t> (entries[3]), out);
              store<int32_t> (to<int32_t> (entries[4]), out);
              store<int32_t> (255 - to<int32_t> (entries[5]), out);
            }
          };







          float32 tr = 0.0f;         /*!< milliseconds */
          float32 flip_angle = 0.0f; /*!< radians */
          float32 te = 0.0f;         /*!< milliseconds */
          float32 ti = 0.0f;         /*!< milliseconds */
          float32 fov = 0.0f;        /*!< IGNORE THIS FIELD (data is inconsistent) */
          Tag transform_tag;
          vector<Tag> tags;          /*!< variable length char strings */
          std::unique_ptr<Eigen::Matrix<default_type, 4, 4>> auto_align_matrix;
          std::string pe_dir ("UNKNOWN");
          float32 field_strength = NaN;
          std::string mri_frames, colour_table;
          vector<Tag> cmdline_tags;

          for (auto entry : H.keyval()) {
            if (entry.first == "command_history") {
              for (auto line : split_lines (entry.second))
                cmdline_tags.push_back (Tag (MGH_TAG_CMDLINE, line));
            } else if (entry.first.size() < 5 || entry.first.substr(0, 4) != "MGH_") {
              continue;
            }
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
              if (id == MGH_TAG_MRI_FRAME) {
                mri_frames = entry.second;
              } else if (id == MGH_TAG_MGH_XFORM) {
                transform_tag.set (MGH_TAG_MGH_XFORM, entry.second);
              } else if (id == MGH_TAG_AUTO_ALIGN) {
                auto_align_matrix.reset (new Eigen::Matrix<default_type, 4, 4> (Eigen::Matrix<default_type, 4, 4>::Zero()));
                const auto lines = split_lines (entry.second);
                if (lines.size() != 4)
                  throw Exception ("Error parsing auto align header entry for MGH format: Invalid number of lines (" + str(lines.size()) + ", should be 4)");
                for (size_t row = 0; row != 4; ++row) {
                  const auto entries = split (lines[row], " ", true);
                  if (entries.size() != 4)
                    throw Exception ("Error parsing auto align header entry for MGH format: Invalid number of entries on line " + str(row) + " (" + str(entries.size()) + ", should be 4)");
                  for (size_t col = 0; col != 4; ++col)
                    (*auto_align_matrix) (row, col) = to<default_type> (entries[col]);
                }
              } else if (id == MGH_TAG_PEDIR) {
                pe_dir = entry.second;
              } else if (id == MGH_TAG_FIELDSTRENGTH) {
                field_strength = to<float32> (entry.second);
              } else if (id == MGH_TAG_COLORTABLE || id == MGH_TAG_OLD_COLORTABLE) {
                colour_table = entry.second;
              } else if (id) {
                tags.push_back (Tag (id, entry.second));
              }
            }
          }

          // Although we could theoretically avoid writing any metadata here at all if there were no interesting
          //   data to write, the fact that "command_history" will always have at least one entry (corresponding
          //   to the currently-executing command) means that MGH_TAG_CMDLINE will always have at least one entry

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
          if (auto_align_matrix)
            write_matrix (*auto_align_matrix, out);
          store<int32_t> (MGH_TAG_PEDIR, out);
          store<int64_t> (pe_dir.size()+1, out);
          out.write (pe_dir.c_str(), pe_dir.size() + 1);
          if (std::isfinite (field_strength)) {
            store<int32_t> (MGH_TAG_FIELDSTRENGTH, out);
            store<int64_t> (sizeof (float32), out);
            // FreeSurfer uses znzTAGwrite() to write the field strength, which means it gets
            //   written with the native endianness of the system, rather than being
            //   forced to big-endian like the rest of the format.
            out.write (reinterpret_cast<char*> (&field_strength), sizeof (float32));
          }
          if (mri_frames.size())
            write_mri_frames (mri_frames, out);
          if (colour_table.size()) {
            const std::string first_line = colour_table.substr (0, colour_table.find_first_of ('\n'));
            const auto entries = split (first_line, ",", true);
            switch (entries.size()) {
              case 5: write_colourtable_V1 (colour_table, out); break;
              case 6: write_colourtable_V2 (colour_table, out); break;
              default: WARN ("Malformed colour table in header (incorrect number of columns); not written to output image"); break;
            }
          }
          if (cmdline_tags.size()) {
            for (auto tag : cmdline_tags) {
              store<int32_t> (tag.id, out);
              store<int64_t> (tag.content.size()+1, out);
              out.write (tag.content.c_str(), tag.content.size()+1);
            }
          }
        }

    }
  }
}

#endif

