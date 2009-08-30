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

#include "file/misc.h"
#include "file/entry.h"
#include "file/config.h"
#include "image/misc.h"
#include "image/header.h"
#include "get_set.h"
#include "image/format/list.h"
#include "image/name_parser.h"
#include "math/math.h"
#include "math/quaternion.h"
#include "file/nifti1.h"

namespace MR {
  namespace Image {
    namespace Format {

      namespace {
        bool  right_left_warning_issued = false;
      }






      bool Analyse::read (Header& H) const
      {
        if (!Path::has_suffix (H.name(), ".img")) return (false);

        File::MMap fmap (H.name().substr (0, H.name().size()-4) + ".hdr");

        const nifti_1_header* NH = (const nifti_1_header*) fmap.address();

        bool is_BE = false;
        if (get<int32_t> (&NH->sizeof_hdr, is_BE) != 348) {
          is_BE = true;
          if (get<int32_t> (&NH->sizeof_hdr, is_BE) != 348) 
            throw Exception ("image \"" + H.name() + "\" is not in Analyse format (sizeof_hdr != 348)");
        }

        bool is_nifti = memcmp (NH->magic, "ni1\0", 4) == 0;

        char db_name[19];
        strncpy (db_name, NH->db_name, 18);
        if (db_name[0]) {
          db_name[18] = '\0';
          H.comments.push_back (db_name);
        }

        // data set dimensions:
        int ndim = get<int16_t> (&NH->dim, is_BE);
        if (ndim < 1) 
          throw Exception ("too few dimensions specified in Analyse image \"" + H.name() + "\"");
        if (ndim > 7) 
          throw Exception ("too many dimensions specified in Analyse image \"" + H.name() + "\"");
        H.axes.ndim() = ndim;


        for (int i = 0; i < ndim; i++) {
          H.axes.dim(i) = get<int16_t> (&NH->dim[i+1], is_BE);
          if (H.axes.dim(i) < 0) {
            info ("dimension along axis " + str(i) + " specified as negative in Analyse image \"" + H.name() + "\" - taking absolute value");
            H.axes.dim(i) = abs (H.axes.dim(i));
          }
          if (!H.axes.dim(i)) H.axes.dim(i) = 1;
          H.axes.order(i) = i;
          H.axes.forward(i) = true;
        }

        // data type:
        switch (get<int16_t> (&NH->datatype, is_BE)) {
          case DT_BINARY:     H.datatype() = DataType::Bit;        break;
          case DT_INT8:       H.datatype() = DataType::Int8;       break;
          case DT_UINT8:      H.datatype() = DataType::UInt8;      break;
          case DT_INT16:      H.datatype() = DataType::Int16;      break;
          case DT_UINT16:     H.datatype() = DataType::UInt16;     break;
          case DT_INT32:      H.datatype() = DataType::Int32;      break;
          case DT_UINT32:     H.datatype() = DataType::UInt32;     break;
          case DT_FLOAT32:    H.datatype() = DataType::Float32;    break;
          case DT_FLOAT64:    H.datatype() = DataType::Float64;    break;
          case DT_COMPLEX64:  H.datatype() = DataType::CFloat32;   break;
          case DT_COMPLEX128: H.datatype() = DataType::CFloat64;   break;
          default: throw Exception ("unknown data type for Analyse image \"" + H.name() + "\"");
        }

        if ( !( H.datatype().is (DataType::UInt8) || H.datatype().is (DataType::Int8) ) ) {
          if (is_BE) H.datatype().set_flag (DataType::BigEndian);
          else H.datatype().set_flag (DataType::LittleEndian);
        }

        if (get<int16_t> (&NH->bitpix, is_BE) != (int16_t) H.datatype().bits())
          error ("WARNING: bitpix field does not match data type in Analyse image \"" + H.name() + "\" - ignored");

        // voxel sizes:
        for (int i = 0; i < ndim; i++) {
          H.axes.vox(i) = get<float> (&NH->pixdim[i+1], is_BE);
          if (H.axes.vox(i) < 0.0) {
            info ("voxel size along axis " + str(i) + " specified as negative in Analyse image \"" + H.name() + "\" - taking absolute value");
            H.axes.vox(i) = Math::abs (H.axes.vox(i));
          }
        }


        // offset and scale:
        H.scale = get<float> (&NH->scl_slope, is_BE);
        if (finite(H.scale) && H.scale != 0.0) {
          H.offset = get<float> (&NH->scl_inter, is_BE);
          H.offset = finite (H.offset) ? H.offset : 0.0;
        }
        else {
          H.scale = 1.0;
          H.offset = 0.0;
        }

        size_t data_offset = ( is_nifti ? 0 : (size_t) get<float> (&NH->vox_offset, is_BE) );

        char descrip[81];
        strncpy (descrip, NH->descrip, 80);
        if (descrip[0]) {
          descrip[80] = '\0';
          H.comments.push_back (descrip);
        }

        if (is_nifti) {
          if (get<int16_t> (&NH->sform_code, is_BE)) {
            Math::Matrix<float>& M (H.transform());
            M.allocate (4,4);
            M(0,0) = get<float> (&NH->srow_x[0], is_BE);
            M(0,1) = get<float> (&NH->srow_x[1], is_BE);
            M(0,2) = get<float> (&NH->srow_x[2], is_BE);
            M(0,3) = get<float> (&NH->srow_x[3], is_BE);

            M(1,0) = get<float> (&NH->srow_y[0], is_BE);
            M(1,1) = get<float> (&NH->srow_y[1], is_BE);
            M(1,2) = get<float> (&NH->srow_y[2], is_BE);
            M(1,3) = get<float> (&NH->srow_y[3], is_BE);

            M(2,0) = get<float> (&NH->srow_z[0], is_BE);
            M(2,1) = get<float> (&NH->srow_z[1], is_BE);
            M(2,2) = get<float> (&NH->srow_z[2], is_BE);
            M(2,3) = get<float> (&NH->srow_z[3], is_BE);

            M(3,0) = M(3,1) = M(3,2) = 0.0;
            M(3,3) = 1.0;

            // get voxel sizes:
            H.axes.vox(0) = Math::sqrt (Math::pow2 (M(0,0)) + Math::pow2 (M(1,0)) + Math::pow2 (M(2,0)));
            H.axes.vox(1) = Math::sqrt (Math::pow2 (M(0,1)) + Math::pow2 (M(1,1)) + Math::pow2 (M(2,1)));
            H.axes.vox(2) = Math::sqrt (Math::pow2 (M(0,2)) + Math::pow2 (M(1,2)) + Math::pow2 (M(2,2)));

            // normalize each transform axis:
            M(0,0) /= H.axes.vox(0);
            M(1,0) /= H.axes.vox(0);
            M(2,0) /= H.axes.vox(0);

            M(0,1) /= H.axes.vox(1);
            M(1,1) /= H.axes.vox(1);
            M(2,1) /= H.axes.vox(1);

            M(0,2) /= H.axes.vox(2);
            M(1,2) /= H.axes.vox(2);
            M(2,2) /= H.axes.vox(2);
          }
          else if (get<int16_t> (&NH->qform_code, is_BE)) {
            Math::Quaternion Q (get<float> (&NH->quatern_b, is_BE), get<float> (&NH->quatern_c, is_BE), get<float> (&NH->quatern_d, is_BE));
            float transform[9];
            Q.to_matrix (transform);
            Math::Matrix<float>& M (H.transform());
            M.allocate (4,4);

            M(0,0) = transform[0];
            M(0,1) = transform[1];
            M(0,2) = transform[2];

            M(1,0) = transform[3];
            M(1,1) = transform[4];
            M(1,2) = transform[5];

            M(2,0) = transform[6];
            M(2,1) = transform[7];
            M(2,2) = transform[8];

            M(0,3) = get<float> (&NH->qoffset_x, is_BE);
            M(1,3) = get<float> (&NH->qoffset_y, is_BE);
            M(2,3) = get<float> (&NH->qoffset_z, is_BE);

            M(3,0) = M(3,1) = M(3,2) = 0.0;
            M(3,3) = 1.0;

            // qfac:
            float qfac = get<float> (&NH->pixdim[0], is_BE);
            if (qfac != 0.0) {
              M(0,2) *= qfac;
              M(1,2) *= qfac;
              M(2,2) *= qfac;
            }
          }
        }



        if (!is_nifti) {
          H.axes.forward(0) = File::Config::get_bool ("Analyse.LeftToRight", true); 
          if (!right_left_warning_issued) {
            info ("assuming Analyse images are encoded " + std::string (H.axes.forward(0) ? "left to right" : "right to left"));
            right_left_warning_issued = true;
          }
        }

        if (!H.axes.description(0).size()) H.axes.description(0) = Axes::left_to_right;
        if (!H.axes.units(0).size()) H.axes.units(0) = Axes::millimeters;
        if (H.ndim() > 1) {
          if (!H.axes.description(1).size()) H.axes.description(1) = Axes::posterior_to_anterior;
          if (!H.axes.units(1).size()) H.axes.units(1) = Axes::millimeters;
          if (H.ndim() > 2) {
            if (!H.axes.description(2).size()) H.axes.description(2) = Axes::inferior_to_superior;
            if (!H.axes.units(2).size()) H.axes.units(2) = Axes::millimeters;
          }
        }

        H.files.push_back (File::Entry (H.name(), data_offset));

        return (true);
      }





      bool Analyse::check (Header& H, int num_axes) const
      {
        if (!Path::has_suffix (H.name(), ".img")) return (false);
        if (num_axes < 3) throw Exception ("cannot create Analyse image with less than 3 dimensions");
        if (num_axes > 8) throw Exception ("cannot create Analyse image with more than 8 dimensions");

        H.axes.ndim() = num_axes;
        for (size_t i = 0; i < H.ndim(); i++) {
          if (H.axes.dim(i) < 1) H.axes.dim(i) = 1;
          H.axes.order(i) = i;
          H.axes.forward(i) = true;
        }

        H.axes.forward(0) = File::Config::get_bool ("Analyse.LeftToRight", true);
        if (!right_left_warning_issued) {
          info ("assuming Analyse images are encoded " + std::string (H.axes[0].forward ? "left to right" : "right to left"));
          right_left_warning_issued = true;
        }

        H.axes.description(0) = Axes::left_to_right;
        H.axes.units(0) = Axes::millimeters;

        H.axes.description(1) = Axes::posterior_to_anterior;
        H.axes.units(1) = Axes::millimeters;

        H.axes.description(2) = Axes::inferior_to_superior;
        H.axes.units(2) = Axes::millimeters;

        switch (H.datatype()()) {
          case DataType::Int8: 
            H.datatype() = DataType::Int16; 
            info ("WARNING: changing data type to Int16 for image \"" + H.name() + "\" to ensure compatibility with Analyse");
            break;
          case DataType::UInt16:
          case DataType::UInt16LE:
          case DataType::UInt16BE:
          case DataType::UInt32LE:
          case DataType::UInt32BE:
          case DataType::UInt32: 
            H.datatype() = DataType::Int32;
            info ("WARNING: changing data type to Int32 for image \"" + H.name() + "\" to ensure compatibility with Analyse");
            break;
          case DataType::CFloat64:
          case DataType::CFloat64LE:
          case DataType::CFloat64BE: 
            H.datatype() = DataType::CFloat32; 
            info ("WARNING: changing data type to CFloat32 for image \"" + H.name() + "\" to ensure compatibility with Analyse");
            break;
        }

        return (true);
      }





      void Analyse::create (Header& H) const
      {
        if (H.ndim() > 7) 
          throw Exception ("Analyse format cannot support more than 7 dimensions for image \"" + H.name() + "\"");

        assert (0);
        // TODO: fix up Analyse writing
        /*
        File::MMap fmap (H.name().substr (0, H.name().size()-4) + ".hdr", 348);
        fmap.map();

        nifti_1_header* NH = (nifti_1_header*) fmap.address();

        bool is_BE = H.datatype().is_big_endian();

        put<int32_t> (348, &NH->sizeof_hdr, is_BE);
        strncpy ((char*) &NH->data_type, "dsr      \0", 10);
        strncpy ((char*) &NH->db_name, H.comments.size() ? H.comments[0].c_str() : "untitled\0\0\0\0\0\0\0\0\0\0\0", 18);
        put<int32_t> (16384, &NH->extents, is_BE);
        strncpy ((char*) &NH->regular, "r\0", 2);

        // data set dimensions:
        put<int16_t> (H.axes.size(), &NH->dim[0], is_BE);
        for (size_t i = 0; i < H.axes.size(); i++) 
          put<int16_t> (H.axes[i].dim, &NH->dim[i+1], is_BE);

        // data type:
        int16_t dt = 0;
        switch (H.datatype()()) {
          case DataType::Bit:       dt = DT_BINARY; break;
          case DataType::UInt8:     dt = DT_UINT8;  break;
          case DataType::Int16LE:   
          case DataType::Int16BE:   dt = DT_INT16; break;
          case DataType::Int32LE: 
          case DataType::Int32BE:   dt = DT_INT32; break;
          case DataType::Float32LE:
          case DataType::Float32BE: dt = DT_FLOAT32; break;
          case DataType::Float64LE:
          case DataType::Float64BE: dt = DT_FLOAT64; break;
          case DataType::CFloat32LE:
          case DataType::CFloat32BE: dt = DT_COMPLEX64; break;
          default: throw Exception ("unknown data type for Analyse image \"" + H.name() + "\"");
        }

        put<int16_t> (dt, &NH->datatype, is_BE);  
        put<int16_t> (H.datatype().bits(), &NH->bitpix, is_BE);

        // voxel sizes:
        for (size_t i = 0; i < H.axes.size(); i++) 
          put<float> (H.axes[i].vox, &NH->pixdim[i+1], is_BE);


        // offset and scale:
        put<float> (H.scale, &NH->scl_slope, is_BE);
        put<float> (H.offset, &NH->scl_inter, is_BE);

        int pos = 0;
        char descrip[81];
        descrip[0] = '\0';
        for (size_t i = 1; i < H.comments.size(); i++) {
          if (pos >= 75) break;
          if (i > 1) {
            descrip[pos++] = ';';
            descrip[pos++] = ' ';
          }
          strncpy (descrip + pos, H.comments[i].c_str(), 80-pos);
          pos += H.comments[i].size();
        }
        strncpy ((char*) &NH->descrip, descrip, 80);
        strncpy ((char*) &NH->aux_file, "none", 24);

        fmap.unmap();
        dmap.add (H.name(), 0, memory_footprint (H.datatype(), voxel_count (H.axes)));
        */
      }

    }
  }
}
