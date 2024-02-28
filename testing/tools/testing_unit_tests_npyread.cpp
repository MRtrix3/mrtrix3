/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#include <string>

#include "command.h"
#include "datatype.h"
#include "file/matrix.h"
#include "file/npy.h"
#include "file/path.h"
#include "half.h"
#include "types.h"

using namespace MR;
using namespace App;

// clang-format off
void usage() {

  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Test the reading of NPY files";

  ARGUMENTS
  + Argument("in_dir", "Path to input directory in which test data have been generated").type_directory_in();

}
// clang-format on

const Eigen::Array<default_type, 3, 1> reference_1d{0.0, 1.0, 2.0};
const Eigen::Array<bool, 3, 1> reference_1d_bool{false, true, true};
const Eigen::Array<default_type, 3, 2> reference_2d{{0.0, 1.0}, {10.0, 11.0}, {20.0, 21.0}};
const Eigen::Array<bool, 3, 2> reference_2d_bool{{false, true}, {true, true}, {true, true}};

template <bool isboolean, bool is1D> bool verify_basic(const std::string &filepath) {
  if (is1D) {
    const Eigen::Array<default_type, Eigen::Dynamic, 1> data = File::Matrix::load_vector(filepath);
    return (isboolean ? data.cast<bool>().isApprox(reference_1d_bool) : data.isApprox(reference_1d));
  } else {
    const Eigen::Array<default_type, Eigen::Dynamic, Eigen::Dynamic> data = File::Matrix::load_matrix(filepath);
    return (isboolean ? data.cast<bool>().isApprox(reference_2d_bool) : data.isApprox(reference_2d));
  }
}

namespace {

template <typename T, int Major> bool check_shape(void *address, const File::NPY::ReadInfo &info) {
  // TODO Two different decisions here:
  // - If length of shape is 1, map to a vector, otherwise map to a matrix;
  // - If only one non-zero dimension, compare to 3x2 matrix reference, otherwise compare to vector reference
  //   (due to transposition, might need to deal with all four cases separately)
  // TODO Failing static assertion here;
  //   possible that I can't have a row-major column vector
  if (info.shape.size() == 1) {
    // if (Major == Eigen::ColMajor) {
    //   Eigen::Map<Eigen::Array<T, Eigen::Dynamic, 1, Major>> map (reinterpret_cast<T*> (address), info.shape[0]);
    //   return map.isApprox(reference_1d.cast<T>());
    // } else {
    //   Eigen::Map<Eigen::Array<T, 1, Eigen::Dynamic, Major>> map (reinterpret_cast<T*> (address), info.shape[0]);
    //   return map.isApprox(reference_1d.transpose().cast<T>());
    // }
    // TODO Try completely ignoring column / row major information in this case
    Eigen::Map<Eigen::Array<T, Eigen::Dynamic, 1>> map(reinterpret_cast<T *>(address), info.shape[0]);
    if (info.data_type == DataType::Bit) {
      std::cerr << map << "\n";
    }
    return map.isApprox(reference_1d.cast<T>());
  }
  Eigen::Map<Eigen::Array<T, Eigen::Dynamic, Eigen::Dynamic, Major>> map(
      reinterpret_cast<T *>(address), info.shape[0], info.shape[1]);
  if (info.data_type == DataType::Bit) {
    std::cerr << map << "\n";
  }
  if (info.shape[0] == 1) // 1x3
    return map.isApprox(reference_1d.transpose().cast<T>());
  if (info.shape[1] == 1) // 3x1
    return map.isApprox(reference_1d.cast<T>());
  // 3x2
  return map.isApprox(reference_2d.cast<T>());
}

template <typename T> bool check_major(void *address, const File::NPY::ReadInfo &info) {
  return info.column_major ? check_shape<T, Eigen::ColMajor>(address, info)
                           : check_shape<T, Eigen::RowMajor>(address, info);
}

bool check_datatype(void *address, const File::NPY::ReadInfo &info) {
  const DataType type = info.data_type() & ~DataType::BigEndian & ~DataType::LittleEndian;
  if (type == DataType::Bit)
    return check_major<int8_t>(address, info);
  if (type == DataType::Int8)
    return check_major<int8_t>(address, info);
  if (type == DataType::UInt8)
    return check_major<uint8_t>(address, info);
  if (type == DataType::Int16)
    return check_major<int16_t>(address, info);
  if (type == DataType::UInt16)
    return check_major<uint16_t>(address, info);
  if (type == DataType::Float16)
    return check_major<half_float::half>(address, info);
  if (type == DataType::Int32)
    return check_major<int32_t>(address, info);
  if (type == DataType::UInt32)
    return check_major<uint32_t>(address, info);
  if (type == DataType::Float32)
    return check_major<float>(address, info);
  if (type == DataType::Int64)
    return check_major<int64_t>(address, info);
  if (type == DataType::UInt64)
    return check_major<uint64_t>(address, info);
  if (type == DataType::Float64)
    return check_major<double>(address, info);
  assert(false);
  return false;
}
} // namespace

bool verify_advanced(const std::string &filepath, const File::NPY::ReadInfo &info) {
  File::MMap mmap({filepath, info.data_offset}, false);
  return check_datatype(mmap.address(), info);
}

void run() {
  Path::Dir dir(argument[0]);
  std::vector<std::string> errors_basic, errors_advanced;
  size_t check_count = 0;
  size_t wrong_endianness_count = 0;
  size_t advanced_boolean_count = 0;
  std::string entry;
  while ((entry = dir.read_name()).size()) {

    // TODO Do two reads:
    // - One with type tailored to what is known about the input file
    // - One using a generic load_vector() / load_matrix()
    //
    // IN the former case, it would actually be nice to test using an Eigen::Map<>
    const std::string fullpath(Path::join(argument[0], entry));
    const std::string basename = entry.substr(0, entry.size() - 4);
    const auto basename_split = split(basename, "_");
    std::string datatype_string = basename_split.back();
    replace(datatype_string, "LE", "<");
    replace(datatype_string, "BE", ">");
    replace(datatype_string, "BOOL", "?");
    const DataType data_type = File::NPY::descr2datatype(datatype_string);
    const bool isboolean(data_type == DataType::Bit);
    const bool is_1D = basename_split[0][0] == '1';

    if (is_1D) {
      if (!(isboolean ? verify_basic<true, true>(fullpath) : verify_basic<false, true>(fullpath)))
        errors_basic.push_back(basename);
    } else {
      if (!(isboolean ? verify_basic<true, false>(fullpath) : verify_basic<false, false>(fullpath)))
        errors_basic.push_back(basename);
    }

    auto info = File::NPY::read_header(fullpath);
    if (isboolean) {
      ++advanced_boolean_count;
      continue;
    }
    if (!info.data_type.is_byte_order_native()) {
      ++wrong_endianness_count;
      continue;
    }
    if (!verify_advanced(fullpath, info))
      errors_advanced.push_back(basename);
    ++check_count;
  }

  if (check_count) {
    CONSOLE(str(wrong_endianness_count) + " files skipped from advanced read due to possessing mismatched endianness");
    CONSOLE(str(advanced_boolean_count) +
            " files skipped from advanced read due to numpy not exporting packed boolean data");
    if (errors_basic.size() || errors_advanced.size())
      throw Exception("Errors on basic read in " + str(errors_basic.size()) + " files & advanced read in " +
                      str(errors_advanced.size()) +
                      " files: "
                      "[" +
                      join(errors_basic, ",") + "] [" + join(errors_advanced, ",") + "]");
    CONSOLE(str(check_count) + " NPY format read checks OK");
  } else {
    WARN("NPY input directory empty; no checks performed");
  }
}
