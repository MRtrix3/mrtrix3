/* Copyright (c) 2008-2019 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include "header.h"
#include "raw.h"
#include "file/config.h"
#include "file/nifti_utils.h"
#include "file/json_utils.h"

namespace MR
{
  namespace File
  {
    namespace NIfTI
    {

      namespace {
        template <class NiftiHeaderType> struct Type {
          using code_type = int16_t;
          using float_type = float32;
          using dim_type = int16_t;
          using vox_offset_type = float32;
          static constexpr bool is_version2 = false;
          static const char* signature_extra() { return "\0\0\0\0"; }
          static const char* magic1() { return "n+1\0"; }
          static const char* magic2() { return "ni1\0"; }
          static const std::string version() { return  "NIFTI-1.1"; }
          static const char* db_name (const NiftiHeaderType& NH) { return NH.db_name; }
          static char* db_name (NiftiHeaderType& NH) { return NH.db_name; }
          static int* extents (NiftiHeaderType& NH) { return &NH.extents; }
          static char* regular (NiftiHeaderType& NH) { return &NH.regular; }
        };

        template <> struct Type<nifti_2_header> {
          using code_type = int32_t;
          using float_type = float64;
          using dim_type = int64_t;
          using vox_offset_type = int64_t;
          static constexpr bool is_version2 = true;
          static const char* signature_extra() { return "\r\n\032\n"; }
          static const char* magic1() { return "n+2\0"; }
          static const char* magic2() { return "ni2\0"; }
          static const std::string version() { return  "NIFTI-2"; }
          static const char* db_name (const nifti_2_header& NH) { return nullptr; }
          static char* db_name (nifti_2_header& NH) { return nullptr; }
          static int* extents (nifti_2_header& NH) { return nullptr; }
          static char* regular (nifti_2_header& NH) { return nullptr; }
        };

      }


      bool right_left_warning_issued = false;




      template <class H>
        size_t read (Header& MH, const H& NH)
        {
          const std::string& version = Type<H>::version();
          using dim_type = typename Type<H>::dim_type;
          using code_type = typename Type<H>::code_type;
          using float_type = typename Type<H>::float_type;

          bool is_BE = false;
          if (Raw::fetch_<int32_t> (&NH.sizeof_hdr, is_BE) != header_size(NH)) {
            is_BE = true;
            if (Raw::fetch_<int32_t> (&NH.sizeof_hdr, is_BE) != header_size(NH))
              throw Exception ("image \"" + MH.name() + "\" is not in " + version +
                  " format (sizeof_hdr != " + str(header_size(NH)) + ")");
          }

          bool is_nifti = true;
          if (memcmp (NH.magic, Type<H>::magic1(), 4) && memcmp (NH.magic, Type<H>::magic2(), 4)) {
            if (Type<H>::is_version2) {
              throw Exception ("image \"" + MH.name() + "\" is not in " + version + " format (invalid magic signature)");
            }
            else {
              is_nifti = false;
              DEBUG ("assuming image \"" + MH.name() + "\" is in AnalyseAVW format.");
            }
          }

          if (Type<H>::is_version2) {
            if (memcmp (NH.magic+4, Type<H>::signature_extra(), 4))
              WARN ("possible file transfer corruption of file \"" + MH.name() + "\" (invalid magic signature)");
          }
          else{
            char db_name[19];
            strncpy (db_name, Type<H>::db_name (NH), 18);
            if (db_name[0]) {
              db_name[18] = '\0';
              add_line (MH.keyval()["comments"], db_name);
            }
          }



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
              throw Exception ("unknown data type for " + version + " image \"" + MH.name() + "\"");
          }

          if (! (dtype.is (DataType::Bit) || dtype.is (DataType::UInt8) || dtype.is (DataType::Int8))) {
            if (is_BE)
              dtype.set_flag (DataType::BigEndian);
            else
              dtype.set_flag (DataType::LittleEndian);
          }

          if (Raw::fetch_<int16_t> (&NH.bitpix, is_BE) != (int16_t) dtype.bits())
            WARN ("bitpix field does not match data type in " + version + " image \"" + MH.name() + "\" - ignored");

          MH.datatype() = dtype;




          const int ndim = Raw::fetch_<dim_type> (&NH.dim, is_BE);
          if (ndim < 1)
            throw Exception ("too few dimensions specified in NIfTI image \"" + MH.name() + "\"");
          if (ndim > 7)
            throw Exception ("too many dimensions specified in NIfTI image \"" + MH.name() + "\"");
          MH.ndim() = ndim;
          for (int i = 0; i != ndim; i++) {
            MH.size(i) = Raw::fetch_<dim_type> (&NH.dim[i+1], is_BE);
            if (MH.size (i) < 0) {
              INFO ("dimension along axis " + str (i) + " specified as negative in NIfTI image \"" + MH.name() + "\" - taking absolute value");
              MH.size(i) = abs (MH.size (i));
            }
            if (!MH.size (i))
              MH.size(i) = 1;
            MH.stride(i) = i+1;
          }

          // voxel sizes:
          for (int i = 0; i < ndim; i++) {
            MH.spacing(i) = Raw::fetch_<float_type> (&NH.pixdim[i+1], is_BE);
            if (MH.spacing (i) < 0.0) {
              INFO ("voxel size along axis " + str (i) + " specified as negative in NIfTI image \"" + MH.name() + "\" - taking absolute value");
              MH.spacing(i) = abs (MH.spacing (i));
            }
          }


          // offset and scale:
          MH.intensity_scale() = Raw::fetch_<float_type> (&NH.scl_slope, is_BE);
          if (std::isfinite (MH.intensity_scale()) && MH.intensity_scale() != 0.0) {
            MH.intensity_offset() = Raw::fetch_<float_type> (&NH.scl_inter, is_BE);
            if (!std::isfinite (MH.intensity_offset()))
              MH.intensity_offset() = 0.0;
          } else {
            MH.reset_intensity_scaling();
          }



          const int64_t data_offset = Raw::fetch_<float_type> (&NH.vox_offset, is_BE);



          char descrip[81];
          strncpy (descrip, NH.descrip, 80);
          if (descrip[0]) {
            descrip[80] = '\0';
            if (strncmp (descrip, "MRtrix version: ", 16) == 0)
              MH.keyval()["mrtrix_version"] = descrip+16;
            else
              add_line (MH.keyval()["comments"], descrip);
          }

          if (is_nifti) {
            bool sform_code = Raw::fetch_<code_type> (&NH.sform_code, is_BE);
            if (sform_code) {
              auto& M (MH.transform().matrix());

              M(0,0) = Raw::fetch_<float_type> (&NH.srow_x[0], is_BE);
              M(0,1) = Raw::fetch_<float_type> (&NH.srow_x[1], is_BE);
              M(0,2) = Raw::fetch_<float_type> (&NH.srow_x[2], is_BE);
              M(0,3) = Raw::fetch_<float_type> (&NH.srow_x[3], is_BE);

              M(1,0) = Raw::fetch_<float_type> (&NH.srow_y[0], is_BE);
              M(1,1) = Raw::fetch_<float_type> (&NH.srow_y[1], is_BE);
              M(1,2) = Raw::fetch_<float_type> (&NH.srow_y[2], is_BE);
              M(1,3) = Raw::fetch_<float_type> (&NH.srow_y[3], is_BE);

              M(2,0) = Raw::fetch_<float_type> (&NH.srow_z[0], is_BE);
              M(2,1) = Raw::fetch_<float_type> (&NH.srow_z[1], is_BE);
              M(2,2) = Raw::fetch_<float_type> (&NH.srow_z[2], is_BE);
              M(2,3) = Raw::fetch_<float_type> (&NH.srow_z[3], is_BE);

              // check voxel sizes:
              for (size_t axis = 0; axis != 3; ++axis) {
                if (size_t(MH.ndim()) > axis)
                  if (abs(MH.spacing(axis) - M.col(axis).head<3>().norm()) > 1e-4) {
                    WARN ("voxel spacings inconsistent between NIFTI s-form and header field pixdim");
                    break;
                  }
              }

              // normalize each transform axis:
              for (size_t axis = 0; axis != 3; ++axis) {
                if (size_t(MH.ndim()) > axis)
                  M.col(axis).normalize();
              }

            }

            if (Raw::fetch_<code_type> (&NH.qform_code, is_BE)) {
              transform_type M_qform;

              Eigen::Quaterniond Q (0.0,
                  Raw::fetch_<float_type> (&NH.quatern_b, is_BE),
                  Raw::fetch_<float_type> (&NH.quatern_c, is_BE),
                  Raw::fetch_<float_type> (&NH.quatern_d, is_BE));
              const double w = 1.0 - Q.squaredNorm();
              if (w < 1.0e-7)
                Q.normalize();
              else
                Q.w() = std::sqrt (w);
              M_qform.matrix().topLeftCorner<3,3>() = Q.matrix();

              M_qform.translation()[0] = Raw::fetch_<float_type> (&NH.qoffset_x, is_BE);
              M_qform.translation()[1] = Raw::fetch_<float_type> (&NH.qoffset_y, is_BE);
              M_qform.translation()[2] = Raw::fetch_<float_type> (&NH.qoffset_z, is_BE);

              // qfac:
              float qfac = Raw::fetch_<float_type> (&NH.pixdim[0], is_BE) >= 0.0 ? 1.0 : -1.0;
              if (qfac < 0.0)
                M_qform.matrix().col(2) *= qfac;

              if (sform_code) {
                Header header2 (MH);
                header2.transform() = M_qform;
                if (!voxel_grids_match_in_scanner_space (MH, header2, 0.1)) {
                  //CONF option: NIfTIUseSform
                  //CONF default: 1 (true)
                  //CONF A boolean value to control whether, in cases where both
                  //CONF the sform and qform transformations are defined in an
                  //CONF input NIfTI image, but those transformations differ, the
                  //CONF sform transformation should be used in preference to the
                  //CONF qform matrix.
                  const bool use_sform = File::Config::get_bool ("NIfTIUseSform", true);
                  WARN ("qform and sform are inconsistent in NIfTI image \"" + MH.name() + "\" - using " + (use_sform ? "sform" : "qform"));
                  if (!use_sform)
                    MH.transform() = M_qform;
                }
              }
              else
                MH.transform() = M_qform;
            }



            //CONF option: NIfTIAutoLoadJSON
            //CONF default: 0 (false)
            //CONF A boolean value to indicate whether, when opening NIfTI images,
            //CONF any corresponding JSON file should be automatically loaded.
            if (File::Config::get_bool ("NIfTIAutoLoadJSON", false)) {
              std::string json_path = MH.name();
              if (Path::has_suffix (json_path, ".nii.gz"))
                json_path = json_path.substr (0, json_path.size()-7);
              else if (Path::has_suffix (json_path, ".nii"))
                json_path = json_path.substr (0, json_path.size()-4);
              else
                assert (0);
              json_path += ".json";
              if (Path::exists (json_path))
                File::JSON::load (MH, json_path);
            }
          }
          else {
            MH.transform()(0,0) = std::numeric_limits<default_type>::quiet_NaN();
            //CONF option: AnalyseLeftToRight
            //CONF default: 0 (false)
            //CONF A boolean value to indicate whether images in Analyse format
            //CONF should be assumed to be in LAS orientation (default) or RAS
            //CONF (when this is option is turned on).
            if (!File::Config::get_bool ("AnalyseLeftToRight", false))
              MH.stride(0) = -MH.stride (0);
            if (!File::NIfTI::right_left_warning_issued) {
              INFO ("assuming Analyse images are encoded " + std::string (MH.stride (0) > 0 ? "left to right" : "right to left"));
              File::NIfTI::right_left_warning_issued = true;
            }
          }

          return data_offset;
        }






      template <class H>
        void write (H& NH, const Header& MH, const bool single_file)
        {
          const std::string& version = Type<H>::version();
          using dim_type = typename Type<H>::dim_type;
          using vox_offset_type = typename Type<H>::vox_offset_type;
          using code_type = typename Type<H>::code_type;
          using float_type = typename Type<H>::float_type;

          if (MH.ndim() > 7)
            throw Exception (version + " format cannot support more than 7 dimensions for image \"" + MH.name() + "\"");

          bool is_BE = MH.datatype().is_big_endian();

          vector<size_t> axes;
          auto M = File::NIfTI::adjust_transform (MH, axes);


          memset (&NH, 0, sizeof (NH));

          // magic number:
          Raw::store<int32_t> (header_size(NH), &NH.sizeof_hdr, is_BE);

          strncpy ( (char*) &NH.magic, single_file ? Type<H>::magic1() : Type<H>::magic2(), 4);
          if (Type<H>::is_version2)
            strncpy ( (char*) &NH.magic+4, Type<H>::signature_extra(), 4);

          if (!Type<H>::is_version2) {
            const auto hit = MH.keyval().find("comments");
            auto comments = split_lines (hit == MH.keyval().end() ? std::string() : hit->second);
            strncpy ( Type<H>::db_name (NH), comments.size() ? comments[0].c_str() : "untitled\0\0\0\0\0\0\0\0\0\0\0", 18);
            Raw::store<int32_t> (16384, Type<H>::extents (NH), is_BE);
            *Type<H>::regular(NH) = 'r';
          }
          NH.dim_info = 0;

          // data type:
          int16_t dt = 0;
          switch (MH.datatype()()) {
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
              throw Exception ("unknown data type for " + version + " image \"" + MH.name() + "\"");
          }
          Raw::store<int16_t> (dt, &NH.datatype, is_BE);

          Raw::store<int16_t> (MH.datatype().bits(), &NH.bitpix, is_BE);

          // data set dimensions:
          Raw::store<dim_type> (MH.ndim(), &NH.dim[0], is_BE);
          {
            size_t i = 0;
            for (; i < 3; i++)
              Raw::store<dim_type> (MH.size (axes[i]), &NH.dim[i+1], is_BE);
            for (; i < MH.ndim(); i++)
              Raw::store<dim_type> (MH.size (i), &NH.dim[i+1], is_BE);

            // pad out the other dimensions with 1, fix for fslview
            ++i;
            for (; i < 8; i++)
              Raw::store<dim_type> (1, &NH.dim[i], is_BE);
          }

          Raw::store<float_type> (1.0, &NH.pixdim[0], is_BE);
          // voxel sizes:
          for (size_t i = 0; i < 3; ++i)
            Raw::store<float_type> (MH.spacing (axes[i]), &NH.pixdim[i+1], is_BE);
          for (size_t i = 3; i < MH.ndim(); ++i)
            Raw::store<float_type> (MH.spacing (i), &NH.pixdim[i+1], is_BE);

          Raw::store<vox_offset_type> (header_size(NH)+4, &NH.vox_offset, is_BE);

          // offset and scale:
          Raw::store<float_type> (MH.intensity_scale(), &NH.scl_slope, is_BE);
          Raw::store<float_type> (MH.intensity_offset(), &NH.scl_inter, is_BE);

          std::string version_string = std::string("MRtrix version: ") + App::mrtrix_version;
          if (App::project_version)
            version_string += std::string(", project version: ") + App::project_version;
          strncpy ( (char*) &NH.descrip, version_string.c_str(), 79);

          Raw::store<code_type> (NIFTI_XFORM_SCANNER_ANAT, &NH.qform_code, is_BE);
          Raw::store<code_type> (NIFTI_XFORM_SCANNER_ANAT, &NH.sform_code, is_BE);

          // qform:
          Eigen::Matrix3d R = M.matrix().topLeftCorner<3,3>();
          if (R.determinant() < 0.0) {
            R.col(2) = -R.col(2);
            Raw::store<float_type> (-1.0, &NH.pixdim[0], is_BE);
          }
          Eigen::Quaterniond Q (R);

          if (Q.w() < 0.0)
            Q.vec() = -Q.vec();

          Raw::store<float_type> (Q.x(), &NH.quatern_b, is_BE);
          Raw::store<float_type> (Q.y(), &NH.quatern_c, is_BE);
          Raw::store<float_type> (Q.z(), &NH.quatern_d, is_BE);

          Raw::store<float_type> (M(0,3), &NH.qoffset_x, is_BE);
          Raw::store<float_type> (M(1,3), &NH.qoffset_y, is_BE);
          Raw::store<float_type> (M(2,3), &NH.qoffset_z, is_BE);

          // sform:
          Raw::store<float_type> (MH.spacing (axes[0]) * M(0,0), &NH.srow_x[0], is_BE);
          Raw::store<float_type> (MH.spacing (axes[1]) * M(0,1), &NH.srow_x[1], is_BE);
          Raw::store<float_type> (MH.spacing (axes[2]) * M(0,2), &NH.srow_x[2], is_BE);
          Raw::store<float_type> (M (0,3), &NH.srow_x[3], is_BE);

          Raw::store<float_type> (MH.spacing (axes[0]) * M(1,0), &NH.srow_y[0], is_BE);
          Raw::store<float_type> (MH.spacing (axes[1]) * M(1,1), &NH.srow_y[1], is_BE);
          Raw::store<float_type> (MH.spacing (axes[2]) * M(1,2), &NH.srow_y[2], is_BE);
          Raw::store<float_type> (M (1,3), &NH.srow_y[3], is_BE);

          Raw::store<float_type> (MH.spacing (axes[0]) * M(2,0), &NH.srow_z[0], is_BE);
          Raw::store<float_type> (MH.spacing (axes[1]) * M(2,1), &NH.srow_z[1], is_BE);
          Raw::store<float_type> (MH.spacing (axes[2]) * M(2,2), &NH.srow_z[2], is_BE);
          Raw::store<float_type> (M (2,3), &NH.srow_z[3], is_BE);

          if (Type<H>::is_version2) {
            const char xyzt_units[4] { NIFTI_UNITS_MM, NIFTI_UNITS_MM, NIFTI_UNITS_MM, NIFTI_UNITS_SEC };
            const int32_t* const xyzt_units_as_int_ptr = reinterpret_cast<const int32_t*>(xyzt_units);
            Raw::store<int32_t> (*xyzt_units_as_int_ptr, &NH.xyzt_units, is_BE);
          }
          else
            NH.xyzt_units = SPACE_TIME_TO_XYZT (NIFTI_UNITS_MM, NIFTI_UNITS_SEC);

          if (single_file && File::Config::get_bool ("NIfTI.AutoSaveJSON", false)) {
            std::string json_path = MH.name();
            if (Path::has_suffix (json_path, ".nii.gz"))
              json_path = json_path.substr (0, json_path.size()-7);
            else if (Path::has_suffix (json_path, ".nii"))
              json_path = json_path.substr (0, json_path.size()-4);
            else
              assert (0);
            json_path += ".json";
            File::JSON::save (MH, json_path);
          }
        }





      // force explicit instantiation:
      template size_t read<nifti_1_header> (Header& MH, const nifti_1_header& NH);
      template size_t read<nifti_2_header> (Header& MH, const nifti_2_header& NH);
      template void write<nifti_1_header> (nifti_1_header& NH, const Header& H, const bool single_file);
      template void write<nifti_2_header> (nifti_2_header& NH, const Header& H, const bool single_file);









      transform_type adjust_transform (const Header& H, vector<size_t>& axes)
      {
        Stride::List strides = Stride::get (H);
        strides.resize (3);
        axes = Stride::order (strides);
        bool flip[] = { strides[axes[0]] < 0, strides[axes[1]] < 0, strides[axes[2]] < 0 };

        if (axes[0] == 0 && axes[1] == 1 && axes[2] == 2 &&
            !flip[0] && !flip[1] && !flip[2])
          return H.transform();

        const auto& M_in = H.transform().matrix();
        transform_type out (H.transform());
        auto& M_out = out.matrix();

        for (size_t i = 0; i < 3; i++)
          M_out.col (i) = M_in.col (axes[i]);

        auto translation = M_out.col(3);
        for (size_t i = 0; i < 3; ++i) {
          if (flip[i]) {
            auto length = default_type (H.size (axes[i])-1) * H.spacing (axes[i]);
            auto axis = M_out.col(i);
            axis = -axis;
            translation -= length * axis;
          }
        }

        return out;
      }





      void check (Header& H, const bool is_analyse)
      {
        for (size_t i = 0; i < H.ndim(); ++i)
          if (H.size (i) < 1)
            H.size(i) = 1;

        // ensure first 3 axes correspond to spatial dimensions
        // while preserving original strides as much as possible
        ssize_t max_spatial_stride = 0;
        for (size_t n = 0; n < 3; ++n)
          if (abs(H.stride(n)) > max_spatial_stride)
            max_spatial_stride = abs(H.stride(n));
        for (size_t n = 3; n < H.ndim(); ++n)
          H.stride(n) += H.stride(n) > 0 ? max_spatial_stride : -max_spatial_stride;
        Stride::symbolise (H);

        // if .img, reset all strides to defaults, since it can't be assumed
        // that downstream software will be able to parse the NIfTI transform
        if (is_analyse) {
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

        //CONF option: NIfTIAllowBitwise
        //CONF default: 0 (false)
        //CONF A boolean value to indicate whether bitwise storage of binary
        //CONF data is permitted (most 3rd party software packages don't
        //CONF support bitwise data). If false (the default), data will be
        //CONF stored using more widely supported unsigned 8-bit integers.
        if (H.datatype() == DataType::Bit)
          if (!File::Config::get_bool ("NIfTIAllowBitwise", false))
            H.datatype() = DataType::UInt8;
      }






      size_t version (Header& H)
      {
        //CONF option: NIfTIAlwaysUseVer2
        //CONF default: 0 (false)
        //CONF A boolean value to indicate whether NIfTI images should
        //CONF always be written in the new NIfTI-2 format. If false,
        //CONF images will be written in the older NIfTI-1 format by
        //CONF default, with the exception being files where the number
        //CONF of voxels along any axis exceeds the maximum permissible
        //CONF in that format (32767), in which case the output file
        //CONF will automatically switch to the NIfTI-2 format.
        if (File::Config::get_bool ("NIfTIAlwaysUseVer2", false))
          return 2;

        for (size_t axis = 0; axis != H.ndim(); ++axis) {
          if (H.size(axis) > std::numeric_limits<int16_t>::max()) {
            INFO ("Forcing file \"" + H.name() + "\" to use NIfTI version 2 due to image dimensions");
            return 2;
          }
        }

        return 1;
      }



    }
  }
}

