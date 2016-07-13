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
    inline bool is_index_image (const Header& in)
    {
      return in.keyval ().count (n_fixels_key);
    }


    inline void check_index_image (const Header& in)
    {
      if (!is_index_image (in))
        throw InvalidImageException (in.name () + " is not a valid fixel index image. Header key " + n_fixels_key + " not found");
    }


    inline bool is_data_image (const Header& in)
    {
      return in.ndim () == 3 && in.size (2) == 1;
    }


    inline void check_data_image (const Header& in)
    {
      if (!is_data_image (in))
        throw InvalidImageException (in.name () + " is not a valid fixel data image. Expected a 3-dimensionl image of size n x m x 1");
    }


    inline bool fixels_match (const Header& index_h, const Header& data_h)
    {
      bool fixels_match (false);

      if (is_index_image (index_h)) {
        fixels_match = std::stoul(index_h.keyval ().at (n_fixels_key)) == (unsigned long)data_h.size (0);
      }

      return fixels_match;
    }


    inline void check_fixel_size (const Header& index_h, const Header& data_h)
    {
      check_index_image (index_h);
      check_data_image (data_h);

      if (!fixels_match (index_h, data_h))
        throw InvalidImageException ("Fixel number mismatch between index image " + index_h.name() + " and data image " + data_h.name());
    }


    inline void check_fixel_folder (const std::string &path, bool create_if_missing = false, bool check_if_empty = false)
    {
      bool exists (true);

      if (!(exists = Path::exists (path))) {
        if (create_if_missing) File::mkdir (path);
        else throw Exception ("Fixel directory " + str(path) + " does not exist");
      }
      else if (!Path::is_dir (path))
        throw Exception (str(path) + " is not a directory");

      if (check_if_empty && Path::Dir (path).read_name ().size () != 0)
        throw Exception ("Expected fixel directory " + path + " to be empty.");
    }


    inline Header find_index_header (const std::string &fixel_folder_path)
    {
      bool index_found (false);
      Header H;
      check_fixel_folder (fixel_folder_path);

      auto dir_walker = Path::Dir (fixel_folder_path);
      std::string fname;
      while ((fname = dir_walker.read_name ()).size ()) {
        auto full_path = Path::join (fixel_folder_path, fname);
        if (Path::has_suffix (fname, FixelFormat::supported_fixel_formats) && is_index_image (H = Header::open (full_path))) {
          index_found = true;
          break;
        }
      }

      if (!index_found)
        throw InvalidFixelDirectoryException ("Could not find index image in directory " + fixel_folder_path);

      return H;
    }


    inline std::vector<Header> find_data_headers (const std::string &fixel_folder_path, const Header &index_header)
    {
      check_index_image (index_header);

      std::vector<Header> data_headers;

      auto dir_walker = Path::Dir (fixel_folder_path);
      std::string fname;
      while ((fname = dir_walker.read_name ()).size ()) {
        auto full_path = Path::join (fixel_folder_path, fname);
        Header H;
        if (Path::has_suffix (fname, FixelFormat::supported_fixel_formats)
                && is_data_image (H = Header::open (full_path))
                && fixels_match (index_header, H)) {
          data_headers.emplace_back (std::move (H));
        }
      }

      return data_headers;
    }
  }
}

#endif

