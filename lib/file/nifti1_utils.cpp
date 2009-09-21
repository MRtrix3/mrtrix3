/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 26/08/09.

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

#include "get_set.h"
#include "file/config.h"
#include "file/nifti1_utils.h"
#include "math/math.h"
#include "math/permutation.h"
#include "math/quaternion.h"
#include "image/header.h"

namespace MR {
  namespace File {
    namespace NIfTI {

      namespace {
        bool  right_left_warning_issued = false;
      }




      size_t read (Image::Header& H, const nifti_1_header& NH)
      {
        bool is_BE = false;
        if (get<int32_t> (&NH.sizeof_hdr, is_BE) != 348) {
          is_BE = true;
          if (get<int32_t> (&NH.sizeof_hdr, is_BE) != 348) 
            throw Exception ("image \"" + H.name() + "\" is not in NIfTI format (sizeof_hdr != 348)");
        }

        bool is_nifti = true;
        if (memcmp (NH.magic, "n+1\0", 4) && memcmp (NH.magic, "ni1\0", 4)) {
          is_nifti = false;
          debug ("assuming image \"" + H.name() + "\" is in AnalyseAVW format.");
        }

        char db_name[19];
        strncpy (db_name, NH.db_name, 18);
        if (db_name[0]) {
          db_name[18] = '\0';
          H.comments.push_back (db_name);
        }

        // data set dimensions:
        int ndim = get<int16_t> (&NH.dim, is_BE);
        if (ndim < 1) 
          throw Exception ("too few dimensions specified in NIfTI image \"" + H.name() + "\"");
        if (ndim > 7) 
          throw Exception ("too many dimensions specified in NIfTI image \"" + H.name() + "\"");
        H.axes.ndim() = ndim;


        for (int i = 0; i < ndim; i++) {
          H.axes.dim(i) = get<int16_t> (&NH.dim[i+1], is_BE);
          if (H.axes.dim(i) < 0) {
            info ("dimension along axis " + str(i) + " specified as negative in NIfTI image \"" + H.name() + "\" - taking absolute value");
            H.axes.dim(i) = abs (H.axes.dim(i));
          }
          if (!H.axes.dim(i)) H.axes.dim(i) = 1;
          H.axes.order(i) = i;
          H.axes.forward(i) = true;
        }

        // data type:
        switch (get<int16_t> (&NH.datatype, is_BE)) {
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

        if (get<int16_t> (&NH.bitpix, is_BE) != (int16_t) H.datatype().bits())
          error ("WARNING: bitpix field does not match data type in NIfTI image \"" + H.name() + "\" - ignored");

        // voxel sizes:
        for (int i = 0; i < ndim; i++) {
          H.axes.vox(i) = get<float32> (&NH.pixdim[i+1], is_BE);
          if (H.axes.vox(i) < 0.0) {
            info ("voxel size along axis " + str(i) + " specified as negative in NIfTI image \"" + H.name() + "\" - taking absolute value");
            H.axes.vox(i) = Math::abs (H.axes.vox(i));
          }
        }


        // offset and scale:
        H.scale = get<float32> (&NH.scl_slope, is_BE);
        if (finite(H.scale) && H.scale != 0.0) {
          H.offset = get<float32> (&NH.scl_inter, is_BE);
          H.offset = finite (H.offset) ? H.offset : 0.0;
        }
        else {
          H.scale = 1.0;
          H.offset = 0.0;
        }

        size_t data_offset = (size_t) get<float32> (&NH.vox_offset, is_BE);

        char descrip[81];
        strncpy (descrip, NH.descrip, 80);
        if (descrip[0]) {
          descrip[80] = '\0';
          H.comments.push_back (descrip);
        }

        if (is_nifti) {
          if (get<int16_t> (&NH.sform_code, is_BE)) {
            Math::Matrix<float>& M (H.transform());
            M.allocate (4,4);

            M(0,0) = get<float32> (&NH.srow_x[0], is_BE);
            M(0,1) = get<float32> (&NH.srow_x[1], is_BE);
            M(0,2) = get<float32> (&NH.srow_x[2], is_BE);
            M(0,3) = get<float32> (&NH.srow_x[3], is_BE);

            M(1,0) = get<float32> (&NH.srow_y[0], is_BE);
            M(1,1) = get<float32> (&NH.srow_y[1], is_BE);
            M(1,2) = get<float32> (&NH.srow_y[2], is_BE);
            M(1,3) = get<float32> (&NH.srow_y[3], is_BE);

            M(2,0) = get<float32> (&NH.srow_z[0], is_BE);
            M(2,1) = get<float32> (&NH.srow_z[1], is_BE);
            M(2,2) = get<float32> (&NH.srow_z[2], is_BE);
            M(2,3) = get<float32> (&NH.srow_z[3], is_BE);

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
          else if (get<int16_t> (&NH.qform_code, is_BE)) {
            Math::Quaternion Q (get<float32> (&NH.quatern_b, is_BE), get<float32> (&NH.quatern_c, is_BE), get<float32> (&NH.quatern_d, is_BE));
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

            M(0,3) = get<float32> (&NH.qoffset_x, is_BE);
            M(1,3) = get<float32> (&NH.qoffset_y, is_BE);
            M(2,3) = get<float32> (&NH.qoffset_z, is_BE);

            M(3,0) = M(3,1) = M(3,2) = 0.0;
            M(3,3) = 1.0;

            // qfac:
            float qfac = get<float32> (&NH.pixdim[0], is_BE);
            if (qfac != 0.0) {
              M(0,2) *= qfac;
              M(1,2) *= qfac;
              M(2,2) *= qfac;
            }
          }
        }
        else {
          H.transform().clear();
          H.axes.forward(0) = File::Config::get_bool ("Analyse.LeftToRight", true); 
          if (!right_left_warning_issued) {
            info ("assuming Analyse images are encoded " + std::string (H.axes.forward(0) ? "left to right" : "right to left"));
            right_left_warning_issued = true;
          }
        }


        if (!H.axes.description(0).size()) H.axes.description(0) = Image::Axes::left_to_right;
        if (!H.axes.units(0).size()) H.axes.units(0) = Image::Axes::millimeters;
        if (H.ndim() > 1) {
          if (!H.axes.description(1).size()) H.axes.description(1) = Image::Axes::posterior_to_anterior;
          if (!H.axes.units(1).size()) H.axes.units(1) = Image::Axes::millimeters;
          if (H.ndim() > 2) {
            if (!H.axes.description(2).size()) H.axes.description(2) = Image::Axes::inferior_to_superior;
            if (!H.axes.units(2).size()) H.axes.units(2) = Image::Axes::millimeters;
          }
        }

        return (data_offset);
      }







      void check (Image::Header& H, bool single_file)
      {
        for (size_t i = 0; i < H.ndim(); ++i) 
          if (H.axes.dim(i) < 1) H.axes.dim(i) = 1;

        if (single_file) { 
          Image::Axes::Order order[H.ndim()];
          size_t i, axis = 0;
          for (i = 0; i < H.ndim() && axis < 3; ++i) 
            if (H.axes.order(i) < 3) order[axis++] = H.axes[i];

          assert (axis == 3);

          for (i = 0; i < H.ndim(); ++i)
            if (H.axes.order(i) >= 3) order[axis++] = H.axes[i];

          assert (axis == H.ndim());

          for (i = 0; i < 3; ++i) {
            H.axes.order(i) = order[i].order;
            H.axes.forward(i) = order[i].forward;
          }

          for (; i < H.ndim(); ++i) {
            H.axes.order(i) = order[i].order;
            H.axes.forward(i) = true;
          }
        }
        else {
          for (size_t i = 0; i < H.ndim(); ++i) {
            H.axes.order(i) = i;
            H.axes.forward(i) = true;
          }
          H.axes.forward(0) = File::Config::get_bool ("Analyse.LeftToRight", true); 

          if (!right_left_warning_issued) {
            info ("assuming Analyse images are encoded " + std::string (H.axes.forward(0) ? "left to right" : "right to left"));
            right_left_warning_issued = true;
          }
        }

        H.axes.description(0) = Image::Axes::left_to_right;
        H.axes.units(0) = Image::Axes::millimeters;

        H.axes.description(1) = Image::Axes::posterior_to_anterior;
        H.axes.units(1) = Image::Axes::millimeters;

        H.axes.description(1) = Image::Axes::inferior_to_superior;
        H.axes.units(1) = Image::Axes::millimeters;

      }





      void write (nifti_1_header& NH, const Image::Header& H, bool single_file)
      {
        if (H.ndim() > 7) 
          throw Exception ("NIfTI-1.1 format cannot support more than 7 dimensions for image \"" + H.name() + "\"");

        bool is_BE = H.datatype().is_big_endian();

        // new transform handling code starts here
        Math::Permutation permutation (3);
        for (size_t i = 0; i < 3; ++i) {
          assert (H.axes.order(i) < 3);
          permutation[i] = H.axes.order(i);
        }

        Math::Matrix<float> M (H.transform());

        if (permutation[0] != 0 || permutation[1] != 1 || permutation[2] != 2 || 
            !H.axes.forward(0)  || !H.axes.forward(1)  || !H.axes.forward(2)) {

          Math::VectorView<float> translation = M.column(3).view(0,3);
          for (size_t i = 0; i < 3; i++) {
            if (!H.axes.forward(i)) {
              float length = float(H.axes.dim(i)-1) * H.axes.vox(i);
              Math::VectorView<float> axis = M.column(i).view(0,3);
              for (size_t n = 0; n < 3; n++) {
                axis[n] = -axis[n];
                translation[n] -= length*axis[n];
              }
            }
          }

          for (size_t i = 0; i < 3; i++) {
            Math::VectorView<float> row = M.row(i).view(0,3);
            permutation.apply (row); 
          }
        }

        // new transform handling code ends here


        // magic number:
        put<int32_t> (348, &NH.sizeof_hdr, is_BE);

        strncpy ((char*) &NH.datatype, "dsr      \0", 10);
        strncpy ((char*) &NH.db_name, H.comments.size() ? H.comments[0].c_str() : "untitled\0\0\0\0\0\0\0\0\0\0\0", 18);
        put<int32_t> (16384, &NH.extents, is_BE);
        strncpy ((char*) &NH.regular, "r\0", 2);

        // data set dimensions:
        put<int16_t> (H.ndim(), &NH.dim[0], is_BE);
        for (size_t i = 0; i < 3; i++) 
          put<int16_t> (H.axes.dim(permutation[i]), &NH.dim[i+1], is_BE);
        for (size_t i = 3; i < H.ndim(); i++) 
          put<int16_t> (H.axes.dim(i), &NH.dim[i+1], is_BE);

        // data type:
        int16_t dt = 0;
        switch (H.datatype()()) {
          case DataType::Bit:       dt = DT_BINARY; break;
          case DataType::Int8:      dt = DT_INT8;  break;
          case DataType::UInt8:     dt = DT_UINT8;  break;
          case DataType::Int16LE:   
          case DataType::Int16BE:   dt = DT_INT16; break;
          case DataType::UInt16LE: 
          case DataType::UInt16BE:  dt = DT_UINT16; break;
          case DataType::Int32LE: 
          case DataType::Int32BE:   dt = DT_INT32; break;
          case DataType::UInt32LE:
          case DataType::UInt32BE:  dt = DT_UINT32; break;
          case DataType::Float32LE:
          case DataType::Float32BE: dt = DT_FLOAT32; break;
          case DataType::Float64LE:
          case DataType::Float64BE: dt = DT_FLOAT64; break;
          case DataType::CFloat32LE:
          case DataType::CFloat32BE: dt = DT_COMPLEX64; break;
          case DataType::CFloat64LE:
          case DataType::CFloat64BE: dt = DT_COMPLEX128; break;
          default: throw Exception ("unknown data type for NIfTI-1.1 image \"" + H.name() + "\"");
        }

        put<int16_t> (dt, &NH.datatype, is_BE);  
        put<int16_t> (H.datatype().bits(), &NH.bitpix, is_BE);

        // voxel sizes:
        for (size_t i = 0; i < 3; i++) 
          put<float32> (H.axes.vox(permutation[i]), &NH.pixdim[i+1], is_BE);
        for (size_t i = 3; i < H.ndim(); i++) 
          put<float32> (H.axes.vox(i), &NH.pixdim[i+1], is_BE);

        put<float32> (352.0, &NH.vox_offset, is_BE);

        // offset and scale:
        put<float32> (H.scale, &NH.scl_slope, is_BE);
        put<float32> (H.offset, &NH.scl_inter, is_BE);

        NH.xyzt_units = SPACE_TIME_TO_XYZT (NIFTI_UNITS_MM, NIFTI_UNITS_SEC);

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
        strncpy ((char*) &NH.descrip, descrip, 80);

        put<int16_t> (NIFTI_XFORM_UNKNOWN, &NH.qform_code, is_BE);
        put<int16_t> (NIFTI_XFORM_SCANNER_ANAT, &NH.sform_code, is_BE);

        put<float32> (H.axes.vox(permutation[0])*M(0,0), &NH.srow_x[0], is_BE);
        put<float32> (H.axes.vox(permutation[1])*M(0,1), &NH.srow_x[1], is_BE);
        put<float32> (H.axes.vox(permutation[2])*M(0,2), &NH.srow_x[2], is_BE);
        put<float32> (M(0,3), &NH.srow_x[3], is_BE);

        put<float32> (H.axes.vox(permutation[0])*M(1,0), &NH.srow_y[0], is_BE);
        put<float32> (H.axes.vox(permutation[1])*M(1,1), &NH.srow_y[1], is_BE);
        put<float32> (H.axes.vox(permutation[2])*M(1,2), &NH.srow_y[2], is_BE);
        put<float32> (M(1,3), &NH.srow_y[3], is_BE);

        put<float32> (H.axes.vox(permutation[0])*M(2,0), &NH.srow_z[0], is_BE);
        put<float32> (H.axes.vox(permutation[1])*M(2,1), &NH.srow_z[1], is_BE);
        put<float32> (H.axes.vox(permutation[2])*M(2,2), &NH.srow_z[2], is_BE);
        put<float32> (M(2,3), &NH.srow_z[3], is_BE);

        strncpy ((char*) &NH.magic, single_file ? "n+1\0" : "ni1\0", 4);
      }

    }
  }
}


