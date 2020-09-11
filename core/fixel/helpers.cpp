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

#include "fixel/helpers.h"

#include "image_diff.h"


namespace MR
{


  namespace Peaks
  {
    void check (const Header& in)
    {
      if (!in.datatype().is_floating_point())
        throw Exception ("Image \"" + in.name() + "\" is not a valid peaks image: Does not contain floating-point data");
      try {
        check_effective_dimensionality (in, 4);
      } catch (Exception& e) {
        throw Exception (e, "Image \"" + in.name() + "\" is not a valid peaks image: Expect 4 dimensions");
      }
      if (in.size(3) % 3)
        throw Exception ("Image \"" + in.name() + "\" is not a valid peaks image: Number of volumes must be a multiple of 3");
    }
  }



  namespace Fixel
  {



    bool is_index_filename (const std::string& path)
    {
      for (std::initializer_list<const std::string>::iterator it = supported_sparse_formats.begin();
           it != supported_sparse_formats.end(); ++it) {
        if (Path::basename (path) == "index" + *it)
          return true;
      }
      return false;
    }



    bool is_directions_filename (const std::string& path)
    {
      for (std::initializer_list<const std::string>::iterator it = supported_sparse_formats.begin();
           it != supported_sparse_formats.end(); ++it) {
        if (Path::basename (path) == "directions" + *it)
          return true;
      }
      return false;
    }



    std::string get_fixel_directory (const std::string& fixel_file) {
      std::string fixel_directory = Path::dirname (fixel_file);
      // assume the user is running the command from within the fixel directory
      if (fixel_directory.empty())
        fixel_directory = Path::cwd();
      return fixel_directory;
    }



    void check_fixel_size (const Header& index_h, const Header& data_h)
    {
      check_index_image (index_h);
      check_data_file (data_h);
      if (!fixels_match (index_h, data_h))
        throw InvalidImageException ("Fixel number mismatch between index image " + index_h.name() + " and data image " + data_h.name());
    }



    Header find_index_header (const std::string &fixel_directory_path)
    {
      Header header;

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



    Header find_directions_header (const std::string fixel_directory_path)
    {
      Header index_header = Fixel::find_index_header (fixel_directory_path);

      bool directions_found (false);
      Header header;

      auto dir_walker = Path::Dir (fixel_directory_path);
      std::string fname;
      while ((fname = dir_walker.read_name()).size()) {
        if (is_directions_filename (fname)) {
          Header tmp_header = Header::open (Path::join (fixel_directory_path, fname));
          if (is_directions_file (tmp_header)) {
            if (fixels_match (index_header, tmp_header)) {
              if (directions_found == true)
                throw Exception ("multiple directions files found in fixel image directory: " + fixel_directory_path);
              directions_found = true;
              header = std::move (tmp_header);
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



    vector<Header> find_data_headers (const std::string &fixel_directory_path,
                                      const Header &index_header,
                                      const bool include_directions)
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



    void check_fixel_directory_in (const std::string& path)
    {
      const std::string path_temp = path.empty() ? Path::cwd() : path;
      if (!Path::is_dir (path_temp))
        throw Exception ("Fixel directory (" + str(path_temp) + ") does not exist");
      try {
        find_index_header (path_temp);
        find_directions_header (path_temp);
      } catch (Exception& e) {
        throw Exception (e, "Unable to interpret \"" + path + "\" as input fixel directory");
      }
    }



    void check_fixel_directory_out (const std::string& path,
                                    const bool new_index,
                                    const bool new_directions)
    {
      const std::string path_temp = path.empty() ? Path::cwd() : path;

      if (Path::exists (path_temp)) {
        if (Path::is_dir (path_temp)) {
          if (Path::Dir (path_temp).read_name ().size () != 0) { // Existing content in directory
            if (new_index) {
              if (App::overwrite_files) {
                WARN("Contents of existing directory \"" + path + "\" being erased");
                File::rmdir (path_temp, true);
                File::mkdir (path_temp);
              } else {
                throw Exception("Output fixel directory \"" + path + "\" already exists and is not empty (use -force to override)");
              }
            } // If we're not writing index & directions, we don't care if the output directory has pre-existing content
          } // Empty directory; if core files are not intended to be written by the command, the command is responsible for duplicating index & directions
        } else { // Exists, but is a file rather than a directory
          if (App::overwrite_files) {
            WARN("Existing file \"" + path_temp + "\" being erased ahead of fixel directory creation");
            File::remove (path_temp);
            File::mkdir (path_temp);
          } else {
            throw Exception ("Target output fixel directory \"" + path + "\" already exists as a file (use -force to override)");
          }
        }
      } else { // Doesn't exist
        if (!new_index && !new_directions)
          throw Exception("Output fixel directory \"" + path + "\" does not exist");
        File::mkdir (path_temp);
      }
    }



    void copy_fixel_file (const std::string& input_file_path, const std::string& output_directory)
    {
      check_fixel_directory_out (output_directory, false, false);
      std::string output_path = Path::join (output_directory, Path::basename (input_file_path));
      Header input_header = Header::open (input_file_path);
      auto input_image = input_header.get_image<float>();
      auto output_image = Image<float>::create (output_path, input_header);
      threaded_copy (input_image, output_image);
    }



    void copy_index_file (const std::string& input_directory, const std::string& output_directory)
    {
      Header input_header = Fixel::find_index_header (input_directory);
      check_fixel_directory_out (output_directory, true, false);

      std::string output_path = Path::join (output_directory, Path::basename (input_header.name()));

      // If the index file already exists check it is the same as the input index file
      if (Path::exists (output_path) && !App::overwrite_files) {
        auto input_image = input_header.get_image<index_type>();
        auto output_image = Image<index_type>::open (output_path);

        if (!images_match_abs (input_image, output_image))
          throw Exception ("output fixel directory (" + output_directory + ") already contains index file, "
                           "which is not the same as the expected output. Use -force to override if desired");

      } else {
        auto output_image = Image<index_type>::create (Path::join (output_directory, Path::basename (input_header.name())), input_header);
        auto input_image = input_header.get_image<index_type>();
        threaded_copy (input_image, output_image);
      }
    }



    void copy_directions_file (const std::string& input_directory, const std::string& output_directory)
    {
      Header input_header = Fixel::find_directions_header (input_directory);
      std::string output_path = Path::join (output_directory, Path::basename (input_header.name()));

      // If the index file already exists check it is the same as the input index file
      if (Path::exists (output_path) && !App::overwrite_files) {
        auto input_image = input_header.get_image<index_type>();
        auto output_image = Image<index_type>::open (output_path);

        if (!images_match_abs (input_image, output_image))
          throw Exception ("output sparse image directory (" + output_directory + ") already contains a directions file, "
                           "which is not the same as the expected output. Use -force to override if desired");

      } else {
        copy_fixel_file (input_header.name(), output_directory);
      }

    }

    void copy_index_and_directions_file (const std::string& input_directory, const std::string &output_directory)
    {
      copy_index_file (input_directory, output_directory);
      copy_directions_file (input_directory, output_directory);
    }



    void copy_all_data_files (const std::string &input_directory, const std::string &output_directory)
    {
      for (auto& input_header : Fixel::find_data_headers (input_directory, Fixel::find_index_header (input_directory)))
        copy_fixel_file (input_header.name(), output_directory);
    }



  }
}
