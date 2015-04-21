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
          std::shared_ptr<Handler::Base> H_handler;
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
          std::shared_ptr<Handler::Base> H_handler ((*format_handler)->create (header));
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

#include "image/info.h"
#include "image/transform.h"
#include "math/permutation.h"

namespace MR
{
  namespace Image
  {


    namespace
    {

      inline size_t not_any_of (size_t a, size_t b)
      {
        for (size_t i = 0; i < 3; ++i) {
          if (a == i || b == i)
            continue;
          return i;
        }
        assert (0);
        return UINT_MAX;
      }

      void disambiguate_permutation (Math::Permutation& permutation)
      {
        if (permutation[0] == permutation[1])
          permutation[1] = not_any_of (permutation[0], permutation[2]);

        if (permutation[0] == permutation[2])
          permutation[2] = not_any_of (permutation[0], permutation[1]);

        if (permutation[1] == permutation[2])
          permutation[2] = not_any_of (permutation[0], permutation[1]);
      }

    }



    void Info::sanitise_voxel_sizes ()
    {
      if (ndim() < 3) {
        INFO ("image contains fewer than 3 dimensions - adding extra dimensions");
        set_ndim (3);
      }

      if (!std::isfinite (vox (0)) || !std::isfinite (vox (1)) || !std::isfinite (vox (2))) {
        WARN ("invalid voxel sizes - resetting to sane defaults");
        float mean_vox_size = 0.0;
        size_t num_valid_vox = 0;
        for (size_t i = 0; i < 3; ++i) {
          if (std::isfinite(vox(i))) {
            ++num_valid_vox; 
            mean_vox_size += vox(i);
          }
        }
        mean_vox_size /= num_valid_vox;
        for (size_t i = 0; i < 3; ++i) 
          if (!std::isfinite(vox(i))) 
            vox(i) = mean_vox_size;
      }
    }


    void Info::sanitise_transform ()
    {
      if (transform().is_set()) {
        if (transform().rows() != 4 || transform().columns() != 4) {
          transform_.clear();
          WARN ("transform matrix is not 4x4 - resetting to sane defaults");
        }
        else {
          for (size_t i = 0; i < 3; i++) {
            for (size_t j = 0; j < 4; j++) {
              if (!std::isfinite (transform_ (i,j))) {
                transform_.clear();
                WARN ("transform matrix contains invalid entries - resetting to sane defaults");
                break;
              }
            }
            if (!transform().is_set()) break;
          }
        }
      }

      if (!transform().is_set())
        Transform::set_default (transform_, *this);

      transform_ (3,0) = transform_ (3,1) = transform_ (3,2) = 0.0;
      transform_ (3,3) = 1.0;
    }




    void Info::realign_transform ()
    {
      // find which row of the transform is closest to each scanner axis:
      Math::Permutation perm (3);
      Math::absmax (transform().row (0).sub (0,3), perm[0]);
      Math::absmax (transform().row (1).sub (0,3), perm[1]);
      Math::absmax (transform().row (2).sub (0,3), perm[2]);
      disambiguate_permutation (perm);
      assert (perm[0] != perm[1] && perm[1] != perm[2] && perm[2] != perm[0]);

      // figure out whether any of the rows of the transform point in the
      // opposite direction to the MRtrix convention:
      bool flip [3];
      flip[perm[0]] = transform_ (0,perm[0]) < 0.0;
      flip[perm[1]] = transform_ (1,perm[1]) < 0.0;
      flip[perm[2]] = transform_ (2,perm[2]) < 0.0;

      // check if image is already near-axial, return if true:
      if (perm[0] == 0 && perm[1] == 1 && perm[2] == 2 &&
          !flip[0] && !flip[1] && !flip[2])
        return;

      Math::Matrix<float> M (transform());
      Math::Vector<float> translation = M.column (3).sub (0,3);

      // modify translation vector:
      for (size_t i = 0; i < 3; ++i) {
        if (flip[i]) {
          const float length = (dim (i)-1) * vox (i);
          Math::Vector<float> axis = M.column (i);
          for (size_t n = 0; n < 3; ++n) {
            axis[n] = -axis[n];
            translation[n] -= length*axis[n];
          }
        }
      }

      // switch and/or invert rows if needed:
      for (size_t i = 0; i < 3; ++i) {
        Math::Vector<float> row = M.row (i).sub (0,3);
        perm.apply (row);
        if (flip[i])
          stride(i) = -stride(i);
      }

      // copy back transform:
      transform().swap (M);

      // switch axes to match:
      Axis a[] = {
        axes_[perm[0]],
        axes_[perm[1]],
        axes_[perm[2]]
      };
      axes_[0] = a[0];
      axes_[1] = a[1];
      axes_[2] = a[2];

    }


  }
}

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
#include "image/axis.h"

namespace MR
{
  namespace Image
  {


    std::vector<ssize_t> Axis::parse (size_t ndim, const std::string& specifier)
    {
      std::vector<ssize_t> parsed (ndim);

      size_t str = 0;
      size_t lim = 0;
      size_t end = specifier.size();
      size_t current = 0;

      try {
        while (str <= end) {
          bool pos = true;
          if (specifier[str] == '+') {
            pos = true;
            str++;
          }
          else if (specifier[str] == '-') {
            pos = false;
            str++;
          }
          else if (!isdigit (specifier[str])) throw 0;

          lim = str;

          while (isdigit (specifier[lim])) lim++;
          if (specifier[lim] != ',' && specifier[lim] != '\0') throw 0;
          parsed[current] = to<ssize_t> (specifier.substr (str, lim-str)) + 1;
          if (!pos) parsed[current] = -parsed[current];

          str = lim+1;
          current++;
        }
      }
      catch (int) {
        throw Exception ("malformed axes specification \"" + specifier + "\"");
      }

      if (current != ndim)
        throw Exception ("incorrect number of axes in axes specification \"" + specifier + "\"");

      check (parsed, ndim);

      return parsed;
    }







    void Axis::check (const std::vector<ssize_t>& parsed, size_t ndim)
    {
      if (parsed.size() != ndim)
        throw Exception ("incorrect number of dimensions for axes specifier");
      for (size_t n = 0; n < parsed.size(); n++) {
        if (!parsed[n] || size_t (std::abs (parsed[n])) > ndim)
          throw Exception ("axis ordering " + str (parsed[n]) + " out of range");

        for (size_t i = 0; i < n; i++)
          if (std::abs (parsed[i]) == std::abs (parsed[n]))
            throw Exception ("duplicate axis ordering (" + str (std::abs (parsed[n])) + ")");
      }
    }










    std::ostream& operator<< (std::ostream& stream, const Axis& axis)
    {
      stream << "[ dim: " << axis.dim << ", vox: " << axis.vox << ", stride: " << axis.stride << " ]";
      return stream;
    }




  }
}

