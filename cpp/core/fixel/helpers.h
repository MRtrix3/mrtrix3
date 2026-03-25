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

#include "algo/loop.h"
#include "app.h"
#include "fixel/fixel.h"
#include "formats/mrtrix_utils.h"
#include "image.h"
#include "image_diff.h"
#include "image_helpers.h"

namespace MR {
class InvalidFixelDirectoryException : public Exception {
public:
  InvalidFixelDirectoryException(std::string msg) : Exception(msg) {}
  InvalidFixelDirectoryException(const Exception &previous_exception, std::string msg)
      : Exception(previous_exception, msg) {}
};
} // namespace MR

namespace MR::Peaks {
void check(const Header &in);
} // namespace MR::Peaks

namespace MR::Fixel {

bool is_index_filename(std::string_view path);

bool is_directions_filename(std::string_view path);

std::string get_fixel_directory(std::string_view fixel_file);

void check_fixel_directory(std::string_view path, bool create_if_missing = false, bool check_if_empty = false);

Header find_index_header(std::string_view fixel_directory_path);

std::vector<Header> find_data_headers(std::string_view fixel_directory_path,
                                      const Header &index_header,
                                      const bool include_directions = false);

Header find_directions_header(std::string_view fixel_directory_path);

//! Generate a header for a fixel data file (Nx1x1)
Header data_header_from_nfixels(const size_t nfixels);

//! Generate a header for a fixel directions data file (Nx3x1) based on knowledge of the number of fixels
Header directions_header_from_nfixels(const size_t nfixels);

//! Copy a file from one fixel directory into another.
void copy_fixel_file(std::string_view input_file_path, std::string_view output_directory);

//! Copy the index file from one fixel directory into another
void copy_index_file(std::string_view input_directory, std::string_view output_directory);

//! Copy the directions file from one fixel directory into another.
void copy_directions_file(std::string_view input_directory, std::string_view output_directory);

void copy_index_and_directions_file(std::string_view input_directory, std::string_view output_directory);

//! Copy all data files in a fixel directory into another directory.
//! Data files do not include the index or directions file.
void copy_all_data_files(std::string_view input_directory, std::string_view output_directory);

template <class HeaderType> bool is_index_image(const HeaderType &in) {
  return is_index_filename(in.name()) && in.ndim() == 4 && in.size(3) == 2;
}

template <class HeaderType> void check_index_image(const HeaderType &index) {
  if (!is_index_image(index))
    throw InvalidImageException(
        index.name() + " is not a valid fixel index image. Image must be 4D with 2 volumes in the 4th dimension");
}

template <class HeaderType> bool is_data_file(const HeaderType &in) { return in.ndim() == 3 && in.size(2) == 1; }

template <class HeaderType> bool is_directions_file(const HeaderType &in) {
  return is_directions_filename(in.name()) && in.ndim() == 3 && in.size(1) == 3 && in.size(2) == 1;
}

template <class HeaderType> void check_data_file(const HeaderType &in) {
  if (!is_data_file(in))
    throw InvalidImageException(in.name() +
                                " is not a valid fixel data file. Expected a 3-dimensional image of size n x m x 1");
}

template <class IndexHeaderType> index_type get_number_of_fixels(IndexHeaderType &index_header) {
  check_index_image(index_header);
  if (index_header.keyval().count(n_fixels_key)) {
    return std::stoul(index_header.keyval().at(n_fixels_key));
  } else {
    auto index_image = Image<index_type>::open(index_header.name());
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

template <class IndexHeaderType, class DataHeaderType>
bool fixels_match(const IndexHeaderType &index_header, const DataHeaderType &data_header) {
  bool fixels_match(false);

  if (is_index_image(index_header)) {
    if (index_header.keyval().count(n_fixels_key)) {
      fixels_match =
          to<index_type>(index_header.keyval().at(n_fixels_key)) == static_cast<index_type>(data_header.size(0));
    } else {
      auto index_image = Image<index_type>::open(index_header.name());
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
      fixels_match = (max_offset + num_fixels) == static_cast<index_type>(data_header.size(0));
    }
  }

  return fixels_match;
}

template <class IndexHeaderType, class DataHeaderType>
void check_fixel_size(const IndexHeaderType &index_h, const DataHeaderType &data_h) {
  check_index_image(index_h);
  check_data_file(data_h);
  if (!fixels_match(index_h, data_h))
    throw InvalidImageException("Fixel number mismatch between index image " + index_h.name() + " and data image " +
                                data_h.name());
}

//! Generate a header for a fixel data file (Nx1x1) using an index image as a template
template <class IndexHeaderType> Header data_header_from_index(IndexHeaderType &index) {
  Header header(data_header_from_nfixels(get_number_of_fixels(index)));
  for (size_t axis = 0; axis != 3; ++axis)
    header.spacing(axis) = index.spacing(axis);
  header.keyval() = index.keyval();
  return header;
}

//! Generate a header for a fixel directions data file (Nx3x1) using an index image as a template
template <class IndexHeaderType> Header directions_header_from_index(IndexHeaderType &index) {
  Header header = data_header_from_index(index);
  for (size_t axis = 0; axis != 3; ++axis)
    header.spacing(axis) = index.spacing(axis);
  header.size(1) = 3;
  header.strides() = {2, 1, 3};
  return header;
}

//! open a data file. checks that a user has not input a fixel directory or index image
template <class ValueType> Image<ValueType> open_fixel_data_file(std::string_view input_file) {
  if (Path::is_dir(input_file))
    throw Exception("please input the specific fixel data file to be converted (not the fixel directory)");

  Header in_data_header = Header::open(input_file);
  Fixel::check_data_file(in_data_header);
  auto in_data_image = in_data_header.get_image<ValueType>();

  Header in_index_header = Fixel::find_index_header(Fixel::get_fixel_directory(input_file));
  if (input_file == in_index_header.name())
    throw Exception("input fixel data file cannot be the index file");

  return in_data_image;
}
} // namespace MR::Fixel
