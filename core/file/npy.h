/* Copyright (c) 2008-2021 the MRtrix3 contributors.
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

#ifndef __file_npy_h__
#define __file_npy_h__

#include <sstream>

#include "datatype.h"
#include "fetch_store.h"
#include "raw.h"
#include "file/config.h"
#include "file/mmap.h"
#include "file/ofstream.h"
#include "file/utils.h"
#include "math/half.hpp"
#include "math/math.h"






namespace MR
{
  namespace File
  {
    namespace NPY
    {



      using half_float::half;



      constexpr unsigned char magic_string[] = "\x93NUMPY";
      constexpr size_t alignment = 16;



      DataType descr2datatype (const std::string&);
      bool descr_is_half (const std::string&, bool &is_little_endian);
      std::string datatype2descr (const DataType);
      size_t float_max_save_precision();
      KeyValues parse_header (std::string);



      template <typename ValueType>
      std::function<ValueType(void*, size_t)> __get_float16_fetch_func (const bool is_little_endian);




      template <typename ValueType>
      Eigen::Matrix<ValueType, Eigen::Dynamic, Eigen::Dynamic> load_matrix (const std::string& path)
      {
        std::ifstream in (path, std::ios_base::in | std::ios_base::binary);
        if (!in)
          throw Exception ("Unable to load file \"" + path + "\"");
        unsigned char magic[7];
        in.read (reinterpret_cast<char*>(magic), 6);
        if (memcmp (magic, magic_string, 6)) {
          magic[6] = 0;
          throw Exception ("Invalid magic string in NPY binary file \"" + path + "\": " + str(magic));
        }
        uint8_t major_version, minor_version;
        in.read (reinterpret_cast<char*>(&major_version), 1);
        in.read (reinterpret_cast<char*>(&minor_version), 1);
        uint32_t header_len;
        switch (major_version) {
          case 1:
            uint16_t header_len_short;
            in.read (reinterpret_cast<char*>(&header_len_short), 2);
            // header length always stored on filesystem as little-endian
            header_len = uint32_t (ByteOrder::LE (header_len_short));
            break;
          case 2:
            in.read (reinterpret_cast<char*>(&header_len), 4);
            // header length always stored on filesystem as little-endian
            header_len = ByteOrder::LE (header_len);
            break;
          default:
            throw Exception ("Incompatible major version (" + str(major_version) + ") detected in NumPy file \"" + path + "\"");
        }
        std::unique_ptr<char[]> header_cstr (new char[header_len+1]);
        in.read (header_cstr.get(), header_len);
        header_cstr[header_len] = '\0';
        KeyValues keyval;
        try {
          keyval = parse_header (std::string (header_cstr.get()));
        } catch (Exception& e) {
          throw Exception (e, "Error parsing header of NumPy file \"" + path + "\"");
        }
        const auto descr_ptr = keyval.find ("descr");
        if (descr_ptr == keyval.end())
          throw Exception ("Error parsing header of NumPy file \"" + path + "\": \"descr\" key absent");
        const std::string descr = descr_ptr->second;
        const auto fortran_order_ptr = keyval.find ("fortran_order");
        if (fortran_order_ptr == keyval.end())
          throw Exception ("Error parsing header of NumPy file \"" + path + "\": \"fortran_order\" key absent");
        const bool column_major = to<bool> (fortran_order_ptr->second);
        const auto shape_ptr = keyval.find ("shape");
        if (shape_ptr == keyval.end())
          throw Exception ("Error parsing header of NumPy file \"" + path + "\": \"shape\" key absent");
        const std::string shape_str = shape_ptr->second;

        DataType data_type;
        bool is_float16 = false;
        std::function<ValueType(void*, size_t)> fetch_func;
        try {
          data_type = descr2datatype (descr);
          fetch_func = __set_fetch_function<ValueType> (data_type);
        } catch (Exception& e) {
          bool is_little_endian;
          if (descr_is_half (descr, is_little_endian)) {
            // TODO These will not incorporate proper rounding if loading from float16 into integer;
            //   may be preferable to add float16 to DataType and initialise in fetch_store()
            fetch_func = __get_float16_fetch_func<ValueType> (is_little_endian);
            is_float16 = true;
          } else {
            throw Exception (e, "Error determining data type for NumPy file \"" + path + "\"");
          }
        }
        const size_t bytes = is_float16 ? 2 : data_type.bytes();

        // Deal with scale
        // Strip the brackets and split by commas
        auto shape_split_str = split(strip(strip(shape_str, "(", true, false), ")", false, true), ",", true);
        if (shape_split_str.size() > 2)
          throw Exception ("NumPy file \"" + path + "\" contains more than two dimensions: " + shape_str);
        const std::array<ssize_t, 2> size {to<ssize_t>(shape_split_str[0]),
                                           shape_split_str.size() == 2 ? to<ssize_t>(shape_split_str[1]) : 1};
        // Make sure that the size of the file matches expectations given the offset to the data, the shape, and the data type
        struct stat sbuf;
        if (stat (path.c_str(), &sbuf))
          throw Exception ("Cannot query size of NumPy file \"" + path + "\": " + strerror (errno));
        const size_t file_size = sbuf.st_size;
        const size_t num_elements = size[0] * size[1];
        const size_t predicted_data_size = (data_type == DataType::Bit) ?
                                           ((num_elements+7) / 8) :
                                           (num_elements * bytes);
        const int64_t data_offset = in.tellg();
        if (data_offset + predicted_data_size != file_size)
          throw Exception ("Size of NumPy file \"" + path + "\" (" + str(file_size) + ") does not meet expectations given "
                           + "total header size (" + str(data_offset) + ") "
                           + "and predicted data size ("
                           + "(" + str(size[0]) + "x" + str(size[1]) + " = " + str(num_elements) + ") "
                           + (data_type == DataType::Bit ?
                              ("bits = " + str((num_elements+7)/8) + " bytes)") :
                              ("values x " + str(bytes) + " bytes per value = " + str(num_elements * bytes) + " bytes)")));

        // Memory-map the remaining content of the file
        in.close();
        File::MMap mmap ({path, data_offset}, false);

        // Actually load the data
        Eigen::Matrix<ValueType, Eigen::Dynamic, Eigen::Dynamic> data (size[0], size[1]);
        size_t i = 0;
        if (column_major) {
          for (ssize_t col = 0; col != size[1]; ++col) {
            for (ssize_t row = 0; row != size[0]; ++row, ++i)
              data(row, col) = fetch_func (mmap.address(), i);
          }
        } else {
          for (ssize_t row = 0; row != size[0]; ++row)
            for (ssize_t col = 0; col != size[1]; ++col, ++i) {
              data(row, col) = fetch_func (mmap.address(), i);
          }
        }
        return data;
      }


      // TODO Provide separate functions for saving matrices vs. vectors,
      //   since the latter applies to std::vector<> also
      //
      // TODO For now, ignore 'fortran_order' field; it could however be utilised down the track
      //   if the function were to be given the capability to write to file using a File::MMap & memcpy()
      //   in cases where data type matches, but need to branch based on Eigen's major format for those particular data
      //   (also may need to be mindful of data alignment...)


      namespace
      {

        struct WriteInfo {
          std::unique_ptr<MMap> mmap;
          DataType data_type;
          bool write_float16;
        };

        template <typename ValueType>
        WriteInfo prepare_ND_write (const std::string& path, const std::array<size_t, 2>& size)
        {
          WriteInfo info;
          info.data_type = DataType::from<ValueType>();
          info.write_float16 = false;
          if (info.data_type.is_floating_point()) {
            const size_t max_precision = float_max_save_precision();
            if (max_precision < info.data_type.bits()) {
              INFO ("Precision of floating-point NumPy file \"" + path + "\" decreased from native " + str(info.data_type.bits()) + " bits to " + str(max_precision));
              if (max_precision == 16)
                info.write_float16 = true;
              else
                info.data_type = DataType::native (DataType::Float32);
            }
          }

          // Need to construct the header string in order to discover its length
          std::string header (std::string ("{'descr': '")
                              + (info.write_float16 ? (std::string(MRTRIX_IS_BIG_ENDIAN ? ">" : "<") + "f2") : datatype2descr (info.data_type))
                              + "', 'fortran_order': False, 'shape': (" + str(size[0]) + "," + (size[1] ? (" " + str(size[1])) : "") + "), }");
          // Pad with spaces so that, for version 1, upon adding a newline at the end, the file size (i.e. eventual offset to the data) is a multiple of alignment (16)
          uint32_t space_count = alignment - ((header.size() + 11) % alignment); // 11 = 6 magic number + 2 version + 2 header length + 1 newline for header
          uint32_t padded_header_length = header.size() + space_count + 1;
          File::OFStream out (path, std::ios_base::out | std::ios_base::binary);
          if (!out)
            throw Exception ("Unable to create NumPy file \"" + path + "\"");
          out.write (reinterpret_cast<const char*>(magic_string), 6);
          const unsigned char minor_version = '\x00';
          if (10 + padded_header_length > std::numeric_limits<uint16_t>::max()) {
            const unsigned char major_version = '\x02';
            out.write (reinterpret_cast<const char*>(&major_version), 1);
            out.write (reinterpret_cast<const char*>(&minor_version), 1);
            space_count = alignment - ((header.size() + 13) % alignment); // 13 = 6 magic number + 2 version + 4 header length + 1 newline for header
            padded_header_length = header.size() + space_count + 1;
            padded_header_length = ByteOrder::LE (padded_header_length);
            out.write (reinterpret_cast<const char*>(&padded_header_length), 4);
          } else {
            const unsigned char major_version = '\x01';
            out.write (reinterpret_cast<const char*>(&major_version), 1);
            out.write (reinterpret_cast<const char*>(&minor_version), 1);
            uint16_t padded_header_length_short = ByteOrder::LE (uint16_t (padded_header_length));
            out.write (reinterpret_cast<const char*>(&padded_header_length_short), 2);
          }
          for (size_t i = 0; i != space_count; ++i)
            header.push_back (' ');
          header.push_back ('\n');
          out.write (header.c_str(), header.size());
          const int64_t leadin_size = out.tellp();
          assert (!(out.tellp() % alignment));
          out.close();
          const size_t num_elements = size[0] * (size[1] ? size[1] : 1);
          const size_t data_size = (info.data_type == DataType::Bit) ?
                                   ((num_elements + 7) / 8) :
                                   (num_elements * (info.write_float16 ? 2 : info.data_type.bytes()));
          File::resize (path, leadin_size + data_size);
          info.mmap.reset (new File::MMap ({path, leadin_size}, true, false));
          return info;
        }


        template <typename ValueType>
        std::function<void(ValueType, void*, size_t)> __set_store_function (const DataType data_type, const bool write_float16)
        {
          if (write_float16) {
            return [] (ValueType value, void* addr, size_t i) -> void {
              reinterpret_cast<half*>(addr)[i] = value;
            };
          } else {
            return MR::__set_store_function<ValueType> (data_type);
          }
        }

      }



      template <class ContType>
      void save_vector (const ContType& data, const std::string& path)
      {
        using ValueType = typename container_value_type<ContType>::type;
        const WriteInfo info = prepare_ND_write<ValueType> (path, {size_t(data.size()), 0});
        auto store_func = __set_store_function<ValueType> (info.data_type, info.write_float16);
        for (size_t i = 0; i != size_t(data.size()); ++i)
          store_func (data[i], info.mmap->address(), i);
      }



      template <class ContType>
      void save_matrix (const ContType& data, const std::string& path)
      {
        using ValueType = typename ContType::Scalar;
        const WriteInfo info = prepare_ND_write<ValueType> (path, {size_t(data.rows()), size_t(data.cols())});
        auto store_func = __set_store_function<ValueType> (info.data_type, info.write_float16);
        size_t i = 0;
        for (ssize_t row = 0; row != data.rows(); ++row) {
          for (ssize_t col = 0; col != data.cols(); ++col, ++i)
            store_func (data(row, col), info.mmap->address(), i);
        }
      }




    }
  }
}

#endif

