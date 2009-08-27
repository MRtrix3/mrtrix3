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

#include "file/misc.h"
#include "file/entry.h"
#include "file/path.h"
#include "file/key_value.h"
#include "image/misc.h"
#include "image/header.h"
#include "image/name_parser.h"
#include "image/format/list.h"

namespace MR {
  namespace Image {
    namespace Format {

      // extensions are: 
      // mih: MRtrix Image Header
      // mif: MRtrix Image File

      bool MRtrix::read (Header& H) const
      { 
        if (!Path::has_suffix (H.name(), ".mih") && !Path::has_suffix (H.name(), ".mif")) return (false);

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
          else if (key == "comments") H.comments.push_back (kv.value());
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
          else error ("WARNING: invalid key \"" + kv.key() + " in MRtrix image header \"" + H.name() + "\" - ignored");
        }

        if (dim.empty()) throw Exception ("missing \"dim\" specification for MRtrix image \"" + H.name() + "\"");
        H.axes.ndim() = dim.size();
        for (size_t n = 0; n < dim.size(); n++) {
          if (dim[n] < 1) throw Exception ("invalid dimensions for MRtrix image \"" + H.name() + "\"");
          H.axes.dim(n) = dim[n];
        }

        if (vox.empty()) throw Exception ("missing \"vox\" specification for MRtrix image \"" + H.name() + "\"");
        for (size_t n = 0; n < H.axes.ndim(); n++) {
          if (vox[n] < 0.0) throw Exception ("invalid voxel size for MRtrix image \"" + H.name() + "\"");
          H.axes.vox(n) = vox[n];
        }


        if (dtype.empty()) throw Exception ("missing \"datatype\" specification for MRtrix image \"" + H.name() + "\"");
        H.datatype().parse (dtype);


        if (layout.empty()) throw Exception ("missing \"layout\" specification for MRtrix image \"" + H.name() + "\"");
        std::vector<Axes::Order> ax = parse_axes_specifier (H.axes, layout);
        if (ax.size() != H.axes.ndim()) 
          throw Exception ("specified layout does not match image dimensions for MRtrix image \"" + H.name() + "\"");

        for (size_t i = 0; i < ax.size(); i++) {
          H.axes.order(i) = ax[i].order;
          H.axes.forward(i) = ax[i].forward;
        }

        for (size_t n = 0; n < MIN (H.axes.ndim(), labels.size()); n++) H.axes.description(n) = labels[n];
        for (size_t n = 0; n < MIN (H.axes.ndim(), units.size()); n++) H.axes.units(n) = units[n];

        if (transform.size()) {
          if (transform.size() < 9) throw Exception ("invalid \"transform\" specification for MRtrix image \"" + H.name() + "\"");
          Math::Matrix<float> T (4,4);
          int count = 0;
          for (int row = 0; row < 3; row++) 
            for (int col = 0; col < 4; col++) 
              T(row,col) = transform[count++];
          T(3,0) = T(3,1) = T(3,2) = 0.0; T(3,3) = 1.0;
          H.transform() = T;
        }


        if (dw_scheme.size()) {
          if (dw_scheme.size() % 4) info ("WARNING: invalid \"dw_scheme\" specification for MRtrix image \"" + H.name() + "\" - ignored");
          else {
            Math::Matrix<float> M (dw_scheme.size()/4, 4);
            int count = 0;
            for (size_t row = 0; row < M.rows(); row++) 
              for (size_t col = 0; col < 4; col++) 
                M(row,col) = dw_scheme[count++];
            H.DW_scheme = M;
          }
        }


        if (scaling.size()) {
          if (scaling.size() != 2) throw Exception ("invalid \"scaling\" specification for MRtrix image \"" + H.name() + "\"");
          H.offset = scaling[0];
          H.scale = scaling[1];
        }

        if (file.empty()) throw Exception ("missing \"file\" specification for MRtrix image \"" + H.name() + "\"");
        std::istringstream files_stream (file);
        std::string fname;
        files_stream >> fname;
        size_t offset = 0;
        if (files_stream.good()) {
          try { files_stream >> offset; }
          catch (...) { throw Exception ("invalid offset specified for file \"" + fname + "\" in MRtrix image header \"" + H.name() + "\""); }
        }

        if (fname == ".") {
          if (offset == 0) throw Exception ("invalid offset specified for embedded MRtrix image \"" + H.name() + "\""); 
          fname = H.name();
        }
        else fname = Path::join (Path::dirname (H.name()), fname);

        ParsedNameList list;
        std::vector<int> num = list.parse_scan_check (fname);

        for (size_t n = 0; n < list.size(); n++) 
          H.files.push_back (File::Entry (list[n]->name(), offset));

        return (true);
      }





      bool MRtrix::check (Header& H, int num_axes) const
      {
        if (H.name().size() && !Path::has_suffix (H.name(), ".mih") && !Path::has_suffix (H.name(), ".mif")) return (false);

        H.axes.ndim() = num_axes;
        for (size_t i = 0; i < H.axes.ndim(); i++) 
          if (H.axes.dim(i) < 1) H.axes.dim(i) = 1;

        return (true);
      }




      void MRtrix::create (Header& H) const
      {
        File::create (H.name());
        std::ofstream out (H.name().c_str(), std::ios::out | std::ios::binary);
        if (!out) throw Exception ("error creating file \"" + H.name() + "\":" + strerror(errno));

        out << "mrtrix image\n";
        out << "dim: " << H.axes.dim(0);
        for (size_t n = 1; n < H.axes.ndim(); n++) out << "," << H.axes.dim(n);

        out << "\nvox: " << H.axes.vox(0);
        for (size_t n = 1; n < H.axes.ndim(); n++) out << "," << H.axes.vox(n);

        out << "\nlayout: " << ( H.axes.forward(0) ? "+" : "-" ) << H.axes.order(0);
        for (size_t n = 1; n < H.axes.ndim(); n++) out << "," << ( H.axes.forward(n) ? "+" : "-" ) << H.axes.order(n);

        out << "\ndatatype: " << H.datatype().specifier();

        out << "\nlabels: " << H.axes.description(0);
        for (size_t n = 1; n < H.axes.ndim(); n++) out << "\\" << H.axes.description(n);

        out << "\nunits: " <<  H.axes.units(0);
        for (size_t n = 1; n < H.axes.ndim(); n++) out << "\\" << H.axes.units(n);

        for (std::vector<std::string>::const_iterator i = H.comments.begin(); i != H.comments.end(); i++) 
          out << "\ncomments: " << *i;


        if (H.transform().is_set()) {
          out << "\ntransform: " << H.transform()(0,0) << "," <<  H.transform()(0,1) << "," << H.transform()(0,2) << "," << H.transform()(0,3);
          out << "\ntransform: " << H.transform()(1,0) << "," <<  H.transform()(1,1) << "," << H.transform()(1,2) << "," << H.transform()(1,3);
          out << "\ntransform: " << H.transform()(2,0) << "," <<  H.transform()(2,1) << "," << H.transform()(2,2) << "," << H.transform()(2,3);
        }

        if (H.offset != 0.0 || H.scale != 1.0) 
          out << "\nscaling: " << H.offset << "," << H.scale;

        if (H.DW_scheme.is_set()) {
          for (size_t i = 0; i < H.DW_scheme.rows(); i++)
            out << "\ndw_scheme: " << H.DW_scheme (i,0) << "," << H.DW_scheme (i,1) << "," << H.DW_scheme (i,2) << "," << H.DW_scheme (i,3);
        }

        bool single_file = Path::has_suffix (H.name(), ".mif");

        off64_t offset = 0;
        out << "\nfile: ";
        if (single_file) {
          offset = out.tellp();
          offset += 14;
          out << ". " << offset << "\nEND\n";
        }
        else out << Path::basename (H.name().substr (0, H.name().size()-4) + ".dat") << "\n";

        out.close();

        if (single_file) {
          File::resize (H.name(), offset + memory_footprint (H));
          H.files.push_back (File::Entry (H.name(), offset));
        }
        else {
          std::string data_file (H.name().substr (0, H.name().size()-4) + ".dat");
          File::create (data_file, memory_footprint (H));
          H.files.push_back (File::Entry (data_file));
        }
      }


    }
  }
}
