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


#ifndef __formats_mrtrix_utils_h__
#define __formats_mrtrix_utils_h__

#include <vector>

#include "header.h"
#include "file/gz.h"
#include "file/key_value.h"
#include "file/ofstream.h"
#include "file/path.h"
#include "file/utils.h"



namespace MR
{
  namespace Formats
  {


    // Read generic image header information - common between conventional, compressed and sparse formats
    template <class SourceType>
      void read_mrtrix_header (Header&, SourceType&);

    // These are helper functiosn for reading key/value pairs from either a File::KeyValue construct,
    //   or from a GZipped file (where the getline() function must be used explicitly)
    bool next_keyvalue (File::KeyValue&, std::string&, std::string&);
    bool next_keyvalue (File::GZ&,       std::string&, std::string&);

    // Get the path to a file - use same function for image data and sparse data
    // Note that the 'file' and 'sparse_file' fields are read in as entries in the map<string, string>
    //   by read_mrtrix_header(), and are therefore erased by this function so that they do not propagate
    //   into new images created using this header
    void get_mrtrix_file_path (Header&, const std::string&, std::string&, size_t&);

    // Write generic image header information to a stream -
    //   this could be an ofstream in the case of .mif, or a stringstream in the case of .mif.gz
    template <class StreamType>
      void write_mrtrix_header (const Header&, StreamType&);


    vector<ssize_t> parse_axes (size_t ndim, const std::string& specifier);




    template <class SourceType>
      void read_mrtrix_header (Header& H, SourceType& kv)
      {
        std::string dtype, layout;
        vector<int> dim;
        vector<default_type> vox, scaling;
        vector<vector<default_type>> transform;

        std::string key, value;
        while (next_keyvalue (kv, key, value)) {
          const std::string lkey = lowercase (key);
          if (lkey == "dim") dim = parse_ints (value);
          else if (lkey == "vox") vox = parse_floats (value);
          else if (lkey == "layout") layout = value;
          else if (lkey == "datatype") dtype = value;
          else if (lkey == "scaling") scaling = parse_floats (value);
          else if (lkey == "transform")
            transform.push_back (parse_floats (value));
          else if (key.size() && value.size())
            add_line (H.keyval()[key], value); // Preserve capitalization if not a compulsory key
        }

        if (dim.empty())
          throw Exception ("missing \"dim\" specification for MRtrix image \"" + H.name() + "\"");
        H.ndim() = dim.size();
        for (size_t n = 0; n < dim.size(); n++) {
          if (dim[n] < 1)
            throw Exception ("invalid dimensions for MRtrix image \"" + H.name() + "\"");
          H.size(n) = dim[n];
        }

        if (vox.empty())
          throw Exception ("missing \"vox\" specification for MRtrix image \"" + H.name() + "\"");
        if (vox.size() < 3)
          throw Exception ("too few entries in \"vox\" specification for MRtrix image \"" + H.name() + "\"");
        for (size_t n = 0; n < std::min<size_t> (vox.size(), H.ndim()); n++) {
          if (vox[n] < 0.0)
            throw Exception ("invalid voxel size for MRtrix image \"" + H.name() + "\"");
          H.spacing(n) = vox[n];
        }


        if (dtype.empty())
          throw Exception ("missing \"datatype\" specification for MRtrix image \"" + H.name() + "\"");
        H.datatype() = DataType::parse (dtype);


        if (layout.empty())
          throw Exception ("missing \"layout\" specification for MRtrix image \"" + H.name() + "\"");
        auto ax = parse_axes (H.ndim(), layout);
        for (size_t i = 0; i < ax.size(); ++i)
          H.stride(i) = ax[i];

        if (transform.size()) {

          auto check_transform = [&transform]() {
            if (transform.size() < 3) return false;
            for (auto row : transform)
              if (row.size() != 4)
                return false;
            return true;
          };
          if (!check_transform())
            throw Exception ("invalid \"transform\" specification for MRtrix image \"" + H.name() + "\"");

          for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 4; ++col)
              H.transform() (row,col) = transform[row][col];
        }


        if (scaling.size()) {
          if (scaling.size() != 2)
            throw Exception ("invalid \"scaling\" specification for MRtrix image \"" + H.name() + "\"");
          H.set_intensity_scaling (scaling[1], scaling[0]);
        }

      }







    template <class StreamType>
      void write_mrtrix_header (const Header& H, StreamType& out)
      {
        out << "dim: " << H.size (0);
        for (size_t n = 1; n < H.ndim(); ++n)
          out << "," << H.size (n);

        out << "\nvox: " << H.spacing (0);
        for (size_t n = 1; n < H.ndim(); ++n)
          out << "," << H.spacing (n);

        auto stride = Stride::get (H);
        Stride::symbolise (stride);

        out << "\nlayout: " << (stride[0] >0 ? "+" : "-") << std::abs (stride[0])-1;
        for (size_t n = 1; n < H.ndim(); ++n)
          out << "," << (stride[n] >0 ? "+" : "-") << std::abs (stride[n])-1;

        DataType dt = H.datatype();
        dt.set_byte_order_native();
        out << "\ndatatype: " << dt.specifier();

        Eigen::IOFormat fmt(Eigen::FullPrecision, Eigen::DontAlignCols, ", ", "\ntransform: ", "", "", "\ntransform: ", "");
        out << H.transform().matrix().topLeftCorner(3,4).format(fmt);

        if (H.intensity_offset() != 0.0 || H.intensity_scale() != 1.0)
          out << "\nscaling: " << H.intensity_offset() << "," << H.intensity_scale();

        for (const auto i : H.keyval())
          for (const auto line : split_lines (i.second))
            out << "\n" << i.first << ": " << line;


        out << "\n";
      }





  }
}

#endif

