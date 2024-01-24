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

#ifndef __file_npy_h__
#define __file_npy_h__

#include <array>
#include <sstream>

#include "datatype.h"
#include "fetch_store.h"
#include "raw.h"
#include "types.h"
#include "file/config.h"
#include "file/mmap.h"
#include "file/ofstream.h"
#include "file/utils.h"
#include "math/math.h"






namespace MR
{
  namespace File
  {
    namespace NPY
    {



      constexpr unsigned char magic_string[] = "\x93NUMPY";
      constexpr size_t alignment = 16;



      DataType descr2datatype (const std::string&);
      bool descr_is_half (const std::string&, bool &is_little_endian);
      std::string datatype2descr (const DataType);
      size_t float_max_save_precision();
      KeyValues parse_dict (std::string);



      struct ReadInfo {
        DataType data_type;
        bool column_major;
        vector<ssize_t> shape;
        KeyValues keyval;
        int64_t data_offset;
      };

      ReadInfo read_header (const std::string& path);

      template <typename ValueType>
      Eigen::Matrix<ValueType, Eigen::Dynamic, Eigen::Dynamic> load_matrix (const std::string& path)
      {
        const ReadInfo info = read_header (path);

        // Memory-map the data content of the file
        File::MMap mmap ({path, info.data_offset}, false);

        // Actually load the data
        const ssize_t cols = info.shape.size() == 2 ? info.shape[1] : 1;
        Eigen::Matrix<ValueType, Eigen::Dynamic, Eigen::Dynamic> data (info.shape[0], cols);
        const std::function<ValueType(void*, size_t)> fetch_func (__set_fetch_function<ValueType> (info.data_type));
        size_t i = 0;
        if (info.column_major) {
          for (ssize_t col = 0; col != cols; ++col) {
            for (ssize_t row = 0; row != info.shape[0]; ++row)
              data(row, col) = fetch_func (mmap.address(), i++);
          }
        } else {
          for (ssize_t row = 0; row != info.shape[0]; ++row)
            for (ssize_t col = 0; col != cols; ++col) {
              data(row, col) = fetch_func (mmap.address(), i++);
          }
        }
        return data;
      }



      struct WriteInfo {
        std::unique_ptr<MMap> mmap;
        DataType data_type;
      };

      WriteInfo prepare_ND_write (const std::string& path, const DataType data_type, const vector<size_t>& shape);



      template <class ContType>
      void save_vector (const ContType& data, const std::string& path)
      {
        using ValueType = typename container_value_type<ContType>::type;
        const WriteInfo info = prepare_ND_write (path, DataType::from<ValueType>(), {size_t(data.size())});
        if (info.data_type == DataType::Bit) {
          uint8_t* const out = reinterpret_cast<uint8_t* const>(info.mmap->address());
          for (size_t i = 0; i != size_t(data.size()); ++i)
            out[i] = data[i] ? uint8_t(1) : uint8_t(0);
          return;
        }
        auto store_func = __set_store_function<ValueType> (info.data_type);
        for (size_t i = 0; i != size_t(data.size()); ++i)
          store_func (data[i], info.mmap->address(), i);
      }



      template <class ContType>
      void save_matrix (const ContType& data, const std::string& path)
      {
        using ValueType = typename ContType::Scalar;
        const WriteInfo info = prepare_ND_write (path, DataType::from<ValueType>(), {size_t(data.rows()), size_t(data.cols())});
        if (info.data_type == DataType::Bit) {
          uint8_t* const out = reinterpret_cast<uint8_t* const>(info.mmap->address());
          size_t i = 0;
          for (ssize_t col = 0; col != data.cols(); ++col)
            for (ssize_t row = 0; row != data.rows(); ++row) {
              out[i++] = data(row, col) ? uint8_t(1) : uint8_t(0);
          }
          return;
        }
        auto store_func = __set_store_function<ValueType> (info.data_type);
        size_t i = 0;
        for (ssize_t col = 0; col != data.cols(); ++col)
          for (ssize_t row = 0; row != data.rows(); ++row) {
            store_func (data(row, col), info.mmap->address(), i++);
        }
      }




    }
  }
}

#endif

