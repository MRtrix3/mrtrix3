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

#ifndef __fixel_helpers_h__
#define __fixel_helpers_h__

#include "formats/mrtrix_utils.h"
#include "fixel_format/keys.h"
#include "algo/loop.h"

namespace MR
{
  class InvalidFixelDirectoryException : public Exception
  {
    public:
      InvalidFixelDirectoryException (const std::string& msg) : Exception(msg) {}
      InvalidFixelDirectoryException (const Exception& previous_exception, const std::string& msg)
        : Exception(previous_exception, msg) {}
  };

  namespace FixelFormat
  {
    FORCE_INLINE bool is_index_image (const Header& in)
    {
      bool is_index = false;
      if (in.ndim() == 4) {
        if (in.size(3) == 2) {
          for (std::initializer_list<const std::string>::iterator it = FixelFormat::supported_fixel_formats.begin();
               it != FixelFormat::supported_fixel_formats.end(); ++it) {
            if (Path::basename (in.name()) == "index" + *it)
              is_index = true;
          }
        }
      }
      return is_index;
    }

    template <class IndexHeaderType>
    FORCE_INLINE void check_index_image (const IndexHeaderType& index)
    {
      if (!is_index_image (index))
        throw InvalidImageException (index.name() + " is not a valid fixel index image. Image must be 4D with 2 volumes in the 4th dimension");
    }


    FORCE_INLINE bool is_data_file (const Header& in)
    {
      return in.ndim() == 3 && in.size(2) == 1;
    }


    FORCE_INLINE bool is_directions_file (const Header& in)
    {
      bool is_directions = false;
      if (in.ndim() == 3) {
        if (in.size(1) == 3 && in.size(2) == 1) {
          for (std::initializer_list<const std::string>::iterator it = FixelFormat::supported_fixel_formats.begin();
               it != FixelFormat::supported_fixel_formats.end(); ++it) {
            if (Path::basename (in.name()) == "directions" + *it)
              is_directions = true;
          }
        }
      }
      return is_directions;
    }


    FORCE_INLINE void check_data_file (const Header& in)
    {
      if (!is_data_file (in))
        throw InvalidImageException (in.name() + " is not a valid fixel data file. Expected a 3-dimensional image of size n x m x 1");
    }

    FORCE_INLINE std::string get_fixel_folder (const std::string& fixel_file) {
      std::string fixel_folder = Path::dirname (fixel_file);
      // assume the user is running the command from within the fixel directory
      if (fixel_folder.empty())
        fixel_folder = Path::cwd();
      return fixel_folder;
    }


    template <class IndexHeaderType>
    FORCE_INLINE uint32_t get_number_of_fixels (IndexHeaderType& index_header) {
      check_index_image (index_header);
      if (index_header.keyval().count (n_fixels_key)) {
        return std::stoul (index_header.keyval().at(n_fixels_key));
      } else {
        auto index_image = Image<uint32_t>::open (index_header.name());
        index_image.index(3) = 1;
        uint32_t num_fixels = 0;
        uint32_t max_offset = 0;
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
    FORCE_INLINE bool fixels_match (const IndexHeaderType& index_header, const DataHeaderType& data_header)
    {
      bool fixels_match (false);

      if (is_index_image (index_header)) {
        if (index_header.keyval().count (n_fixels_key)) {
          fixels_match = std::stoul (index_header.keyval().at(n_fixels_key)) == (uint32_t)data_header.size(0);
        } else {
          auto index_image = Image<uint32_t>::open (index_header.name());
          index_image.index(3) = 1;
          uint32_t num_fixels = 0;
          uint32_t max_offset = 0;
          for (auto i = MR::Loop (index_image, 0, 3) (index_image); i; ++i) {
            if (index_image.value() > max_offset) {
              max_offset = index_image.value();
              index_image.index(3) = 0;
              num_fixels = index_image.value();
              index_image.index(3) = 1;
            }
          }
          fixels_match = (max_offset + num_fixels) == (uint32_t)data_header.size(0);
        }
      }

      return fixels_match;
    }


    FORCE_INLINE void check_fixel_size (const Header& index_h, const Header& data_h)
    {
      check_index_image (index_h);
      check_data_file (data_h);

      if (!fixels_match (index_h, data_h))
        throw InvalidImageException ("Fixel number mismatch between index image " + index_h.name() + " and data image " + data_h.name());
    }


    FORCE_INLINE void check_fixel_folder (const std::string &path, bool create_if_missing = false, bool check_if_empty = false)
    {
      std::string path_temp = path;
      // handle the use case when a fixel command is run from inside a fixel folder
      if (path.empty())
        path_temp = Path::cwd();

      bool exists (true);

      if (!(exists = Path::exists (path_temp))) {
        if (create_if_missing) File::mkdir (path_temp);
        else throw Exception ("Fixel directory (" + str(path_temp) + ") does not exist");
      }
      else if (!Path::is_dir (path_temp))
        throw Exception (str(path_temp) + " is not a directory");

      if (check_if_empty && Path::Dir (path_temp).read_name ().size () != 0)
        throw Exception ("Expected fixel directory " + path_temp + " to be empty.");
    }


    FORCE_INLINE Header find_index_header (const std::string &fixel_folder_path)
    {
      Header header;
      check_fixel_folder (fixel_folder_path);

      for (std::initializer_list<const std::string>::iterator it = FixelFormat::supported_fixel_formats.begin();
           it != FixelFormat::supported_fixel_formats.end(); ++it) {
        std::string full_path = Path::join (fixel_folder_path, "index" + *it);
        if (Path::exists(full_path)) {
          if (header.valid())
            throw InvalidFixelDirectoryException ("Multiple index images found in directory " + fixel_folder_path);
          header = Header::open (full_path);
        }
      }
      if (!header.valid())
        throw InvalidFixelDirectoryException ("Could not find index image in directory " + fixel_folder_path);

      check_index_image (header);
      return header;
    }


    FORCE_INLINE std::vector<Header> find_data_headers (const std::string &fixel_folder_path, const Header &index_header, const bool include_directions = false)
    {
      check_index_image (index_header);
      std::vector<Header> data_headers;

      auto dir_walker = Path::Dir (fixel_folder_path);
      std::string fname;
      while ((fname = dir_walker.read_name ()).size ()) {
        auto full_path = Path::join (fixel_folder_path, fname);
        Header H;
        if (Path::has_suffix (fname, FixelFormat::supported_fixel_formats) && is_data_file (H = Header::open (full_path))) {
          if (fixels_match (index_header, H)) {
            if (!is_directions_file (H) || include_directions)
              data_headers.emplace_back (std::move (H));
          } else {
            WARN ("fixel data file (" + fname + ") does not contain the same number of elements as fixels in the index file");
          }
        }
      }

      return data_headers;
    }


    FORCE_INLINE Header find_directions_header (const std::string fixel_folder_path)
    {

      bool directions_found (false);
      Header header;
      check_fixel_folder (fixel_folder_path);
      Header index_header = FixelFormat::find_index_header (fixel_folder_path);

      auto dir_walker = Path::Dir (fixel_folder_path);
      std::string fname;
      while ((fname = dir_walker.read_name ()).size ()) {
        Header tmp_header;
        auto full_path = Path::join (fixel_folder_path, fname);
        if (Path::has_suffix (fname, FixelFormat::supported_fixel_formats)
              && is_directions_file (tmp_header = Header::open (full_path))) {
          if (is_directions_file (tmp_header)) {
            if (fixels_match (index_header, tmp_header)) {
              if (directions_found == true)
                throw Exception ("multiple directions files found in fixel image folder: " + fixel_folder_path);
              directions_found = true;
              header = std::move(tmp_header);
            } else {
              WARN ("fixel directions file (" + fname + ") does not contain the same number of elements as fixels in the index file" );
            }
          }
        }
      }

      if (!directions_found)
        throw InvalidFixelDirectoryException ("Could not find directions image in directory " + fixel_folder_path);

      return header;
    }

    //! Generate a header for a sparse data file (Nx1x1) using an index image as a template
    template <class IndexHeaderType>
    FORCE_INLINE Header data_header_from_index (IndexHeaderType& index) {
      Header header (index);
      header.ndim() = 3;
      header.size(0) = get_number_of_fixels (index);
      header.size(1) = 1;
      header.size(2) = 1;
      header.datatype() = DataType::Float32;
      header.datatype().set_byte_order_native();
      return header;
    }

    //! Generate a header for a fixel directions data file (Nx3x1) using an index image as a template
    template <class IndexHeaderType>
    FORCE_INLINE Header directions_header_from_index (IndexHeaderType& index) {
      Header header = data_header_from_index (index);
      header.size(1) = 3;
      return header;
    }

    //! Copy a file from one fixel folder into another.
    FORCE_INLINE void copy_fixel_file (const std::string& input_file_path, const std::string& output_folder) {
      check_fixel_folder (output_folder, true);
      std::string output_path = Path::join (output_folder, Path::basename (input_file_path));
      Header input_header = Header::open (input_file_path);
      auto input_image = input_header.get_image<float>();
      auto output_image = Image<float>::create (output_path, input_header);
      threaded_copy (input_image, output_image);
    }

    //! Copy the index file from one fixel folder into another
    FORCE_INLINE void copy_index_file (const std::string& input_folder, const std::string& output_folder) {
      Header input_header = FixelFormat::find_index_header (input_folder);
      check_fixel_folder (output_folder, true);
      auto output_image = Image<uint32_t>::create (Path::join (output_folder, Path::basename (input_header.name())), input_header);
      auto input_image = input_header.get_image<uint32_t>();
      threaded_copy (input_image, output_image);
    }

    //! Copy the directions file from one fixel folder into another.
    FORCE_INLINE void copy_directions_file (const std::string& input_folder, const std::string& output_folder) {
      Header directions_header = FixelFormat::find_directions_header (input_folder);
      copy_fixel_file (directions_header.name(), output_folder);
    }

    FORCE_INLINE void copy_index_and_directions_file (const std::string& input_folder, const std::string &output_folder) {
      copy_index_file (input_folder, output_folder);
      copy_directions_file (input_folder, output_folder);
    }


    //! Copy all data files in a fixel folder into another folder. Data files do not include the index or directions file.
    FORCE_INLINE void copy_all_data_files (const std::string &input_folder, const std::string &output_folder) {
      for (auto& input_header : FixelFormat::find_data_headers (input_folder, FixelFormat::find_index_header (input_folder)))
        copy_fixel_file (input_header.name(), output_folder);
    }

    //! open a data file. checks that a user has not input a fixel folder or index image
    template <class ValueType>
    Image<ValueType> open_fixel_data_file (const std::string& input_file) {
      if (Path::is_dir (input_file))
        throw Exception ("please input the specific fixel data file to be converted (not the fixel folder)");

      Header in_data_header = Header::open (input_file);
      FixelFormat::check_data_file (in_data_header);
      auto in_data_image = in_data_header.get_image<ValueType>();

      Header in_index_header = FixelFormat::find_index_header (FixelFormat::get_fixel_folder (input_file));
      if (input_file == in_index_header.name())
        throw Exception ("input fixel data file cannot be the index file");

      return in_data_image;
    }


  }
}

#endif

