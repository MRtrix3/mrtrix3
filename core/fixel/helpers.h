/* Copyright (c) 2008-2020 the MRtrix3 contributors.
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

#ifndef __fixel_helpers_h__
#define __fixel_helpers_h__

#include "header.h"
#include "image.h"
#include "algo/loop.h"
#include "fixel/keys.h"
#include "fixel/types.h"

namespace MR
{



  class InvalidFixelDirectoryException : public Exception
  { NOMEMALIGN
    public:
      InvalidFixelDirectoryException (const std::string& msg) : Exception(msg) {}
      InvalidFixelDirectoryException (const Exception& previous_exception, const std::string& msg)
        : Exception(previous_exception, msg) {}
  };



  namespace Peaks
  {
    void check (const Header& in);
  }



  namespace Fixel
  {



    bool is_index_filename (const std::string& path);
    bool is_directions_filename (const std::string& path);

    std::string filename2directory (const std::string& fixel_file);

    void check_fixel_size (const Header& index_h, const Header& data_h);

    Header find_index_header (const std::string &fixel_directory_path);
    Header find_directions_header (const std::string fixel_directory_path);
    vector<Header> find_data_headers (const std::string &fixel_directory_path,
                                      const Header &index_header,
                                      const bool include_directions = false);

    void check_fixel_directory_in (const std::string& path);
    void check_fixel_directory_out (const std::string& path,
                                    const bool new_index,
                                    const bool new_directions);

    //! Copy the index file from one fixel directory into another
    void copy_index_file (const std::string& input_directory, const std::string& output_directory);
    //! Copy the directions file from one fixel directory into another
    void copy_directions_file (const std::string& input_directory, const std::string& output_directory);
    //! Copy both the index and directions file from one fixel directory into another
    void copy_index_and_directions_file (const std::string& input_directory, const std::string &output_directory);
    //! Copy a file from one fixel directory into another.
    void copy_data_file (const std::string& input_file_path, const std::string& output_directory);
    //! Copy all data files in a fixel directory into another directory. Data files do not include the index or directions file.
    void copy_all_data_files (const std::string &input_directory, const std::string &output_directory);



    template <class HeaderType>
    bool is_index_image (const HeaderType& in)
    {
      return is_index_filename (in.name())
          && in.ndim() == 4
          && in.size(3) == 2;
    }
    template <class HeaderType>
    void check_index_image (const HeaderType& index)
    {
      if (!is_index_image (index))
        throw InvalidImageException (index.name() + " is not a valid fixel index image. Image must be 4D with 2 volumes in the 4th dimension");
    }



    template <class HeaderType>
    bool is_data_file (const HeaderType& in)
    {
      return in.ndim() == 3 && in.size(2) == 1;
    }
    template <class HeaderType>
    void check_data_file (const HeaderType& in)
    {
      if (!is_data_file (in))
        throw InvalidImageException (in.name() + " is not a valid fixel data file; expected a 3-dimensional image of size n x m x 1");
    }



    template <class HeaderType>
    bool is_directions_file (const HeaderType& in)
    {
      return is_directions_filename (in.name())
          && in.ndim() == 3
          && in.size(1) == 3
          && in.size(2) == 1;
    }







    template <class IndexHeaderType>
    index_type get_number_of_fixels (IndexHeaderType& index_header) {
      check_index_image (index_header);
      if (index_header.keyval().count (n_fixels_key)) {
        return std::stoul (index_header.keyval().at(n_fixels_key));
      } else {
        auto index_image = Image<index_type>::open (index_header.name());
        index_image.index(3) = 1;
        index_type num_fixels = 0;
        index_type max_offset = 0;
        for (auto i = MR::Loop (index_image, 0, 3) (index_image); i; ++i) {
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
    bool fixels_match (const IndexHeaderType& index_header, const DataHeaderType& data_header)
    {
      bool fixels_match (false);

      if (is_index_image (index_header)) {
        if (index_header.keyval().count (n_fixels_key)) {
          fixels_match = std::stoul (index_header.keyval().at(n_fixels_key)) == (index_type)data_header.size(0);
        } else {
          auto index_image = Image<index_type>::open (index_header.name());
          index_image.index(3) = 1;
          index_type num_fixels = 0;
          index_type max_offset = 0;
          for (auto i = MR::Loop (index_image, 0, 3) (index_image); i; ++i) {
            if (index_image.value() > max_offset) {
              max_offset = index_image.value();
              index_image.index(3) = 0;
              num_fixels = index_image.value();
              index_image.index(3) = 1;
            }
          }
          fixels_match = (max_offset + num_fixels) == (index_type)data_header.size(0);
        }
      }

      return fixels_match;
    }









    //! Generate a header for a sparse data file (Nx1x1) using an index image as a template
    template <class IndexHeaderType>
    Header data_header_from_index (IndexHeaderType& index) {
      Header header (index);
      header.ndim() = 3;
      header.size(0) = get_number_of_fixels (index);
      header.size(1) = 1;
      header.size(2) = 1;
      header.stride(0) = 1;
      header.stride(1) = 2;
      header.stride(2) = 3;
      header.transform() = transform_type::Identity();
      header.datatype() = DataType::Float32;
      header.datatype().set_byte_order_native();
      return header;
    }

    //! Generate a header for a fixel directions data file (Nx3x1) using an index image as a template
    template <class IndexHeaderType>
    Header directions_header_from_index (IndexHeaderType& index) {
      Header header = data_header_from_index (index);
      header.size(1) = 3;
      return header;
    }



    //! open a data file. checks that a user has not input a fixel directory or index image
    template <class ValueType>
    Image<ValueType> open_fixel_data_file (const std::string& input_file) {
      if (Path::is_dir (input_file))
        throw Exception ("please input the specific fixel data file to be converted (not the fixel directory)");

      Header in_data_header = Header::open (input_file);
      Fixel::check_data_file (in_data_header);
      auto in_data_image = in_data_header.get_image<ValueType>();

      Header in_index_header = Fixel::find_index_header (Fixel::filename2directory (input_file));
      if (input_file == in_index_header.name())
        throw Exception ("input fixel data file cannot be the index file");

      return in_data_image;
    }



  }
}

#endif
