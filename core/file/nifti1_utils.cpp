/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "header.h"
#include "raw.h"
#include "file/config.h"
#include "file/json_utils.h"
#include "file/nifti_utils.h"
#include "file/nifti1_utils.h"

namespace MR
{
  namespace File
  {
    namespace NIfTI1
    {



      size_t read (Header& H, const nifti_1_header& NH)
      {
        bool is_BE = false;
        if (Raw::fetch_<int32_t> (&NH.sizeof_hdr, is_BE) != header_size) {
          is_BE = true;
          if (Raw::fetch_<int32_t> (&NH.sizeof_hdr, is_BE) != header_size)
            throw Exception ("image \"" + H.name() + "\" is not in NIfTI-1.1 format (sizeof_hdr != " + str(header_size) + ")");
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
          add_line (H.keyval()["comments"], db_name);
        }

        // data set dimensions:
        int ndim = Raw::fetch_<int16_t> (&NH.dim, is_BE);
        if (ndim < 1)
          throw Exception ("too few dimensions specified in NIfTI-1.1 image \"" + H.name() + "\"");
        if (ndim > 7)
          throw Exception ("too many dimensions specified in NIfTI-1.1 image \"" + H.name() + "\"");
        H.ndim() = ndim;


        for (int i = 0; i < ndim; i++) {
          H.size(i) = Raw::fetch_<int16_t> (&NH.dim[i+1], is_BE);
          if (H.size (i) < 0) {
            INFO ("dimension along axis " + str (i) + " specified as negative in NIfTI-1.1 image \"" + H.name() + "\" - taking absolute value");
            H.size(i) = abs (H.size (i));
          }
          if (!H.size (i))
            H.size(i) = 1;
          H.stride(i) = i+1;
        }

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
            throw Exception ("unknown data type for NIfTI-1.1 image \"" + H.name() + "\"");
        }

        if (! (dtype.is (DataType::Bit) || dtype.is (DataType::UInt8) || dtype.is (DataType::Int8))) {
          if (is_BE)
            dtype.set_flag (DataType::BigEndian);
          else
            dtype.set_flag (DataType::LittleEndian);
        }

        if (Raw::fetch_<int16_t> (&NH.bitpix, is_BE) != (int16_t) dtype.bits())
          WARN ("bitpix field does not match data type in NIfTI-1.1 image \"" + H.name() + "\" - ignored");

        H.datatype() = dtype;

        // voxel sizes:
        for (int i = 0; i < ndim; i++) {
          H.spacing(i) = Raw::fetch_<float32> (&NH.pixdim[i+1], is_BE);
          if (H.spacing (i) < 0.0) {
            INFO ("voxel size along axis " + str (i) + " specified as negative in NIfTI-1.1 image \"" + H.name() + "\" - taking absolute value");
            H.spacing(i) = abs (H.spacing (i));
          }
        }


        // offset and scale:
        H.intensity_scale() = Raw::fetch_<float32> (&NH.scl_slope, is_BE);
        if (std::isfinite (H.intensity_scale()) && H.intensity_scale() != 0.0) {
          H.intensity_offset() = Raw::fetch_<float32> (&NH.scl_inter, is_BE);
          if (!std::isfinite (H.intensity_offset()))
            H.intensity_offset() = 0.0;
        }
        else
          H.reset_intensity_scaling();

        size_t data_offset = (size_t) Raw::fetch_<float32> (&NH.vox_offset, is_BE);

        char descrip[81];
        strncpy (descrip, NH.descrip, 80);
        if (descrip[0]) {
          descrip[80] = '\0';
          if (strncmp (descrip, "MRtrix version: ", 16) == 0)
            H.keyval()["mrtrix_version"] = descrip+16;
          else
            add_line (H.keyval()["comments"], descrip);
        }

        if (is_nifti) {
          bool sform_code = Raw::fetch_<int16_t> (&NH.sform_code, is_BE);
          if (sform_code) {
            auto& M (H.transform().matrix());

            M(0,0) = Raw::fetch_<float32> (&NH.srow_x[0], is_BE);
            M(0,1) = Raw::fetch_<float32> (&NH.srow_x[1], is_BE);
            M(0,2) = Raw::fetch_<float32> (&NH.srow_x[2], is_BE);
            M(0,3) = Raw::fetch_<float32> (&NH.srow_x[3], is_BE);

            M(1,0) = Raw::fetch_<float32> (&NH.srow_y[0], is_BE);
            M(1,1) = Raw::fetch_<float32> (&NH.srow_y[1], is_BE);
            M(1,2) = Raw::fetch_<float32> (&NH.srow_y[2], is_BE);
            M(1,3) = Raw::fetch_<float32> (&NH.srow_y[3], is_BE);

            M(2,0) = Raw::fetch_<float32> (&NH.srow_z[0], is_BE);
            M(2,1) = Raw::fetch_<float32> (&NH.srow_z[1], is_BE);
            M(2,2) = Raw::fetch_<float32> (&NH.srow_z[2], is_BE);
            M(2,3) = Raw::fetch_<float32> (&NH.srow_z[3], is_BE);

            // check voxel sizes:
            for (size_t axis = 0; axis != 3; ++axis) {
              if (size_t(ndim) > axis)
                if (abs(H.spacing(axis) - M.col(axis).head<3>().norm()) > 1e-4) {
                    WARN ("voxel spacings inconsistent between NIFTI s-form and header field pixdim");
                    break;
                }
            }

            // normalize each transform axis:
            for (size_t axis = 0; axis != 3; ++axis) {
              if (size_t(ndim) > axis)
                M.col(axis).normalize();
            }

          }

          if (Raw::fetch_<int16_t> (&NH.qform_code, is_BE)) {
            transform_type M_qform;

            Eigen::Quaterniond Q (0.0, Raw::fetch_<float32> (&NH.quatern_b, is_BE), Raw::fetch_<float32> (&NH.quatern_c, is_BE), Raw::fetch_<float32> (&NH.quatern_d, is_BE));
            const double w = 1.0 - Q.squaredNorm();
            if (w < 1.0e-7)
              Q.normalize();
            else
              Q.w() = std::sqrt (w);
            M_qform.matrix().topLeftCorner<3,3>() = Q.matrix();

            M_qform.translation()[0] = Raw::fetch_<float32> (&NH.qoffset_x, is_BE);
            M_qform.translation()[1] = Raw::fetch_<float32> (&NH.qoffset_y, is_BE);
            M_qform.translation()[2] = Raw::fetch_<float32> (&NH.qoffset_z, is_BE);

            // qfac:
            float qfac = Raw::fetch_<float32> (&NH.pixdim[0], is_BE) >= 0.0 ? 1.0 : -1.0;
            if (qfac < 0.0)
              M_qform.matrix().col(2) *= qfac;

            if (sform_code) {
              Header header2 (H);
              header2.transform() = M_qform;
              if (!voxel_grids_match_in_scanner_space (H, header2, 0.1)) {
                //CONF option: NIfTIUseSform
                //CONF default: 0 (false)
                //CONF A boolean value to control whether, in cases where both
                //CONF the sform and qform transformations are defined in an
                //CONF input NIfTI image, but those transformations differ, the
                //CONF sform transformation should be used in preference to the
                //CONF qform matrix (the default behaviour).
                const bool use_sform = File::Config::get_bool ("NIfTIUseSform", false);
                WARN ("qform and sform are inconsistent in NIfTI image \"" + H.name() + "\" - using " + (use_sform ? "sform" : "qform"));
                if (!use_sform)
                  H.transform() = M_qform;
              }
            }
            else
              H.transform() = M_qform;
          }

          //CONF option: NIfTIAutoLoadJSON
          //CONF default: 0 (false)
          //CONF A boolean value to indicate whether, when opening NIfTI images,
          //CONF any corresponding JSON file should be automatically loaded.
          if (File::Config::get_bool ("NIfTIAutoLoadJSON", false)) {
            std::string json_path = H.name();
            if (Path::has_suffix (json_path, ".nii.gz"))
              json_path = json_path.substr (0, json_path.size()-7);
            else if (Path::has_suffix (json_path, ".nii"))
              json_path = json_path.substr (0, json_path.size()-4);
            else
              assert (0);
            json_path += ".json";
            if (Path::exists (json_path))
              File::JSON::load (H, json_path);
          }

        }
        else {
          H.transform()(0,0) = std::numeric_limits<default_type>::quiet_NaN();
          //CONF option: AnalyseLeftToRight
          //CONF default: 0 (false)
          //CONF A boolean value to indicate whether images in Analyse format
          //CONF should be assumed to be in LAS orientation (default) or RAS
          //CONF (when this is option is turned on).
          if (!File::Config::get_bool ("AnalyseLeftToRight", false))
            H.stride(0) = -H.stride (0);
          if (!File::NIfTI::right_left_warning_issued) {
            INFO ("assuming Analyse images are encoded " + std::string (H.stride (0) > 0 ? "left to right" : "right to left"));
            File::NIfTI::right_left_warning_issued = true;
          }
        }

        return data_offset;
      }







      void write (nifti_1_header& NH, const Header& H, const bool single_file)
      {
        if (H.ndim() > 7)
          throw Exception ("NIfTI-1.1 format cannot support more than 7 dimensions for image \"" + H.name() + "\"");

        bool is_BE = H.datatype().is_big_endian();

        vector<size_t> axes;
        auto M = File::NIfTI::adjust_transform (H, axes);

        memset (&NH, 0, sizeof (NH));

        // magic number:
        Raw::store<int32_t> (header_size, &NH.sizeof_hdr, is_BE);

        const auto hit = H.keyval().find("comments");
        auto comments = split_lines (hit == H.keyval().end() ? std::string() : hit->second);
        strncpy ( (char*) &NH.db_name, comments.size() ? comments[0].c_str() : "untitled\0\0\0\0\0\0\0\0\0\0\0", 18);
        Raw::store<int32_t> (16384, &NH.extents, is_BE);
        NH.regular = 'r';
        NH.dim_info = 0;

        // data set dimensions:
        Raw::store<int16_t> (H.ndim(), &NH.dim[0], is_BE);
        {
          size_t i = 0;
          for (; i < 3; i++)
            Raw::store<int16_t> (H.size (axes[i]), &NH.dim[i+1], is_BE);
          for (; i < H.ndim(); i++)
            Raw::store<int16_t> (H.size (i), &NH.dim[i+1], is_BE);

          // pad out the other dimensions with 1, fix for fslview
          ++i;
          for (; i < 8; i++)
            Raw::store<int16_t> (1, &NH.dim[i], is_BE);
        }


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
            throw Exception ("unknown data type for NIfTI-1.1 image \"" + H.name() + "\"");
        }

        Raw::store<int16_t> (dt, &NH.datatype, is_BE);
        Raw::store<int16_t> (H.datatype().bits(), &NH.bitpix, is_BE);

        Raw::store<float32> (1.0, &NH.pixdim[0], is_BE);
        // voxel sizes:
        for (size_t i = 0; i < 3; ++i)
          Raw::store<float32> (H.spacing (axes[i]), &NH.pixdim[i+1], is_BE);
        for (size_t i = 3; i < H.ndim(); ++i)
          Raw::store<float32> (H.spacing (i), &NH.pixdim[i+1], is_BE);

        Raw::store<float32> (float32(header_with_ext_size), &NH.vox_offset, is_BE);

        // offset and scale:
        Raw::store<float32> (H.intensity_scale(), &NH.scl_slope, is_BE);
        Raw::store<float32> (H.intensity_offset(), &NH.scl_inter, is_BE);

        NH.xyzt_units = SPACE_TIME_TO_XYZT (NIFTI_UNITS_MM, NIFTI_UNITS_SEC);

        memset ((char*) &NH.descrip, 0, 80);
        std::string version_string = std::string("MRtrix version: ") + App::mrtrix_version;
        if (App::project_version)
          version_string += std::string(", project version: ") + App::project_version;
        strncpy ( (char*) &NH.descrip, version_string.c_str(), 79);

        Raw::store<int16_t> (NIFTI_XFORM_SCANNER_ANAT, &NH.qform_code, is_BE);
        Raw::store<int16_t> (NIFTI_XFORM_SCANNER_ANAT, &NH.sform_code, is_BE);

        // qform:
        Eigen::Matrix3d R = M.matrix().topLeftCorner<3,3>();
        if (R.determinant() < 0.0) {
          R.col(2) = -R.col(2);
          Raw::store<float32> (-1.0, &NH.pixdim[0], is_BE);
        }
        Eigen::Quaterniond Q (R);

        if (Q.w() < 0.0)
          Q.vec() = -Q.vec();

        Raw::store<float32> (Q.x(), &NH.quatern_b, is_BE);
        Raw::store<float32> (Q.y(), &NH.quatern_c, is_BE);
        Raw::store<float32> (Q.z(), &NH.quatern_d, is_BE);


        // sform:

        Raw::store<float32> (M(0,3), &NH.qoffset_x, is_BE);
        Raw::store<float32> (M(1,3), &NH.qoffset_y, is_BE);
        Raw::store<float32> (M(2,3), &NH.qoffset_z, is_BE);

        Raw::store<float32> (H.spacing (axes[0]) * M(0,0), &NH.srow_x[0], is_BE);
        Raw::store<float32> (H.spacing (axes[1]) * M(0,1), &NH.srow_x[1], is_BE);
        Raw::store<float32> (H.spacing (axes[2]) * M(0,2), &NH.srow_x[2], is_BE);
        Raw::store<float32> (M (0,3), &NH.srow_x[3], is_BE);

        Raw::store<float32> (H.spacing (axes[0]) * M(1,0), &NH.srow_y[0], is_BE);
        Raw::store<float32> (H.spacing (axes[1]) * M(1,1), &NH.srow_y[1], is_BE);
        Raw::store<float32> (H.spacing (axes[2]) * M(1,2), &NH.srow_y[2], is_BE);
        Raw::store<float32> (M (1,3), &NH.srow_y[3], is_BE);

        Raw::store<float32> (H.spacing (axes[0]) * M(2,0), &NH.srow_z[0], is_BE);
        Raw::store<float32> (H.spacing (axes[1]) * M(2,1), &NH.srow_z[1], is_BE);
        Raw::store<float32> (H.spacing (axes[2]) * M(2,2), &NH.srow_z[2], is_BE);
        Raw::store<float32> (M (2,3), &NH.srow_z[3], is_BE);

        strncpy ( (char*) &NH.magic, single_file ? "n+1\0" : "ni1\0", 4);

        //CONF option: NIfTIAutoSaveJSON
        //CONF default: 0 (false)
        //CONF A boolean value to indicate whether, when writing NIfTI images,
        //CONF a corresponding JSON file should be automatically created in order
        //CONF to save any header entries that cannot be stored in the NIfTI
        //CONF header.
        if (single_file && File::Config::get_bool ("NIfTIAutoSaveJSON", false)) {
          std::string json_path = H.name();
          if (Path::has_suffix (json_path, ".nii.gz"))
            json_path = json_path.substr (0, json_path.size()-7);
          else if (Path::has_suffix (json_path, ".nii"))
            json_path = json_path.substr (0, json_path.size()-4);
          else
            assert (0);
          json_path += ".json";
          File::JSON::save (H, json_path);
        }
      }



    }
  }
}

