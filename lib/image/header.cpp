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

#include "app.h"
#include "image/header.h"
#include "image/stride.h"
#include "image/handler/base.h"
#include "image/name_parser.h"
#include "image/format/list.h"
#include "image/handler/default.h"
#include "math/permutation.h"

namespace MR
{
  namespace Image
  {

    bool Header::do_not_realign_transform = false;



    void Header::merge (const Header& H)
    {
      if (datatype() != H.datatype())
        throw Exception ("data types differ between image files for \"" + name() + "\"");

      if (offset_ != H.offset_ || scale_ != H.scale_)
        throw Exception ("scaling coefficients differ between image files for \"" + name() + "\"");

      if (ndim() != H.ndim())
        throw Exception ("dimension mismatch between image files for \"" + name() + "\"");

      for (size_t n = 0; n < ndim(); ++n) {
        if (dim (n) != H.dim (n))
          throw Exception ("dimension mismatch between image files for \"" + name() + "\"");

        if (stride (n) != H.stride (n))
          throw Exception ("data strides differs image files for \"" + name() + "\"");

        if (std::isfinite(vox(n)) && std::isfinite(H.vox(n)) && vox (n) != H.vox (n))
          WARN ("voxel dimensions differ between image files for \"" + name() + "\"");
      }

      if (!transform().is_set() && H.transform().is_set())
        transform() = H.transform();

      if (!DW_scheme().is_set() && H.DW_scheme().is_set())
        DW_scheme() = H.DW_scheme();

      for (std::map<std::string, std::string>::const_iterator item = H.begin(); item != H.end(); ++item)
        if (std::find (begin(), end(), *item) == end())
          insert (*item);

      for (std::vector<std::string>::const_iterator item = H.comments_.begin(); item != H.comments_.end(); ++item)
        if (std::find (comments_.begin(), comments_.end(), *item) == comments_.end())
          comments().push_back (*item);

    }







    void Header::open (const std::string& image_name)
    {
      if (image_name.empty())
        throw Exception ("no name supplied to open image!");

      try {
        INFO ("opening image \"" + image_name + "\"...");

        ParsedName::List list;
        std::vector<int> num = list.parse_scan_check (image_name);

        const Format::Base** format_handler = Format::handlers;
        size_t item = 0;
        name() = list[item].name();

        for (; *format_handler; format_handler++) {
          if ( (handler_ = (*format_handler)->read (*this)) )
            break;
        }

        if (!*format_handler)
          throw Exception ("unknown format for image \"" + name() + "\"");
        assert (handler_);

        format_ = (*format_handler)->description;

        while (++item < list.size()) {
          Header header (*this);
          RefPtr<Handler::Base> H_handler;
          header.name() = list[item].name();
          if (!(H_handler = (*format_handler)->read (header)))
            throw Exception ("image specifier contains mixed format files");
          assert (H_handler);
          merge (header);
          handler_->merge (*H_handler);
        }

        if (num.size()) {
          int a = 0, n = 0;
          for (size_t i = 0; i < ndim(); i++)
            if (stride (i))
              ++n;
          set_ndim (n + num.size());

          for (size_t i = 0; i < num.size(); ++i) {
            while (stride (a)) ++a;
            dim(a) = num[num.size()-1-i];
            stride(a) = ++n;
          }
          name() = image_name;
        }

        sanitise();
        if (!do_not_realign_transform) 
          realign_transform();
        handler_->set_name (name());
      }
      catch (Exception& E) {
        throw Exception (E, "error opening image \"" + image_name + "\"");
      }
    }





    void Header::create (const std::string& image_name)
    {
      if (image_name.empty())
        throw Exception ("no name supplied to open image!");

      try {
        INFO ("creating image \"" + image_name + "\"...");

        (*this)["mrtrix_version"] = App::mrtrix_version;
        if (App::project_version)
          (*this)["project_version"] = App::project_version;

        sanitise();

        NameParser parser;
        parser.parse (image_name);
        std::vector<int> Pdim (parser.ndim());

        std::vector<int> Hdim (ndim());
        for (size_t i = 0; i < ndim(); ++i)
          Hdim[i] = dim (i);

        name() = image_name;

        const Format::Base** format_handler = Format::handlers;
        for (; *format_handler; format_handler++)
          if ((*format_handler)->check (*this, ndim() - Pdim.size()))
            break;

        if (!*format_handler)
          throw Exception ("unknown format for image \"" + image_name + "\"");

        format_ = (*format_handler)->description;

        datatype().set_byte_order_native();
        int a = 0;
        for (size_t n = 0; n < Pdim.size(); ++n) {
          while (a < int(ndim()) && stride (a))
            a++;
          Pdim[n] = Hdim[a];
        }
        parser.calculate_padding (Pdim);

        Header header (*this);
        std::vector<int> num (Pdim.size());

        if (image_name != "-")
          name() = parser.name (num);

        handler_ = (*format_handler)->create (*this);

        assert (handler_);

        while (get_next (num, Pdim)) {
          header.name() = parser.name (num);
          RefPtr<Handler::Base> H_handler ((*format_handler)->create (header));
          assert (H_handler);
          merge (header);
          handler_->merge (*H_handler);
        }

        if (Pdim.size()) {
          int a = 0, n = 0;
          for (size_t i = 0; i < ndim(); ++i)
            if (stride (i))
              ++n;

          set_ndim (n + Pdim.size());

          for (size_t i = 0; i < Pdim.size(); ++i) {
            while (stride (a)) 
              ++a;
            dim(a) = Pdim[i];
            stride(a) = ++n;
          }

          name() = image_name;
        }

        handler_->set_name (name());
        handler_->set_image_is_new (true);
        handler_->set_readwrite (true);

        sanitise();
      }
      catch (Exception& E) {
        throw Exception (E, "error creating image \"" + image_name + "\"");
      }
    }












    std::string Header::description() const
    {
      std::string desc (
        "************************************************\n"
        "Image:               \"" + name() + "\"\n"
        "************************************************\n"
        "  Format:            " + (format() ? format() : "undefined") + "\n"
        "  Dimensions:        ");

      size_t i;
      for (i = 0; i < ndim(); i++) {
        if (i) desc += " x ";
        desc += str (dim (i));
      }

      desc += "\n  Voxel size:        ";
      for (i = 0; i < ndim(); i++) {
        if (i) desc += " x ";
        desc += std::isnan (vox (i)) ? "?" : str (vox (i));
      }

      desc += std::string ("\n  Data type:         ") + (datatype().description() ? datatype().description() : "invalid") + "\n"
              "  Data strides:      [ ";
      std::vector<ssize_t> strides (Image::Stride::get (*this));
      Image::Stride::symbolise (strides);
      for (i = 0; i < ndim(); i++)
        desc += stride (i) ? str (strides[i]) + " " : "? ";


      desc += "]\n"
              "  Intensity scaling: offset = " + str (intensity_offset()) + ", multiplier = " + str (intensity_scale()) + "\n"
              "  Comments:          " + (comments().size() ? comments() [0] : "(none)") + "\n";


      for (i = 1; i < comments().size(); i++)
        desc += "                     " + comments() [i] + "\n";

      if (size() > 0) {
        desc += "  Properties:\n";
        for (std::map<std::string, std::string>::const_iterator it = begin(); it != end(); ++it)
          desc += "    " + it->first + ": " + it->second + "\n";
      }

      if (transform().is_set()) {
        desc += "  Transform:         ";
        for (size_t i = 0; i < transform().rows(); i++) {
          if (i) desc +=  "                     ";
          for (size_t j = 0; j < transform().columns(); j++) {
            char buf[14], buf2[14];
            snprintf (buf, 14, "%.4g", transform() (i,j));
            snprintf (buf2, 14, "%12.10s", buf);
            desc += buf2;
          }
          desc += "\n";

        }
      }

      if (DW_scheme().is_set())
        desc += "  DW scheme:         " + str (DW_scheme().rows()) + " x " + str (DW_scheme().columns()) + "\n";

      return desc;
    }






  }
}
