/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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
#include "file/path.h"
#include "file/utils.h"
#include "file/config.h"
#include "file/nifti_utils.h"
#include "file/json_utils.h"
#include "file/gz.h"
#include "image_io/default.h"
#include "image_io/gz.h"

namespace MR
{
  namespace File
  {
    namespace NIfTI
    {

      namespace {
        template <class NiftiHeader> struct Type { NOMEMALIGN
          using code_type = int16_t;
          using float_type = float32;
          using dim_type = int16_t;
          using vox_offset_type = float32;
          static constexpr bool is_version2 = false;
          static const char* signature_extra() { return "\0\0\0\0"; }
          static const char* magic1() { return "n+1\0"; }
          static const char* magic2() { return "ni1\0"; }
          static const std::string version() { return  "NIFTI-1.1"; }
          static const char* db_name (const NiftiHeader& NH) { return NH.db_name; }
          static char* db_name (NiftiHeader& NH) { return NH.db_name; }
          static int* extents (NiftiHeader& NH) { return &NH.extents; }
          static char* regular (NiftiHeader& NH) { return &NH.regular; }
        };

        template <> struct Type<nifti_2_header> { NOMEMALIGN
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

        const vector<std::string> suffixes { ".nii", ".img" };
      }


      bool right_left_warning_issued = false;




      template <class NiftiHeader>
        size_t fetch (Header& H, const NiftiHeader& NH)
        {
          const std::string& version = Type<NiftiHeader>::version();
          using dim_type = typename Type<NiftiHeader>::dim_type;
          using code_type = typename Type<NiftiHeader>::code_type;
          using float_type = typename Type<NiftiHeader>::float_type;
          using vox_offset_type = typename Type<NiftiHeader>::vox_offset_type;

          bool is_BE = false;
          if (Raw::fetch_<int32_t> (&NH.sizeof_hdr, is_BE) != sizeof(NH)) {
            is_BE = true;
            if (Raw::fetch_<int32_t> (&NH.sizeof_hdr, is_BE) != sizeof(NH))
              throw Exception ("image \"" + H.name() + "\" is not in " + version +
                  " format (sizeof_hdr != " + str(sizeof(NH)) + ")");
          }

          bool is_nifti = true;
          if (memcmp (NH.magic, Type<NiftiHeader>::magic1(), 4) && memcmp (NH.magic, Type<NiftiHeader>::magic2(), 4)) {
            if (Type<NiftiHeader>::is_version2) {
              throw Exception ("image \"" + H.name() + "\" is not in " + version + " format (invalid magic signature)");
            }
            else {
              is_nifti = false;
              DEBUG ("assuming image \"" + H.name() + "\" is in AnalyseAVW format.");
            }
          }

          if (Type<NiftiHeader>::is_version2) {
            if (memcmp (NH.magic+4, Type<NiftiHeader>::signature_extra(), 4))
              WARN ("possible file transfer corruption of file \"" + H.name() + "\" (invalid magic signature)");
          }
          else{
            char db_name[19];
            strncpy (db_name, Type<NiftiHeader>::db_name (NH), 18);
            if (db_name[0]) {
              db_name[18] = '\0';
              add_line (H.keyval()["comments"], db_name);
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
              throw Exception ("unknown data type for " + version + " image \"" + H.name() + "\"");
          }

          if (! (dtype.is (DataType::Bit) || dtype.is (DataType::UInt8) || dtype.is (DataType::Int8))) {
            if (is_BE)
              dtype.set_flag (DataType::BigEndian);
            else
              dtype.set_flag (DataType::LittleEndian);
          }

          if (Raw::fetch_<int16_t> (&NH.bitpix, is_BE) != (int16_t) dtype.bits())
            WARN ("bitpix field does not match data type in " + version + " image \"" + H.name() + "\" - ignored");

          H.datatype() = dtype;




          const int ndim = Raw::fetch_<dim_type> (&NH.dim, is_BE);
          if (ndim < 1)
            throw Exception ("too few dimensions specified in NIfTI image \"" + H.name() + "\"");
          if (ndim > 7)
            throw Exception ("too many dimensions specified in NIfTI image \"" + H.name() + "\"");
          H.ndim() = ndim;
          for (int i = 0; i != ndim; i++) {
            H.size(i) = Raw::fetch_<dim_type> (&NH.dim[i+1], is_BE);
            if (H.size (i) < 0) {
              INFO ("dimension along axis " + str (i) + " specified as negative in NIfTI image \"" + H.name() + "\" - taking absolute value");
              H.size(i) = abs (H.size (i));
            }
            if (!H.size (i))
              H.size(i) = 1;
            H.stride(i) = i+1;
          }

          // voxel sizes:
          double pixdim[8];
          for (int i = 0; i < ndim; i++) {
            pixdim[i] = Raw::fetch_<float_type> (&NH.pixdim[i+1], is_BE);
            if (pixdim[i] < 0.0) {
              INFO ("voxel size along axis " + str (i) + " specified as negative in NIfTI image \"" + H.name() + "\" - taking absolute value");
              pixdim[i] = abs (pixdim[i]);
            }
            H.spacing(i) = pixdim[i];
          }


          // offset and scale:
          H.intensity_scale() = Raw::fetch_<float_type> (&NH.scl_slope, is_BE);
          if (std::isfinite (H.intensity_scale()) && H.intensity_scale() != 0.0) {
            H.intensity_offset() = Raw::fetch_<float_type> (&NH.scl_inter, is_BE);
            if (!std::isfinite (H.intensity_offset()))
              H.intensity_offset() = 0.0;
          } else {
            H.reset_intensity_scaling();
          }



          const int64_t data_offset = Raw::fetch_<vox_offset_type> (&NH.vox_offset, is_BE);



          char descrip[81];
          strncpy (descrip, NH.descrip, 80);
          if (descrip[0]) {
            descrip[80] = '\0';
            if (strncmp (descrip, "MRtrix version: ", 16) == 0)
              H.keyval()["mrtrix_version"] = descrip+16;
            else
              add_line (H.keyval()["comments"], descrip);
          }

          // used to rescale voxel sizes in case of mismatch between pixdim and
          // length of cosine vectors in sform:
          bool rescale_voxel_sizes = false;


          if (is_nifti) {
            bool sform_code = Raw::fetch_<code_type> (&NH.sform_code, is_BE);
            bool qform_code = Raw::fetch_<code_type> (&NH.qform_code, is_BE);

            if (sform_code) {
              auto& M (H.transform().matrix());

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
                if (size_t(H.ndim()) > axis) {
                  if (abs(pixdim[axis]/M.col(axis).head<3>().norm() - 1.0) > 1e-5) {
                    WARN ("voxel spacings inconsistent between NIFTI s-form and header field pixdim");
                    rescale_voxel_sizes = true;
                    break;
                  }
                }
              }

              // normalize each transform axis and rescale voxel sizes if
              // needed:
              for (size_t axis = 0; axis != 3; ++axis) {
                if (size_t(H.ndim()) > axis) {
                  auto length = M.col(axis).head<3>().norm();
                  M.col(axis).head<3>() /= length;
                  H.spacing(axis) = rescale_voxel_sizes ? length : pixdim[axis];
                }
              }

            }

            if (qform_code) {
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

              //CONF option: NIfTIUseSform
              //CONF default: 1 (true)
              //CONF A boolean value to control whether, in cases where both
              //CONF the sform and qform transformations are defined in an
              //CONF input NIfTI image, but those transformations differ, the
              //CONF sform transformation should be used in preference to the
              //CONF qform matrix. The default is to use the sform matrix;
              //CONF set to 0 / false to override and instead use the qform.
              const bool use_sform = File::Config::get_bool ("NIfTIUseSform", true);

              if (sform_code) {
                Header header2 (H);
                header2.transform() = M_qform;
                if (!voxel_grids_match_in_scanner_space (H, header2, 0.1))
                  WARN ("qform and sform are inconsistent in NIfTI image \"" + H.name() +
                      "\" - using " + (use_sform ? "sform" : "qform"));
              }

              if (!sform_code || !use_sform) {
                H.transform() = M_qform;
                H.spacing(0) = pixdim[0];
                H.spacing(1) = pixdim[1];
                H.spacing(2) = pixdim[2];
              }
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






      template <class NiftiHeader>
        void store (NiftiHeader& NH, const Header& H, const bool single_file)
        {
          const std::string& version = Type<NiftiHeader>::version();
          using dim_type = typename Type<NiftiHeader>::dim_type;
          using vox_offset_type = typename Type<NiftiHeader>::vox_offset_type;
          using code_type = typename Type<NiftiHeader>::code_type;
          using float_type = typename Type<NiftiHeader>::float_type;

          if (H.ndim() > 7)
            throw Exception (version + " format cannot support more than 7 dimensions for image \"" + H.name() + "\"");

          bool is_BE = H.datatype().is_big_endian();

          vector<size_t> axes;
          auto M = File::NIfTI::adjust_transform (H, axes);


          memset (&NH, 0, sizeof (NH));

          // magic number:
          Raw::store<int32_t> (sizeof(NH), &NH.sizeof_hdr, is_BE);

          memcpy ( (char*) &NH.magic, single_file ? Type<NiftiHeader>::magic1() : Type<NiftiHeader>::magic2(), 4);
          if (Type<NiftiHeader>::is_version2)
            memcpy ( (char*) &NH.magic+4, Type<NiftiHeader>::signature_extra(), 4);

          if (!Type<NiftiHeader>::is_version2) {
            const auto hit = H.keyval().find("comments");
            auto comments = split_lines (hit == H.keyval().end() ? std::string() : hit->second);
            strncpy ( Type<NiftiHeader>::db_name (NH), comments.size() ? comments[0].c_str() : "untitled\0\0\0\0\0\0\0\0\0\0\0", 17);
            Type<NiftiHeader>::db_name (NH)[17] = '\0';
            Raw::store<int32_t> (16384, Type<NiftiHeader>::extents (NH), is_BE);
            *Type<NiftiHeader>::regular(NH) = 'r';
          }
          NH.dim_info = 0;

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
              throw Exception ("unknown data type for " + version + " image \"" + H.name() + "\"");
          }
          Raw::store<int16_t> (dt, &NH.datatype, is_BE);

          Raw::store<int16_t> (H.datatype().bits(), &NH.bitpix, is_BE);

          // data set dimensions:
          Raw::store<dim_type> (H.ndim(), &NH.dim[0], is_BE);
          {
            size_t i = 0;
            for (; i < 3; i++)
              Raw::store<dim_type> (H.size (axes[i]), &NH.dim[i+1], is_BE);
            for (; i < H.ndim(); i++)
              Raw::store<dim_type> (H.size (i), &NH.dim[i+1], is_BE);

            // pad out the other dimensions with 1, fix for fslview
            ++i;
            for (; i < 8; i++)
              Raw::store<dim_type> (1, &NH.dim[i], is_BE);
          }

          Raw::store<float_type> (1.0, &NH.pixdim[0], is_BE);
          // voxel sizes:
          for (size_t i = 0; i < 3; ++i)
            Raw::store<float_type> (H.spacing (axes[i]), &NH.pixdim[i+1], is_BE);
          for (size_t i = 3; i < H.ndim(); ++i)
            Raw::store<float_type> (H.spacing (i), &NH.pixdim[i+1], is_BE);

          Raw::store<vox_offset_type> (sizeof(NH)+4, &NH.vox_offset, is_BE);

          // offset and scale:
          Raw::store<float_type> (H.intensity_scale(), &NH.scl_slope, is_BE);
          Raw::store<float_type> (H.intensity_offset(), &NH.scl_inter, is_BE);

          std::string version_string = std::string("MRtrix version: ") + App::mrtrix_version;
          if (App::project_version)
            version_string += std::string(", project version: ") + App::project_version;
          strncpy ( (char*) &NH.descrip, version_string.c_str(), 79);


          // qform:
          Eigen::Matrix3d R = M.matrix().topLeftCorner<3,3>();
          float qfac = 1.0;
          if (R.determinant() < 0.0) {
            R.col(2) = -R.col(2);
            qfac = -1.0;
          }
          Eigen::Quaterniond Q (R);

          if (Q.w() < 0.0)
            Q.coeffs() *= -1.0;

          if (R.isApprox (Q.matrix(), 1e-6)) {
            Raw::store<code_type> (NIFTI_XFORM_SCANNER_ANAT, &NH.qform_code, is_BE);
            Raw::store<float_type> (qfac, &NH.pixdim[0], is_BE);

            Raw::store<float_type> (Q.x(), &NH.quatern_b, is_BE);
            Raw::store<float_type> (Q.y(), &NH.quatern_c, is_BE);
            Raw::store<float_type> (Q.z(), &NH.quatern_d, is_BE);

            Raw::store<float_type> (M(0,3), &NH.qoffset_x, is_BE);
            Raw::store<float_type> (M(1,3), &NH.qoffset_y, is_BE);
            Raw::store<float_type> (M(2,3), &NH.qoffset_z, is_BE);
          }
          else {
            WARN ("image \"" + H.name() + "\" contains non-rigid transform - qform will not be stored.");
            Raw::store<code_type> (NIFTI_XFORM_UNKNOWN, &NH.qform_code, is_BE);
          }

          // sform:
          Raw::store<code_type> (NIFTI_XFORM_SCANNER_ANAT, &NH.sform_code, is_BE);

          Raw::store<float_type> (H.spacing (axes[0]) * M(0,0), &NH.srow_x[0], is_BE);
          Raw::store<float_type> (H.spacing (axes[1]) * M(0,1), &NH.srow_x[1], is_BE);
          Raw::store<float_type> (H.spacing (axes[2]) * M(0,2), &NH.srow_x[2], is_BE);
          Raw::store<float_type> (M (0,3), &NH.srow_x[3], is_BE);

          Raw::store<float_type> (H.spacing (axes[0]) * M(1,0), &NH.srow_y[0], is_BE);
          Raw::store<float_type> (H.spacing (axes[1]) * M(1,1), &NH.srow_y[1], is_BE);
          Raw::store<float_type> (H.spacing (axes[2]) * M(1,2), &NH.srow_y[2], is_BE);
          Raw::store<float_type> (M (1,3), &NH.srow_y[3], is_BE);

          Raw::store<float_type> (H.spacing (axes[0]) * M(2,0), &NH.srow_z[0], is_BE);
          Raw::store<float_type> (H.spacing (axes[1]) * M(2,1), &NH.srow_z[1], is_BE);
          Raw::store<float_type> (H.spacing (axes[2]) * M(2,2), &NH.srow_z[2], is_BE);
          Raw::store<float_type> (M (2,3), &NH.srow_z[3], is_BE);

          if (Type<NiftiHeader>::is_version2) {
            const char xyzt_units[4] { NIFTI_UNITS_MM, NIFTI_UNITS_MM, NIFTI_UNITS_MM, NIFTI_UNITS_SEC };
            const int32_t* const xyzt_units_as_int_ptr = reinterpret_cast<const int32_t*>(xyzt_units);
            Raw::store<int32_t> (*xyzt_units_as_int_ptr, &NH.xyzt_units, is_BE);
          }
          else
            NH.xyzt_units = SPACE_TIME_TO_XYZT (NIFTI_UNITS_MM, NIFTI_UNITS_SEC);

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
            File::JSON::save (H, json_path, H.name());
          }
        }






      void axes_on_write (const Header& H, vector<size_t>& order, vector<bool>& flip)
      {
        Stride::List strides = Stride::get (H);
        strides.resize (3);
        order = Stride::order (strides);
        flip = { strides[order[0]] < 0, strides[order[1]] < 0, strides[order[2]] < 0 };
      }




      transform_type adjust_transform (const Header& H, vector<size_t>& axes)
      {
        vector<bool> flip;
        axes_on_write (H, axes, flip);

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



      bool check (int VERSION, Header& H, const size_t num_axes, const vector<std::string>& suffixes)
      {
        if (!Path::has_suffix (H.name(), suffixes))
          return false;

        if (version (H) != VERSION)
          return false;

        std::string format = VERSION == 1 ? Type<nifti_1_header>::version() : Type<nifti_2_header>::version();

        if (num_axes < 3)
          throw Exception ("cannot create " + format + " image with less than 3 dimensions");
        if (num_axes > 7)
          throw Exception ("cannot create " + format + " image with more than 7 dimensions");

        H.ndim() = num_axes;

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

        return true;
      }



      namespace {
        template <int VERSION> struct Get { NOMEMALIGN using type = nifti_1_header; };
        template <> struct Get<2> { NOMEMALIGN using type = nifti_2_header; };
      }




      template <int VERSION>
        std::unique_ptr<ImageIO::Base> read (Header& H)
        {
          using nifti_header = typename Get<VERSION>::type;

          if (!Path::has_suffix (H.name(), ".nii") && !Path::has_suffix (H.name(), ".img"))
            return std::unique_ptr<ImageIO::Base>();

          const bool single_file = Path::has_suffix (H.name(), ".nii");
          const std::string header_path = single_file ?  H.name() : H.name().substr (0, H.name().size()-4) + ".hdr";

          try {
            File::MMap fmap (header_path);
            const size_t data_offset = fetch (H, * ( (const nifti_header*) fmap.address()));
            std::unique_ptr<ImageIO::Default> handler (new ImageIO::Default (H));
            handler->files.push_back (File::Entry (H.name(), ( single_file ? data_offset : 0 )));
            return std::move (handler);
          } catch (Exception& e) {
            e.display();
            return std::unique_ptr<ImageIO::Base>();
          }
        }



      template <int VERSION>
        std::unique_ptr<ImageIO::Base> read_gz (Header& H)
        {
          using nifti_header = typename Get<VERSION>::type;

          if (!Path::has_suffix (H.name(), ".nii.gz"))
            return std::unique_ptr<ImageIO::Base>();

          nifti_header NH;
          File::GZ zf (H.name(), "rb");
          zf.read (reinterpret_cast<char*> (&NH), sizeof(NH));
          zf.close();

          try {
            const size_t data_offset = fetch (H, NH);
            std::unique_ptr<ImageIO::GZ> io_handler (new ImageIO::GZ (H, data_offset));
            memcpy (io_handler.get()->header(), &NH, sizeof(NH));
            memset (io_handler.get()->header() + sizeof(NH), 0, sizeof(nifti1_extender));
            io_handler->files.push_back (File::Entry (H.name(), data_offset));
            return std::move (io_handler);
          } catch (...) {
            return std::unique_ptr<ImageIO::Base>();
          }
        }




      template <int VERSION>
        std::unique_ptr<ImageIO::Base> create (Header& H)
        {
          using nifti_header = typename Get<VERSION>::type;
          const std::string& version = Type<nifti_header>::version();

          if (H.ndim() > 7)
            throw Exception (version + " format cannot support more than 7 dimensions for image \"" + H.name() + "\"");

          const bool single_file = Path::has_suffix (H.name(), ".nii");
          const std::string header_path = single_file ? H.name() : H.name().substr (0, H.name().size()-4) + ".hdr";

          nifti_header NH;
          store (NH, H, single_file);
          File::OFStream out (header_path, std::ios::out | std::ios::binary);
          out.write ( (char*) &NH, sizeof (nifti_header));
          nifti1_extender extender;
          memset (extender.extension, 0x00, sizeof (nifti1_extender));
          out.write (extender.extension, sizeof (nifti1_extender));
          out.close();

          const size_t data_offset = single_file ? sizeof(NH)+4 : 0;

          if (single_file)
            File::resize (H.name(), data_offset + footprint(H));
          else
            File::create (H.name(), footprint(H));

          std::unique_ptr<ImageIO::Default> handler (new ImageIO::Default (H));
          handler->files.push_back (File::Entry (H.name(), data_offset));

          return std::move (handler);
        }





      template <int VERSION>
        std::unique_ptr<ImageIO::Base> create_gz (Header& H)
        {
          using nifti_header = typename Get<VERSION>::type;
          const std::string& version = Type<nifti_header>::version();

          if (H.ndim() > 7)
            throw Exception (version + " format cannot support more than 7 dimensions for image \"" + H.name() + "\"");

          std::unique_ptr<ImageIO::GZ> io_handler (new ImageIO::GZ (H, sizeof(nifti_header)+4));
          nifti_header& NH = *reinterpret_cast<nifti_header*> (io_handler->header());

          store (NH, H, true);
          memset (io_handler->header()+sizeof(nifti_header), 0, sizeof(nifti1_extender));

          File::create (H.name());
          io_handler->files.push_back (File::Entry (H.name(), sizeof(nifti_header)+4));

          return std::move (io_handler);
        }



      // force explicit instantiation:
      template std::unique_ptr<ImageIO::Base> read<1> (Header& H);
      template std::unique_ptr<ImageIO::Base> read<2> (Header& H);
      template std::unique_ptr<ImageIO::Base> read_gz<1> (Header& H);
      template std::unique_ptr<ImageIO::Base> read_gz<2> (Header& H);
      template std::unique_ptr<ImageIO::Base> create<1> (Header& H);
      template std::unique_ptr<ImageIO::Base> create<2> (Header& H);
      template std::unique_ptr<ImageIO::Base> create_gz<1> (Header& H);
      template std::unique_ptr<ImageIO::Base> create_gz<2> (Header& H);



      int version (Header& H)
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


      std::string get_json_path (const std::string & nifti_path) {
        std::string json_path;
        if (Path::has_suffix (nifti_path, ".nii.gz"))
          json_path = nifti_path.substr (0, nifti_path.size()-7);
        else if (Path::has_suffix (nifti_path, ".nii"))
          json_path = nifti_path.substr (0, nifti_path.size()-4);
        else
          assert (0);
        return json_path + ".json";
      }


    }
  }
}

