/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __fixel_helpers_h__
#define __fixel_helpers_h__

#include "formats/mrtrix_utils.h"
#include "fixel/keys.h"
#include "algo/loop.h"
#include "image_diff.h"

namespace MR
{
  class InvalidFixelDirectoryException : public Exception
  { NOMEMALIGN
    public:
      InvalidFixelDirectoryException (const std::string& msg) : Exception(msg) {}
      InvalidFixelDirectoryException (const Exception& previous_exception, const std::string& msg)
        : Exception(previous_exception, msg) {}
  };

  namespace Fixel
  {
    FORCE_INLINE bool is_index_image (const Header& in)
    {
      bool is_index = false;
      if (in.ndim() == 4) {
        if (in.size(3) == 2) {
          for (std::initializer_list<const std::string>::iterator it = supported_sparse_formats.begin();
               it != supported_sparse_formats.end(); ++it) {
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
          for (std::initializer_list<const std::string>::iterator it = supported_sparse_formats.begin();
               it != supported_sparse_formats.end(); ++it) {
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

    FORCE_INLINE std::string get_fixel_directory (const std::string& fixel_file) {
      std::string fixel_directory = Path::dirname (fixel_file);
      // assume the user is running the command from within the fixel directory
      if (fixel_directory.empty())
        fixel_directory = Path::cwd();
      return fixel_directory;
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


    FORCE_INLINE void check_fixel_directory (const std::string &path, bool create_if_missing = false, bool check_if_empty = false)
    {
      std::string path_temp = path;
      // handle the use case when a fixel command is run from inside a fixel directory
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


    FORCE_INLINE Header find_index_header (const std::string &fixel_directory_path)
    {
      Header header;
      check_fixel_directory (fixel_directory_path);

      for (std::initializer_list<const std::string>::iterator it = supported_sparse_formats.begin();
           it !=supported_sparse_formats.end(); ++it) {
        std::string full_path = Path::join (fixel_directory_path, "index" + *it);
        if (Path::exists(full_path)) {
          if (header.valid())
            throw InvalidFixelDirectoryException ("Multiple index images found in directory " + fixel_directory_path);
          header = Header::open (full_path);
        }
      }
      if (!header.valid())
        throw InvalidFixelDirectoryException ("Could not find index image in directory " + fixel_directory_path);

      check_index_image (header);
      return header;
    }


    FORCE_INLINE vector<Header> find_data_headers (const std::string &fixel_directory_path, const Header &index_header, const bool include_directions = false)
    {
      check_index_image (index_header);
      auto dir_walker = Path::Dir (fixel_directory_path);
      vector<std::string> file_names;
      {
        std::string temp;
        while ((temp = dir_walker.read_name()).size())
          file_names.push_back (temp);
      }
      std::sort (file_names.begin(), file_names.end());

      vector<Header> data_headers;
      for (auto fname : file_names) {
        if (Path::has_suffix (fname, supported_sparse_formats)) {
          try {
            auto H = Header::open (Path::join (fixel_directory_path, fname));
            if (is_data_file (H)) {
              if (fixels_match (index_header, H)) {
                if (!is_directions_file (H) || include_directions)
                  data_headers.emplace_back (std::move (H));
              } else {
                WARN ("fixel data file (" + fname + ") does not contain the same number of elements as fixels in the index file");
              }
            }
          } catch (...) {
            WARN ("unable to open file \"" + fname + "\" as potential fixel data file");
          }
        }
      }

      return data_headers;
    }



    FORCE_INLINE Header find_directions_header (const std::string fixel_directory_path)
    {
      bool directions_found (false);
      Header header;
      check_fixel_directory (fixel_directory_path);
      Header index_header = Fixel::find_index_header (fixel_directory_path);

      auto dir_walker = Path::Dir (fixel_directory_path);
      std::string fname;
      while ((fname = dir_walker.read_name ()).size ()) {
        Header tmp_header;
        auto full_path = Path::join (fixel_directory_path, fname);
        if (Path::has_suffix (fname, supported_sparse_formats)
              && is_directions_file (tmp_header = Header::open (full_path))) {
          if (is_directions_file (tmp_header)) {
            if (fixels_match (index_header, tmp_header)) {
              if (directions_found == true)
                throw Exception ("multiple directions files found in fixel image directory: " + fixel_directory_path);
              directions_found = true;
              header = std::move(tmp_header);
            } else {
              WARN ("fixel directions file (" + fname + ") does not contain the same number of elements as fixels in the index file" );
            }
          }
        }
      }

      if (!directions_found)
        throw InvalidFixelDirectoryException ("Could not find directions image in directory " + fixel_directory_path);

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

    //! Copy a file from one fixel directory into another.
    FORCE_INLINE void copy_fixel_file (const std::string& input_file_path, const std::string& output_directory) {
      check_fixel_directory (output_directory, true);
      std::string output_path = Path::join (output_directory, Path::basename (input_file_path));
      Header input_header = Header::open (input_file_path);
      auto input_image = input_header.get_image<float>();
      auto output_image = Image<float>::create (output_path, input_header);
      threaded_copy (input_image, output_image);
    }

    //! Copy the index file from one fixel directory into another
    FORCE_INLINE void copy_index_file (const std::string& input_directory, const std::string& output_directory) {
      Header input_header = Fixel::find_index_header (input_directory);
      check_fixel_directory (output_directory, true);

      std::string output_path = Path::join (output_directory, Path::basename (input_header.name()));

      // If the index file already exists check it is the same as the input index file
      if (Path::exists (output_path) && !App::overwrite_files) {
        auto input_image = input_header.get_image<uint32_t>();
        auto output_image = Image<uint32_t>::open (output_path);

        if (!images_match_abs (input_image, output_image))
          throw Exception ("output sparse image directory (" + output_directory + ") already contains index file, "
                           "which is not the same as the expected output. Use -force to override if desired");

      } else {
        auto output_image = Image<uint32_t>::create (Path::join (output_directory, Path::basename (input_header.name())), input_header);
        auto input_image = input_header.get_image<uint32_t>();
        threaded_copy (input_image, output_image);
      }
    }

    //! Copy the directions file from one fixel directory into another.
    FORCE_INLINE void copy_directions_file (const std::string& input_directory, const std::string& output_directory) {
      Header input_header = Fixel::find_directions_header (input_directory);
      std::string output_path = Path::join (output_directory, Path::basename (input_header.name()));

      // If the index file already exists check it is the same as the input index file
      if (Path::exists (output_path) && !App::overwrite_files) {
        auto input_image = input_header.get_image<uint32_t>();
        auto output_image = Image<uint32_t>::open (output_path);

        if (!images_match_abs (input_image, output_image))
          throw Exception ("output sparse image directory (" + output_directory + ") already contains a directions file, "
                           "which is not the same as the expected output. Use -force to override if desired");

      } else {
        copy_fixel_file (input_header.name(), output_directory);
      }

    }

    FORCE_INLINE void copy_index_and_directions_file (const std::string& input_directory, const std::string &output_directory) {
      copy_index_file (input_directory, output_directory);
      copy_directions_file (input_directory, output_directory);
    }


    //! Copy all data files in a fixel directory into another directory. Data files do not include the index or directions file.
    FORCE_INLINE void copy_all_data_files (const std::string &input_directory, const std::string &output_directory) {
      for (auto& input_header : Fixel::find_data_headers (input_directory, Fixel::find_index_header (input_directory)))
        copy_fixel_file (input_header.name(), output_directory);
    }

    //! open a data file. checks that a user has not input a fixel directory or index image
    template <class ValueType>
    Image<ValueType> open_fixel_data_file (const std::string& input_file) {
      if (Path::is_dir (input_file))
        throw Exception ("please input the specific fixel data file to be converted (not the fixel directory)");

      Header in_data_header = Header::open (input_file);
      Fixel::check_data_file (in_data_header);
      auto in_data_image = in_data_header.get_image<ValueType>();

      Header in_index_header = Fixel::find_index_header (Fixel::get_fixel_directory (input_file));
      if (input_file == in_index_header.name())
        throw Exception ("input fixel data file cannot be the index file");

      return in_data_image;
    }
  }
}

#endif

