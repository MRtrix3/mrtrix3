/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#pragma once

#include <filesystem>
#include <string_view>
#include <utility>

#include "algo/loop.h"
#include "app.h"
#include "fixel/fixel.h"
#include "formats/mrtrix_utils.h"
#include "image.h"
#include "image_diff.h"
#include "image_helpers.h"

namespace MR::Fixel {
class InvalidDirectoryException : public Exception {
public:
  InvalidDirectoryException(std::string msg) : Exception(std::move(msg)) {}
  InvalidDirectoryException(const Exception &previous_exception, std::string msg)
      : Exception(previous_exception, std::move(msg)) {}
};

FORCE_INLINE bool is_index_filename(const std::filesystem::path &path) {
  for (std::initializer_list<const std::string>::iterator it = supported_image_formats.begin();
       it != supported_image_formats.end();
       ++it) {
    if (path.filename().string() == "index" + *it)
      return true;
  }
  return false;
}

template <class HeaderType> FORCE_INLINE bool is_index_image(const HeaderType &in) {
  return is_index_filename(in.name()) && in.ndim() == 4 && in.size(3) == 2;
}

template <class HeaderType> FORCE_INLINE void check_index_image(const HeaderType &index) {
  if (!is_index_image(index))
    throw InvalidImageException(
        index.name() + " is not a valid fixel index image. Image must be 4D with 2 volumes in the 4th dimension");
}

template <class HeaderType> FORCE_INLINE bool is_data_file(const HeaderType &in) {
  return in.ndim() == 3 && in.size(2) == 1;
}

FORCE_INLINE bool is_directions_filename(const std::filesystem::path &path) {
  for (std::initializer_list<const std::string>::iterator it = supported_image_formats.begin();
       it != supported_image_formats.end();
       ++it) {
    if (path.filename().string() == "directions" + *it)
      return true;
  }
  return false;
}

template <class HeaderType> FORCE_INLINE bool is_directions_file(const HeaderType &in) {
  return is_directions_filename(in.name()) && in.ndim() == 3 && in.size(1) == 3 && in.size(2) == 1;
}

template <class HeaderType> FORCE_INLINE void check_data_file(const HeaderType &in) {
  if (!is_data_file(in))
    throw InvalidImageException(in.name() + " is not a valid fixel data file;" +      //
                                " expected a 3-dimensional image of size n x m x 1"); //
}

FORCE_INLINE std::filesystem::path get_fixel_directory(const std::filesystem::path &fixel_file) {
  std::filesystem::path fixel_directory = fixel_file.parent_path();
  // assume the user is running the command from within the fixel directory
  if (fixel_directory.empty())
    fixel_directory = std::filesystem::current_path();
  return fixel_directory;
}

FORCE_INLINE index_type get_number_of_fixels(const Header &index_header) {
  check_index_image(index_header);
  if (index_header.keyval().count(n_fixels_key) != 0u) {
    return std::stoul(index_header.keyval().at(n_fixels_key));
  } else {
    auto index_image = Image<index_type>::open(index_header.path());
    index_image.index(3) = 1;
    index_type num_fixels = 0;
    index_type max_offset = 0;
    for (auto i = MR::Loop(index_image, 0, 3)(index_image); i; ++i) {
      if (index_image.value() > max_offset) {
        max_offset = index_image.value();
        index_image.index(3) = 0;
        num_fixels = index_image.value();
        index_image.index(3) = 1;
      }
    }
    return (max_offset + num_fixels);
  }
}

template <class DataHeaderType>
FORCE_INLINE bool fixels_match(const Header &index_header, const DataHeaderType &data_header) {
  return data_header.size(0) == get_number_of_fixels(index_header);
}

FORCE_INLINE void check_fixel_size(const Header &index_h, const Header &data_h) {
  check_index_image(index_h);
  check_data_file(data_h);

  if (!fixels_match(index_h, data_h))
    throw InvalidImageException("Fixel number mismatch between index image " + index_h.path().string() + //
                                " and data image " + data_h.path().string());                            //
}

FORCE_INLINE void check_fixel_size(const Header &H, const index_type nfixels) {
  check_data_file(H);
  if (H.size(0) != nfixels)
    throw InvalidImageException("Data image " + H.path().string() + " fixel count (" + str(H.size(0)) + ")" + //
                                " does not match expected number of fixels (" + str(nfixels) + ")");          //
}

FORCE_INLINE void
check_fixel_directory(const std::filesystem::path &path, bool create_if_missing = false, bool check_if_empty = false) {
  // handle the use case when a fixel command is run from inside a fixel directory
  std::filesystem::path const fixel_dir = path.empty() ? std::filesystem::current_path() : path;

  if (!std::filesystem::exists(fixel_dir)) {
    if (create_if_missing)
      std::filesystem::create_directory(fixel_dir);
    else
      throw Exception("Fixel directory (" + fixel_dir.string() + ") does not exist");
  } else if (!std::filesystem::is_directory(fixel_dir))
    throw Exception(fixel_dir.string() + " is not a directory");

  if (check_if_empty && std::filesystem::directory_iterator(fixel_dir) != std::filesystem::directory_iterator())
    throw Exception("Output fixel directory \"" + fixel_dir.string() + "\" is not empty" +
                    (App::overwrite_files
                         ? " (-force option cannot safely be applied on directories; please erase manually instead)"
                         : ""));
}

FORCE_INLINE Header find_index_header(const std::filesystem::path &fixel_directory_path) {
  Header header;
  check_fixel_directory(fixel_directory_path);

  for (std::initializer_list<const std::string>::iterator it = supported_image_formats.begin();
       it != supported_image_formats.end();
       ++it) {
    std::filesystem::path const full_path = fixel_directory_path / ("index" + *it);
    if (std::filesystem::exists(full_path)) {
      if (header.valid())
        throw InvalidDirectoryException("Multiple index images found in directory " + fixel_directory_path.string());
      header = Header::open(full_path);
    }
  }
  if (!header.valid())
    throw InvalidDirectoryException("Could not find index image in directory " + fixel_directory_path.string());

  check_index_image(header);
  return header;
}

FORCE_INLINE std::vector<Header> find_data_headers(const std::filesystem::path &fixel_directory_path,
                                                   const Header &index_header,
                                                   const bool include_directions = false) {
  check_index_image(index_header);
  std::vector<std::filesystem::path> file_paths;
  {
    for (const auto &entry : std::filesystem::directory_iterator(fixel_directory_path))
      file_paths.push_back(entry.path());
  }
  std::sort(file_paths.begin(), file_paths.end());

  std::vector<Header> data_headers;
  for (const auto &fpath : file_paths) {
    if (Path::has_suffix(fpath.filename(), supported_image_formats)) {
      try {
        auto H = Header::open(fpath);
        if (is_data_file(H)) {
          if (fixels_match(index_header, H)) {
            if (!is_directions_file(H) || include_directions)
              data_headers.emplace_back(std::move(H));
          } else {
            WARN("fixel data file (" + fpath.string() + ")" +                                  //
                 " does not contain the same number of elements as fixels in the index file"); //
          }
        }
      } catch (...) {
        WARN("unable to open file \"" + fpath.string() + "\" as potential fixel data file");
      }
    }
  }

  return data_headers;
}

FORCE_INLINE Header find_directions_header(const std::filesystem::path &fixel_directory_path) {
  bool directions_found(false);
  Header header;
  check_fixel_directory(fixel_directory_path);
  Header const index_header = Fixel::find_index_header(fixel_directory_path);

  for (const auto &entry : std::filesystem::directory_iterator(fixel_directory_path)) {
    if (is_directions_filename(entry.path().filename())) {
      Header tmp_header = Header::open(entry.path());
      if (is_directions_file(tmp_header)) {
        if (fixels_match(index_header, tmp_header)) {
          if (directions_found == true)
            throw Exception("multiple directions files found in fixel image directory: " +
                            fixel_directory_path.string());
          directions_found = true;
          header = std::move(tmp_header);
        } else {
          WARN("fixel directions file (" + entry.path().string() + ")" +                     //
               " does not contain the same number of elements as fixels in the index file"); //
        }
      }
    }
  }

  if (!directions_found)
    throw InvalidDirectoryException("Could not find directions image in directory " + fixel_directory_path.string());

  return header;
}

//! Generate a header for a fixel data file (Nx1x1)
FORCE_INLINE Header data_header_from_nfixels(const size_t nfixels) {
  Header header;
  header.ndim() = 3;
  header.size(0) = nfixels;
  header.size(1) = 1;
  header.size(2) = 1;
  header.spacing(0) = header.spacing(1) = header.spacing(2) = 1.0;
  header.stride(0) = 1;
  header.stride(1) = 2;
  header.stride(2) = 3;
  header.spacing(0) = header.spacing(1) = header.spacing(2) = 1.0;
  header.transform().setIdentity();
  header.datatype() = DataType::native(DataType::Float32);
  return header;
}

//! Generate a header for a fixel data file (Nx1x1) using an index image as a template
template <class IndexHeaderType> FORCE_INLINE Header data_header_from_index(IndexHeaderType &index) {
  Header header(data_header_from_nfixels(get_number_of_fixels(index)));
  for (size_t axis = 0; axis != 3; ++axis)
    header.spacing(axis) = index.spacing(axis);
  header.keyval() = index.keyval();
  return header;
}

//! Generate a header for a fixel directions data file (Nx3x1) based on knowledge of the number of fixels
FORCE_INLINE Header directions_header_from_nfixels(const size_t nfixels) {
  Header header = data_header_from_nfixels(nfixels);
  header.size(1) = 3;
  header.stride(0) = 2;
  header.stride(1) = 1;
  return header;
}

//! Generate a header for a fixel directions data file (Nx3x1) using an index image as a template
template <class IndexHeaderType> FORCE_INLINE Header directions_header_from_index(IndexHeaderType &index) {
  Header header = data_header_from_index(index);
  for (size_t axis = 0; axis != 3; ++axis)
    header.spacing(axis) = index.spacing(axis);
  header.size(1) = 3;
  header.stride(0) = 2;
  header.stride(1) = 1;
  return header;
}

//! Copy a file from one fixel directory into another.
FORCE_INLINE void copy_fixel_file(const std::filesystem::path &input_file_path,
                                  const std::filesystem::path &output_directory) {
  check_fixel_directory(output_directory, true);
  std::filesystem::path const output_path = output_directory / input_file_path.filename().string();
  Header input_header = Header::open(input_file_path);
  auto input_image = input_header.get_image<float>();
  auto output_image = Image<float>::create(output_path, input_header);
  threaded_copy(input_image, output_image);
}

//! Copy the index file from one fixel directory into another
FORCE_INLINE void copy_index_file(const std::filesystem::path &input_directory,
                                  const std::filesystem::path &output_directory) {
  Header input_header = Fixel::find_index_header(input_directory);
  check_fixel_directory(output_directory, true);

  std::filesystem::path const output_path =
      output_directory / static_cast<std::filesystem::path>(input_header.path()).filename();

  // If the index file already exists check it is the same as the input index file
  if (std::filesystem::exists(output_path)) {
    auto input_image = input_header.get_image<index_type>();
    auto output_image = Image<index_type>::open(output_path);
    if (!images_match_abs(input_image, output_image))
      throw Exception("output fixel directory \"" + output_directory.string() + "\" already contains index file, " +
                      "which is not the same as the expected output" +
                      (App::overwrite_files
                           ? " (-force option cannot safely be applied on directories; please erase manually instead)"
                           : ""));
  } else {
    auto output_image = Image<index_type>::create(
        output_directory / static_cast<std::filesystem::path>(input_header.path()).filename(), input_header);
    auto input_image = input_header.get_image<index_type>();
    threaded_copy(input_image, output_image);
  }
}

//! Copy the directions file from one fixel directory into another.
FORCE_INLINE void copy_directions_file(const std::filesystem::path &input_directory,
                                       const std::filesystem::path &output_directory) {
  Header input_header = Fixel::find_directions_header(input_directory);
  namespace fs = std::filesystem;
  fs::path const output_path = output_directory / input_header.path().filename();

  // If the directions file already exists check it is the same as the input directions file
  if (std::filesystem::exists(output_path)) {
    auto input_image = input_header.get_image<float>();
    auto output_image = Image<float>::open(output_path);
    if (!images_match_abs(input_image, output_image))
      throw Exception("output fixel directory \"" + output_directory.string() + "\"" +  //
                      " already contains directions file, " +                           //
                      "which is not the same as the expected output" +                  //
                      (App::overwrite_files                                             //
                           ? " (-force option cannot safely be applied on directories;" //
                             " please erase manually instead)"                          //
                           : ""));                                                      //
  } else {
    auto output_image = Image<float>::create(output_directory / input_header.path().filename(), input_header);
    auto input_image = input_header.get_image<float>();
    threaded_copy(input_image, output_image);
  }
}

FORCE_INLINE void copy_index_and_directions_file(const std::filesystem::path &input_directory,
                                                 const std::filesystem::path &output_directory) {
  copy_index_file(input_directory, output_directory);
  copy_directions_file(input_directory, output_directory);
}

//! Copy all data files in a fixel directory into another directory. Data files do not include the index or directions
//! file.
FORCE_INLINE void copy_all_data_files(const std::filesystem::path &input_directory,
                                      const std::filesystem::path &output_directory) {
  for (auto &input_header : Fixel::find_data_headers(input_directory, Fixel::find_index_header(input_directory)))
    copy_fixel_file(input_header.path(), output_directory);
}

//! open a data file. checks that a user has not input a fixel directory or index image
template <class ValueType> Image<ValueType> open_fixel_data_file(const std::filesystem::path &input_file) {
  if (std::filesystem::is_directory(input_file))
    throw Exception("please input the specific fixel data file to be converted (not the fixel directory)");

  Header in_data_header = Header::open(input_file);
  Fixel::check_data_file(in_data_header);
  auto in_data_image = in_data_header.get_image<ValueType>();

  Header in_index_header = Fixel::find_index_header(Fixel::get_fixel_directory(input_file));
  if (input_file == in_index_header.path())
    throw Exception("input fixel data file cannot be the index file");

  return in_data_image;
}

} // namespace MR::Fixel
