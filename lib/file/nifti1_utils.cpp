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

#include "image/stride.h"
#include "get_set.h"
#include "file/config.h"
#include "file/nifti1_utils.h"
#include "math/LU.h"
#include "math/permutation.h"
#include "math/versor.h"
#include "image/header.h"

namespace MR
{
  namespace File
  {
    namespace NIfTI
    {

      namespace
      {
        bool  right_left_warning_issued = false;
      }





      Math::Matrix<float> adjust_transform (const Image::Header& H, std::vector<size_t>& axes)
      {
        Image::Stride::List strides = Image::Stride::get (H);
        strides.resize (3);
        axes = Image::Stride::order (strides);
        bool flip[] = { strides[axes[0]] < 0, strides[axes[1]] < 0, strides[axes[2]] < 0 };

        if (axes[0] == 0 && axes[1] == 1 && axes[2] == 2 &&
            !flip[0] && !flip[1] && !flip[2])
          return H.transform();

        Math::Matrix<float> M (H.transform());

        for (size_t i = 0; i < 3; i++)
          M.column (i) = H.transform().column (axes[i]);

        Math::Vector<float> translation = M.column (3).sub (0,3);
        for (size_t i = 0; i < 3; ++i) {
          if (flip[i]) {
            float length = float (H.dim (axes[i])-1) * H.vox (axes[i]);
            Math::Vector<float> axis = M.column (i).sub (0,3);
            for (size_t n = 0; n < 3; n++) {
              axis[n] = -axis[n];
              translation[n] -= length*axis[n];
            }
          }
        }

        return M;
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
          DEBUG ("assuming image \"" + H.name() + "\" is in AnalyseAVW format.");
        }

        char db_name[19];
        strncpy (db_name, NH.db_name, 18);
        if (db_name[0]) {
          db_name[18] = '\0';
          H.comments().push_back (db_name);
        }

        // data set dimensions:
        int ndim = get<int16_t> (&NH.dim, is_BE);
        if (ndim < 1)
          throw Exception ("too few dimensions specified in NIfTI image \"" + H.name() + "\"");
        if (ndim > 7)
          throw Exception ("too many dimensions specified in NIfTI image \"" + H.name() + "\"");
        H.set_ndim (ndim);


        for (int i = 0; i < ndim; i++) {
          H.dim(i) = get<int16_t> (&NH.dim[i+1], is_BE);
          if (H.dim (i) < 0) {
            INFO ("dimension along axis " + str (i) + " specified as negative in NIfTI image \"" + H.name() + "\" - taking absolute value");
            H.dim(i) = std::abs (H.dim (i));
          }
          if (!H.dim (i))
            H.dim(i) = 1;
          H.stride(i) = i+1;
        }

        // data type:
        DataType dtype;
        switch (get<int16_t> (&NH.datatype, is_BE)) {
          case DT_BINARY:
            dtype = DataType::Bit;
            break;
          case DT_INT8:
            dtype = DataType::Int8;
            break;
          case DT_UINT8:
            dtype = DataType::UInt8;
            break;
          case DT_INT16:
            dtype = DataType::Int16;
            break;
          case DT_UINT16:
            dtype = DataType::UInt16;
            break;
          case DT_INT32:
            dtype = DataType::Int32;
            break;
          case DT_UINT32:
            dtype = DataType::UInt32;
            break;
          case DT_INT64:
            dtype = DataType::Int64;
            break;
          case DT_UINT64:
            dtype = DataType::UInt64;
            break;
          case DT_FLOAT32:
            dtype = DataType::Float32;
            break;
          case DT_FLOAT64:
            dtype = DataType::Float64;
            break;
          case DT_COMPLEX64:
            dtype = DataType::CFloat32;
            break;
          case DT_COMPLEX128:
            dtype = DataType::CFloat64;
            break;
          default:
            throw Exception ("unknown data type for Analyse image \"" + H.name() + "\"");
        }

        if (! (dtype.is (DataType::Bit) || dtype.is (DataType::UInt8) || dtype.is (DataType::Int8))) {
          if (is_BE)
            dtype.set_flag (DataType::BigEndian);
          else
            dtype.set_flag (DataType::LittleEndian);
        }

        if (get<int16_t> (&NH.bitpix, is_BE) != (int16_t) dtype.bits())
          WARN ("bitpix field does not match data type in NIfTI image \"" + H.name() + "\" - ignored");

        H.datatype() = dtype;

        // voxel sizes:
        for (int i = 0; i < ndim; i++) {
          H.vox(i) = get<float32> (&NH.pixdim[i+1], is_BE);
          if (H.vox (i) < 0.0) {
            INFO ("voxel size along axis " + str (i) + " specified as negative in NIfTI image \"" + H.name() + "\" - taking absolute value");
            H.vox(i) = std::abs (H.vox (i));
          }
        }


        // offset and scale:
        H.intensity_scale() = get<float32> (&NH.scl_slope, is_BE);
        if (std::isfinite (H.intensity_scale()) && H.intensity_scale() != 0.0) {
          H.intensity_offset() = get<float32> (&NH.scl_inter, is_BE);
          if (!std::isfinite (H.intensity_offset()))
            H.intensity_offset() = 0.0;
        }
        else {
          H.intensity_offset() = 0.0;
          H.intensity_scale() = 1.0;
        }

        size_t data_offset = (size_t) get<float32> (&NH.vox_offset, is_BE);

        char descrip[81];
        strncpy (descrip, NH.descrip, 80);
        if (descrip[0] && strncmp (descrip, "MRtrix version: ", 16)) {
          descrip[80] = '\0';
          H.comments().push_back (descrip);
        }

        if (is_nifti) {
          if (get<int16_t> (&NH.sform_code, is_BE)) {
            Math::Matrix<float>& M (H.transform());
            M.allocate (4,4);

            M (0,0) = get<float32> (&NH.srow_x[0], is_BE);
            M (0,1) = get<float32> (&NH.srow_x[1], is_BE);
            M (0,2) = get<float32> (&NH.srow_x[2], is_BE);
            M (0,3) = get<float32> (&NH.srow_x[3], is_BE);

            M (1,0) = get<float32> (&NH.srow_y[0], is_BE);
            M (1,1) = get<float32> (&NH.srow_y[1], is_BE);
            M (1,2) = get<float32> (&NH.srow_y[2], is_BE);
            M (1,3) = get<float32> (&NH.srow_y[3], is_BE);

            M (2,0) = get<float32> (&NH.srow_z[0], is_BE);
            M (2,1) = get<float32> (&NH.srow_z[1], is_BE);
            M (2,2) = get<float32> (&NH.srow_z[2], is_BE);
            M (2,3) = get<float32> (&NH.srow_z[3], is_BE);

            M (3,0) = M (3,1) = M (3,2) = 0.0;
            M (3,3) = 1.0;

            // get voxel sizes:
            H.vox(0) = std::sqrt (Math::pow2 (M(0,0)) + Math::pow2 (M(1,0)) + Math::pow2 (M(2,0)));
            H.vox(1) = std::sqrt (Math::pow2 (M(0,1)) + Math::pow2 (M(1,1)) + Math::pow2 (M(2,1)));
            H.vox(2) = std::sqrt (Math::pow2 (M(0,2)) + Math::pow2 (M(1,2)) + Math::pow2 (M(2,2)));

            // normalize each transform axis:
            M (0,0) /= H.vox (0);
            M (1,0) /= H.vox (0);
            M (2,0) /= H.vox (0);

            M (0,1) /= H.vox (1);
            M (1,1) /= H.vox (1);
            M (2,1) /= H.vox (1);

            M (0,2) /= H.vox (2);
            M (1,2) /= H.vox (2);
            M (2,2) /= H.vox (2);
          }
          else if (get<int16_t> (&NH.qform_code, is_BE)) {
            H.transform().allocate(4,4);

            {
              Math::Versor<double> Q (get<float32> (&NH.quatern_b, is_BE), get<float32> (&NH.quatern_c, is_BE), get<float32> (&NH.quatern_d, is_BE));
              Math::Matrix<double> R (3,3);
              Q.to_matrix (R);
              H.transform().sub (0,3,0,3) = R;
            }

            H.transform()(0,3) = get<float32> (&NH.qoffset_x, is_BE);
            H.transform()(1,3) = get<float32> (&NH.qoffset_y, is_BE);
            H.transform()(2,3) = get<float32> (&NH.qoffset_z, is_BE);

            H.transform()(3,0) = H.transform()(3,1) = H.transform()(3,2) = 0.0;
            H.transform()(3,3) = 1.0;

            // qfac:
            float qfac = get<float32> (&NH.pixdim[0], is_BE) >= 0.0 ? 1.0 : -1.0;
            if (qfac < 0.0) {
              H.transform()(0,2) *= qfac;
              H.transform()(1,2) *= qfac;
              H.transform()(2,2) *= qfac;
            }
          }
        }
        else {
          H.transform().clear();
          //CONF option: Analyse.LeftToRight
          //CONF default: 0 (false)
          //CONF A boolean value to indicate whether images in Analyse format
          //CONF should be assumed to be in LAS orientation (default) or RAS
          //CONF (when this is option is turned on).
          if (!File::Config::get_bool ("Analyse.LeftToRight", false))
            H.stride(0) = -H.stride (0);
          if (!right_left_warning_issued) {
            INFO ("assuming Analyse images are encoded " + std::string (H.stride (0) >0 ? "left to right" : "right to left"));
            right_left_warning_issued = true;
          }
        }

        return data_offset;
      }







      void check (Image::Header& H, bool has_nii_suffix)
      {
        for (size_t i = 0; i < H.ndim(); ++i)
          if (H.dim (i) < 1)
            H.dim(i) = 1;

        // ensure first 3 axes correspond to spatial dimensions
        // while preserving original strides as much as possible
        ssize_t max_spatial_stride = 0;
        for (size_t n = 0; n < 3; ++n)
          if (std::abs(H.stride(n)) > max_spatial_stride)
            max_spatial_stride = std::abs(H.stride(n));
        for (size_t n = 3; n < H.ndim(); ++n)
          H.stride(n) += H.stride(n) > 0 ? max_spatial_stride : -max_spatial_stride;
        Image::Stride::symbolise (H);

        // if .img, reset all strides to defaults, since it can't be assumed
        // that downstream software will be able to parse the NIfTI transform 
        if (!has_nii_suffix) {
          for (size_t i = 0; i < H.ndim(); ++i)
            H.stride(i) = i+1;
          bool analyse_left_to_right = File::Config::get_bool ("Analyse.LeftToRight", false);
          if (analyse_left_to_right)
            H.stride(0) = -H.stride (0);

          if (!right_left_warning_issued) {
            INFO ("assuming Analyse images are encoded " + std::string (analyse_left_to_right ? "left to right" : "right to left"));
            right_left_warning_issued = true;
          }
        }

        // by default, prevent output of bitwise data in NIfTI, since most 3rd
        // party software package can't handle them

        //CONF option: NIFTI.AllowBitwise
        //CONF default: 0 (false)
        //CONF A boolean value to indicate whether bitwise storage of binary
        //CONF data is permitted (most 3rd party software packages don't
        //CONF support bitwise data). If false (the default), data will be
        //CONF stored using more widely supported unsigned 8-bit integers.
        if (H.datatype() == DataType::Bit) 
          if (!File::Config::get_bool ("NIFTI.AllowBitwise", false))
            H.datatype() = DataType::UInt8;
      }











      void write (nifti_1_header& NH, const Image::Header& H, bool has_nii_suffix)
      {
        if (H.ndim() > 7)
          throw Exception ("NIfTI-1.1 format cannot support more than 7 dimensions for image \"" + H.name() + "\"");

        bool is_BE = H.datatype().is_big_endian();

        std::vector<size_t> axes;
        Math::Matrix<float> M = adjust_transform (H, axes);


        memset (&NH, 0, sizeof (NH));

        // magic number:
        put<int32_t> (348, &NH.sizeof_hdr, is_BE);

        strncpy ( (char*) &NH.db_name, H.comments().size() ? H.comments() [0].c_str() : "untitled\0\0\0\0\0\0\0\0\0\0\0", 18);
        put<int32_t> (16384, &NH.extents, is_BE);
        NH.regular = 'r';
        NH.dim_info = 0;

        // data set dimensions:
        put<int16_t> (H.ndim(), &NH.dim[0], is_BE);
        for (size_t i = 0; i < 3; i++)
          put<int16_t> (H.dim (axes[i]), &NH.dim[i+1], is_BE);
        for (size_t i = 3; i < H.ndim(); i++)
          put<int16_t> (H.dim (i), &NH.dim[i+1], is_BE);

        // data type:
        int16_t dt = 0;
        switch (H.datatype() ()) {
          case DataType::Bit:
            dt = DT_BINARY;
            break;
          case DataType::Int8:
            dt = DT_INT8;
            break;
          case DataType::UInt8:
            dt = DT_UINT8;
            break;
          case DataType::Int16LE:
          case DataType::Int16BE:
            dt = DT_INT16;
            break;
          case DataType::UInt16LE:
          case DataType::UInt16BE:
            dt = DT_UINT16;
            break;
          case DataType::Int32LE:
          case DataType::Int32BE:
            dt = DT_INT32;
            break;
          case DataType::UInt32LE:
          case DataType::UInt32BE:
            dt = DT_UINT32;
            break;
          case DataType::Int64LE:
          case DataType::Int64BE:
            dt = DT_INT64;
            break;
          case DataType::UInt64LE:
          case DataType::UInt64BE:
            dt = DT_UINT64;
            break;
          case DataType::Float32LE:
          case DataType::Float32BE:
            dt = DT_FLOAT32;
            break;
          case DataType::Float64LE:
          case DataType::Float64BE:
            dt = DT_FLOAT64;
            break;
          case DataType::CFloat32LE:
          case DataType::CFloat32BE:
            dt = DT_COMPLEX64;
            break;
          case DataType::CFloat64LE:
          case DataType::CFloat64BE:
            dt = DT_COMPLEX128;
            break;
          default:
            throw Exception ("unknown data type for NIfTI-1.1 image \"" + H.name() + "\"");
        }

        put<int16_t> (dt, &NH.datatype, is_BE);
        put<int16_t> (H.datatype().bits(), &NH.bitpix, is_BE);
        NH.pixdim[0] = 1.0;

        // voxel sizes:
        for (size_t i = 0; i < 3; ++i)
          put<float32> (H.vox (axes[i]), &NH.pixdim[i+1], is_BE);
        for (size_t i = 3; i < H.ndim(); ++i)
          put<float32> (H.vox (i), &NH.pixdim[i+1], is_BE);

        put<float32> (352.0, &NH.vox_offset, is_BE);

        // offset and scale:
        put<float32> (H.intensity_scale(), &NH.scl_slope, is_BE);
        put<float32> (H.intensity_offset(), &NH.scl_inter, is_BE);

        NH.xyzt_units = SPACE_TIME_TO_XYZT (NIFTI_UNITS_MM, NIFTI_UNITS_SEC);

        memset ((char*) &NH.descrip, 0, 80);
        std::string version_string = std::string("MRtrix version: ") + App::mrtrix_version;
        if (App::project_version)
          version_string += std::string(", project version: ") + App::project_version;
        strncpy ( (char*) &NH.descrip, version_string.c_str(), 79);

        put<int16_t> (NIFTI_XFORM_SCANNER_ANAT, &NH.qform_code, is_BE);
        put<int16_t> (NIFTI_XFORM_SCANNER_ANAT, &NH.sform_code, is_BE);

        // qform:
        Math::Matrix<double> R = M.sub(0,3,0,3);
        if (Math::LU::sgndet (R) < 0.0) {
          R.column (2) *= -1.0;
          NH.pixdim[0] = -1.0;
        }
        const Math::Versor<double> Q (R);

        put<float32> (Q[1], &NH.quatern_b, is_BE);
        put<float32> (Q[2], &NH.quatern_c, is_BE);
        put<float32> (Q[3], &NH.quatern_d, is_BE);


        // sform:

        put<float32> (M(0,3), &NH.qoffset_x, is_BE);
        put<float32> (M(1,3), &NH.qoffset_y, is_BE);
        put<float32> (M(2,3), &NH.qoffset_z, is_BE);

        put<float32> (H.vox (axes[0]) * M(0,0), &NH.srow_x[0], is_BE);
        put<float32> (H.vox (axes[1]) * M(0,1), &NH.srow_x[1], is_BE);
        put<float32> (H.vox (axes[2]) * M(0,2), &NH.srow_x[2], is_BE);
        put<float32> (M (0,3), &NH.srow_x[3], is_BE);

        put<float32> (H.vox (axes[0]) * M(1,0), &NH.srow_y[0], is_BE);
        put<float32> (H.vox (axes[1]) * M(1,1), &NH.srow_y[1], is_BE);
        put<float32> (H.vox (axes[2]) * M(1,2), &NH.srow_y[2], is_BE);
        put<float32> (M (1,3), &NH.srow_y[3], is_BE);

        put<float32> (H.vox (axes[0]) * M(2,0), &NH.srow_z[0], is_BE);
        put<float32> (H.vox (axes[1]) * M(2,1), &NH.srow_z[1], is_BE);
        put<float32> (H.vox (axes[2]) * M(2,2), &NH.srow_z[2], is_BE);
        put<float32> (M (2,3), &NH.srow_z[3], is_BE);

        strncpy ( (char*) &NH.magic, has_nii_suffix ? "n+1\0" : "ni1\0", 4);
      }

    }
  }
}


