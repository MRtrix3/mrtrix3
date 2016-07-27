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
#include "file/config.h"
#include "file/nifti1.h"
#include "file/nifti_utils.h"
#include "file/nifti2_utils.h"

namespace MR
{
  namespace File
  {
    namespace NIfTI2
    {



      size_t read (Header& H, const nifti_2_header& NH)
      {
        bool is_BE = false;
        if (Raw::fetch_<int32_t> (&NH.sizeof_hdr, is_BE) != header_size) {
          is_BE = true;
          if (Raw::fetch_<int32_t> (&NH.sizeof_hdr, is_BE) != header_size)
            throw Exception ("image \"" + H.name() + "\" is not in NIfTI-2 format (sizeof_hdr != " + str(header_size) + ")");
        }

        if (memcmp (NH.magic, "n+2\0", 4) && memcmp (NH.magic, "ni2\0", 4))
          throw Exception ("image \"" + H.name() + "\" is not in NIfTI-2 format (invalid magic signature)");

        if (memcmp (NH.magic+4, signature_extra, 4))
          WARN ("possible file transfer corruption of file \"" + H.name() + "\" (invalid magic signature)");

        // data type:
        DataType dtype;
        switch (Raw::fetch_<int16_t> (&NH.datatype, is_BE)) {
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
            throw Exception ("unknown data type for NIfTI-2 image \"" + H.name() + "\"");
        }

        if (! (dtype.is (DataType::Bit) || dtype.is (DataType::UInt8) || dtype.is (DataType::Int8))) {
          if (is_BE)
            dtype.set_flag (DataType::BigEndian);
          else
            dtype.set_flag (DataType::LittleEndian);
        }

        H.datatype() = dtype;

        if (Raw::fetch_<int16_t> (&NH.bitpix, is_BE) != (int16_t) dtype.bits())
          WARN ("bitpix field does not match data type in NIfTI-2 image \"" + H.name() + "\" - ignored");

        // data set dimensions:
        const int64_t ndim = Raw::fetch_<int64_t> (&NH.dim, is_BE);
        if (ndim < 1)
          throw Exception ("too few dimensions specified in NIfTI-2 image \"" + H.name() + "\"");
        if (ndim > 7)
          throw Exception ("too many dimensions specified in NIfTI-2 image \"" + H.name() + "\"");
        H.ndim() = ndim;
        for (int64_t i = 0; i != ndim; i++) {
          H.size(i) = Raw::fetch_<int64_t> (&NH.dim[i+1], is_BE);
          if (H.size (i) < 0) {
            INFO ("dimension along axis " + str (i) + " specified as negative in NIfTI-2 image \"" + H.name() + "\" - taking absolute value");
            H.size(i) = std::abs (H.size (i));
          }
          if (!H.size (i))
            H.size(i) = 1;
          H.stride(i) = i+1;
        }

        // voxel sizes:
        for (int i = 0; i < ndim; i++) {
          H.spacing(i) = Raw::fetch_<float64> (&NH.pixdim[i+1], is_BE);
          if (H.spacing (i) < 0.0) {
            INFO ("voxel size along axis " + str (i) + " specified as negative in NIfTI-2 image \"" + H.name() + "\" - taking absolute value");
            H.spacing(i) = std::abs (H.spacing (i));
          }
        }

        const int64_t data_offset = Raw::fetch_<int64_t> (&NH.vox_offset, is_BE);

        // offset and scale:
        H.intensity_scale() = Raw::fetch_<float64> (&NH.scl_slope, is_BE);
        if (std::isfinite (H.intensity_scale()) && H.intensity_scale() != 0.0) {
          H.intensity_offset() = Raw::fetch_<float64> (&NH.scl_inter, is_BE);
          if (!std::isfinite (H.intensity_offset()))
            H.intensity_offset() = 0.0;
        } else {
          H.reset_intensity_scaling();
        }

        char descrip[81];
        strncpy (descrip, NH.descrip, 80);
        if (descrip[0]) {
          descrip[80] = '\0';
          if (strncmp (descrip, "MRtrix version: ", 16) == 0)
            H.keyval()["mrtrix_version"] = descrip+16;
          else
            add_line (H.keyval()["comments"], descrip);
        }

        // Note: Unlike reading from a nifti_1_header class, here we
        //   don't have to worry about whether or not the file is in
        //   Analyse format; we can treat it as a NIfTI regardless of
        //   whether the hedaer & data are in the same file or not.
        if (Raw::fetch_<int32_t> (&NH.sform_code, is_BE)) {
          auto& M (H.transform().matrix());

          M(0,0) = Raw::fetch_<float64> (&NH.srow_x[0], is_BE);
          M(0,1) = Raw::fetch_<float64> (&NH.srow_x[1], is_BE);
          M(0,2) = Raw::fetch_<float64> (&NH.srow_x[2], is_BE);
          M(0,3) = Raw::fetch_<float64> (&NH.srow_x[3], is_BE);

          M(1,0) = Raw::fetch_<float64> (&NH.srow_y[0], is_BE);
          M(1,1) = Raw::fetch_<float64> (&NH.srow_y[1], is_BE);
          M(1,2) = Raw::fetch_<float64> (&NH.srow_y[2], is_BE);
          M(1,3) = Raw::fetch_<float64> (&NH.srow_y[3], is_BE);

          M(2,0) = Raw::fetch_<float64> (&NH.srow_z[0], is_BE);
          M(2,1) = Raw::fetch_<float64> (&NH.srow_z[1], is_BE);
          M(2,2) = Raw::fetch_<float64> (&NH.srow_z[2], is_BE);
          M(2,3) = Raw::fetch_<float64> (&NH.srow_z[3], is_BE);

          // get voxel sizes:
          H.spacing(0) = std::sqrt (Math::pow2 (M(0,0)) + Math::pow2 (M(1,0)) + Math::pow2 (M(2,0)));
          H.spacing(1) = std::sqrt (Math::pow2 (M(0,1)) + Math::pow2 (M(1,1)) + Math::pow2 (M(2,1)));
          H.spacing(2) = std::sqrt (Math::pow2 (M(0,2)) + Math::pow2 (M(1,2)) + Math::pow2 (M(2,2)));

          // normalize each transform axis:
          M (0,0) /= H.spacing (0);
          M (1,0) /= H.spacing (0);
          M (2,0) /= H.spacing (0);

          M (0,1) /= H.spacing (1);
          M (1,1) /= H.spacing (1);
          M (2,1) /= H.spacing (1);

          M (0,2) /= H.spacing (2);
          M (1,2) /= H.spacing (2);
          M (2,2) /= H.spacing (2);

        } else if (Raw::fetch_<int32_t> (&NH.qform_code, is_BE)) {
          { // TODO update with Eigen3 Quaternions
            Eigen::Quaterniond Q (0.0, Raw::fetch_<float64> (&NH.quatern_b, is_BE), Raw::fetch_<float64> (&NH.quatern_c, is_BE), Raw::fetch_<float64> (&NH.quatern_d, is_BE));
            Q.w() = std::sqrt (std::max (1.0 - Q.squaredNorm(), 0.0));
            H.transform().matrix().topLeftCorner<3,3>() = Q.matrix();
          }

          H.transform().translation()[0] = Raw::fetch_<float64> (&NH.qoffset_x, is_BE);
          H.transform().translation()[1] = Raw::fetch_<float64> (&NH.qoffset_y, is_BE);
          H.transform().translation()[2] = Raw::fetch_<float64> (&NH.qoffset_z, is_BE);

          // qfac:
          const float64 qfac = Raw::fetch_<float64> (&NH.pixdim[0], is_BE) >= 0.0 ? 1.0 : -1.0;
          if (qfac < 0.0)
            H.transform().matrix().col(2) *= qfac;
        }

        return data_offset;
      }






      void write (nifti_2_header& NH, const Header& H, const bool single_file)
      {
        if (H.ndim() > 7)
          throw Exception ("NIfTI-2 format cannot support more than 7 dimensions for image \"" + H.name() + "\"");

        bool is_BE = H.datatype().is_big_endian();

        std::vector<size_t> axes;
        auto M = File::NIfTI::adjust_transform (H, axes);


        memset (&NH, 0, sizeof (NH));

        // magic number:
        Raw::store<int32_t> (header_size, &NH.sizeof_hdr, is_BE);

        strncpy ( (char*) &NH.magic, single_file ? "n+2\0" : "ni2\0", 4);
        strncpy ( (char*) &NH.magic+4, signature_extra, 4);

        // data type:
        int16_t dt = 0;
        switch (H.datatype()()) {
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
            throw Exception ("unknown data type for NIfTI-2 image \"" + H.name() + "\"");
        }
        Raw::store<int16_t> (dt, &NH.datatype, is_BE);

        Raw::store<int16_t> (H.datatype().bits(), &NH.bitpix, is_BE);

        // data set dimensions:
        Raw::store<int64_t> (H.ndim(), &NH.dim[0], is_BE);
        {
          size_t i = 0;
          for (; i < 3; i++)
            Raw::store<int64_t> (H.size (axes[i]), &NH.dim[i+1], is_BE);
          for (; i < H.ndim(); i++)
            Raw::store<int64_t> (H.size (i), &NH.dim[i+1], is_BE);

          // pad out the other dimensions with 1, fix for fslview
          ++i;
          for (; i < 8; i++)
            Raw::store<int64_t> (1, &NH.dim[i], is_BE);
        }

        Raw::store<float64> (1.0, &NH.pixdim[0], is_BE);
        // voxel sizes:
        for (size_t i = 0; i < 3; ++i)
          Raw::store<float64> (H.spacing (axes[i]), &NH.pixdim[i+1], is_BE);
        for (size_t i = 3; i < H.ndim(); ++i)
          Raw::store<float64> (H.spacing (i), &NH.pixdim[i+1], is_BE);

        Raw::store<int64_t> (int64_t(header_with_ext_size), &NH.vox_offset, is_BE);

        // offset and scale:
        Raw::store<float64> (H.intensity_scale(), &NH.scl_slope, is_BE);
        Raw::store<float64> (H.intensity_offset(), &NH.scl_inter, is_BE);

        std::string version_string = std::string("MRtrix version: ") + App::mrtrix_version;
        if (App::project_version)
          version_string += std::string(", project version: ") + App::project_version;
        strncpy ( (char*) &NH.descrip, version_string.c_str(), 79);

        Raw::store<int32_t> (NIFTI_XFORM_SCANNER_ANAT, &NH.qform_code, is_BE);
        Raw::store<int32_t> (NIFTI_XFORM_SCANNER_ANAT, &NH.sform_code, is_BE);

        // qform:
        Eigen::Matrix3d R = M.matrix().topLeftCorner<3,3>();
        if (R.determinant() < 0.0) {
          R.col(2) = -R.col(2);
          Raw::store<float64> (-1.0, &NH.pixdim[0], is_BE);
        }
        Eigen::Quaterniond Q (R);

        if (Q.w() < 0.0)
          Q.vec() = -Q.vec();

        Raw::store<float64> (Q.x(), &NH.quatern_b, is_BE);
        Raw::store<float64> (Q.y(), &NH.quatern_c, is_BE);
        Raw::store<float64> (Q.z(), &NH.quatern_d, is_BE);

        Raw::store<float64> (M(0,3), &NH.qoffset_x, is_BE);
        Raw::store<float64> (M(1,3), &NH.qoffset_y, is_BE);
        Raw::store<float64> (M(2,3), &NH.qoffset_z, is_BE);

        // sform:
        Raw::store<float64> (H.spacing (axes[0]) * M(0,0), &NH.srow_x[0], is_BE);
        Raw::store<float64> (H.spacing (axes[1]) * M(0,1), &NH.srow_x[1], is_BE);
        Raw::store<float64> (H.spacing (axes[2]) * M(0,2), &NH.srow_x[2], is_BE);
        Raw::store<float64> (M (0,3), &NH.srow_x[3], is_BE);

        Raw::store<float64> (H.spacing (axes[0]) * M(1,0), &NH.srow_y[0], is_BE);
        Raw::store<float64> (H.spacing (axes[1]) * M(1,1), &NH.srow_y[1], is_BE);
        Raw::store<float64> (H.spacing (axes[2]) * M(1,2), &NH.srow_y[2], is_BE);
        Raw::store<float64> (M (1,3), &NH.srow_y[3], is_BE);

        Raw::store<float64> (H.spacing (axes[0]) * M(2,0), &NH.srow_z[0], is_BE);
        Raw::store<float64> (H.spacing (axes[1]) * M(2,1), &NH.srow_z[1], is_BE);
        Raw::store<float64> (H.spacing (axes[2]) * M(2,2), &NH.srow_z[2], is_BE);
        Raw::store<float64> (M (2,3), &NH.srow_z[3], is_BE);

        const char xyzt_units[4] { NIFTI_UNITS_MM, NIFTI_UNITS_MM, NIFTI_UNITS_MM, NIFTI_UNITS_SEC };
        const int32_t* const xyzt_units_as_int_ptr = reinterpret_cast<const int32_t*>(xyzt_units);
        Raw::store<int32_t> (*xyzt_units_as_int_ptr, &NH.xyzt_units, is_BE);
      }



    }
  }
}

