/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <unistd.h>
#include <fcntl.h>
#include <fstream>

#include "types.h"
#include "file/utils.h"
#include "file/entry.h"
#include "file/path.h"
#include "file/key_value.h"
#include "image/utils.h"
#include "image/header.h"
#include "image/handler/default.h"
#include "image/name_parser.h"
#include "image/format/list.h"

namespace MR
{
  namespace Image
  {
    namespace Format
    {

      // extensions are:
      // mih: MRtrix Image Header
      // mif: MRtrix Image File

      Handler::Base* MRtrix::read (Header& H) const
      {
        if (!Path::has_suffix (H.name(), ".mih") && !Path::has_suffix (H.name(), ".mif"))
          return NULL;

        File::KeyValue kv (H.name(), "mrtrix image");

        std::string dtype, layout, file;
        std::vector<int> dim;
        std::vector<float> transform, dw_scheme, vox, scaling;
        std::vector<std::string> units, labels;

        while (kv.next()) {
          std::string key = lowercase (kv.key());
          if (key == "dim") dim = parse_ints (kv.value());
          else if (key == "vox") vox = parse_floats (kv.value());
          else if (key == "layout") layout = kv.value();
          else if (key == "datatype") dtype = kv.value();
          else if (key == "file") file = kv.value();
          else if (key == "scaling") scaling = parse_floats (kv.value());
          else if (key == "comments") H.comments().push_back (kv.value());
          else if (key == "units") units = split (kv.value(), "\\");
          else if (key == "labels") labels = split (kv.value(), "\\");
          else if (key == "transform") {
            std::vector<float> V (parse_floats (kv.value()));
            transform.insert (transform.end(), V.begin(), V.end());
          }
          else if (key == "dw_scheme") {
            std::vector<float> V (parse_floats (kv.value()));
            dw_scheme.insert (dw_scheme.end(), V.begin(), V.end());
          }
          else H[key] = kv.value();
        }

        if (dim.empty())
          throw Exception ("missing \"dim\" specification for MRtrix image \"" + H.name() + "\"");
        H.set_ndim (dim.size());
        for (size_t n = 0; n < dim.size(); n++) {
          if (dim[n] < 1)
            throw Exception ("invalid dimensions for MRtrix image \"" + H.name() + "\"");
          H.dim(n) = dim[n];
        }

        if (vox.empty())
          throw Exception ("missing \"vox\" specification for MRtrix image \"" + H.name() + "\"");
        for (size_t n = 0; n < H.ndim(); n++) {
          if (vox[n] < 0.0)
            throw Exception ("invalid voxel size for MRtrix image \"" + H.name() + "\"");
          H.vox(n) = vox[n];
        }


        if (dtype.empty())
          throw Exception ("missing \"datatype\" specification for MRtrix image \"" + H.name() + "\"");
        H.datatype() = DataType::parse (dtype);


        if (layout.empty())
          throw Exception ("missing \"layout\" specification for MRtrix image \"" + H.name() + "\"");
        std::vector<ssize_t> ax = Axis::parse (H.ndim(), layout);
        for (size_t i = 0; i < ax.size(); ++i)
          H.stride(i) = ax[i];

        if (transform.size()) {

          if (transform.size() < 9)
            throw Exception ("invalid \"transform\" specification for MRtrix image \"" + H.name() + "\"");

          H.transform().allocate (4,4);
          int count = 0;
          for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 4; ++col)
              H.transform() (row,col) = transform[count++];
          H.transform()(3,0) = H.transform()(3,1) = H.transform()(3,2) = 0.0;
          H.transform()(3,3) = 1.0;
        }


        if (dw_scheme.size()) {
          if (dw_scheme.size() % 4)
            inform ("WARNING: invalid \"dw_scheme\" specification for MRtrix image \"" + H.name() + "\" - ignored");
          else {
            H.DW_scheme().allocate (dw_scheme.size() /4, 4);
            int count = 0;
            for (size_t row = 0; row < H.DW_scheme().rows(); ++row)
              for (size_t col = 0; col < 4; ++col)
                H.DW_scheme()(row,col) = dw_scheme[count++];
          }
        }


        if (scaling.size()) {
          if (scaling.size() != 2)
            throw Exception ("invalid \"scaling\" specification for MRtrix image \"" + H.name() + "\"");
          H.intensity_offset() = scaling[0];
          H.intensity_scale() = scaling[1];
        }

        if (file.empty())
          throw Exception ("missing \"file\" specification for MRtrix image \"" + H.name() + "\"");

        std::istringstream files_stream (file);
        std::string fname;
        files_stream >> fname;
        size_t offset = 0;
        if (files_stream.good()) {
          try {
            files_stream >> offset;
          }
          catch (...) {
            throw Exception ("invalid offset specified for file \"" + fname
                             + "\" in MRtrix image header \"" + H.name() + "\"");
          }
        }

        if (fname == ".") {
          if (offset == 0)
            throw Exception ("invalid offset specified for embedded MRtrix image \"" + H.name() + "\"");
          fname = H.name();
        }
        else fname = Path::join (Path::dirname (H.name()), fname);

        ParsedName::List list;
        std::vector<int> num = list.parse_scan_check (fname);

        Ptr<Handler::Base> handler (new Handler::Default (H));
        for (size_t n = 0; n < list.size(); ++n)
          handler->files.push_back (File::Entry (list[n].name(), offset));

        return handler.release();
      }





      bool MRtrix::check (Header& H, size_t num_axes) const
      {
        if (!Path::has_suffix (H.name(), ".mih") &&
            !Path::has_suffix (H.name(), ".mif"))
          return false;

        H.set_ndim (num_axes);
        for (size_t i = 0; i < H.ndim(); i++)
          if (H.dim (i) < 1)
            H.dim(i) = 1;

        return true;
      }




      Handler::Base* MRtrix::create (Header& H) const
      {
        if (!File::is_tempfile (H.name()))
          File::create (H.name());

        std::ofstream out (H.name().c_str(), std::ios::out | std::ios::binary);
        if (!out)
          throw Exception ("error creating file \"" + H.name() + "\":" + strerror (errno));

        out << "mrtrix image\n";
        out << "dim: " << H.dim (0);
        for (size_t n = 1; n < H.ndim(); ++n)
          out << "," << H.dim (n);

        out << "\nvox: " << H.vox (0);
        for (size_t n = 1; n < H.ndim(); ++n)
          out << "," << H.vox (n);

        out << "\nlayout: " << (H.stride (0) >0 ? "+" : "-") << abs (H.stride (0))-1;
        for (size_t n = 1; n < H.ndim(); ++n)
          out << "," << (H.stride (n) >0 ? "+" : "-") << abs (H.stride (n))-1;

        out << "\ndatatype: " << H.datatype().specifier();

        for (std::map<std::string, std::string>::iterator i = H.begin(); i != H.end(); ++i)
          out << "\n" << i->first << ": " << i->second;

        for (std::vector<std::string>::const_iterator i = H.comments().begin(); i != H.comments().end(); i++)
          out << "\ncomments: " << *i;


        if (H.transform().is_set()) {
          out << "\ntransform: " << H.transform() (0,0) << "," <<  H.transform() (0,1) << "," << H.transform() (0,2) << "," << H.transform() (0,3);
          out << "\ntransform: " << H.transform() (1,0) << "," <<  H.transform() (1,1) << "," << H.transform() (1,2) << "," << H.transform() (1,3);
          out << "\ntransform: " << H.transform() (2,0) << "," <<  H.transform() (2,1) << "," << H.transform() (2,2) << "," << H.transform() (2,3);
        }

        if (H.intensity_offset() != 0.0 || H.intensity_scale() != 1.0)
          out << "\nscaling: " << H.intensity_offset() << "," << H.intensity_scale();

        if (H.DW_scheme().is_set()) {
          for (size_t i = 0; i < H.DW_scheme().rows(); i++)
            out << "\ndw_scheme: " << H.DW_scheme() (i,0) << "," << H.DW_scheme() (i,1) << "," << H.DW_scheme() (i,2) << "," << H.DW_scheme() (i,3);
        }

        bool single_file = Path::has_suffix (H.name(), ".mif");

        int64_t offset = 0;
        out << "\nfile: ";
        if (single_file) {
          offset = out.tellp();
          offset += 14;
          out << ". " << offset << "\nEND\n";
        }
        else out << Path::basename (H.name().substr (0, H.name().size()-4) + ".dat") << "\n";

        out.close();

        Ptr<Handler::Base> handler (new Handler::Default (H));
        if (single_file) {
          File::resize (H.name(), offset + Image::footprint(H));
          handler->files.push_back (File::Entry (H.name(), offset));
        }
        else {
          std::string data_file (H.name().substr (0, H.name().size()-4) + ".dat");
          File::create (data_file, Image::footprint(H));
          handler->files.push_back (File::Entry (data_file));
        }

        return handler.release();
      }


    }
  }
}
