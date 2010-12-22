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
#include "dataset/stride.h"
#include "dataset/transform.h"
#include "math/matrix.h"
#include "math/permutation.h"
#include "image/axis.h"
#include "image/name_parser.h"
#include "image/format/list.h"
#include "image/handler/default.h"

namespace MR
{
  namespace Image
  {

    namespace
    {
      class AxesWrapper
      {
        private:
          std::vector<Axis>& A;
        public:
          AxesWrapper (std::vector<Axis>& axes) : A (axes) { }

          size_t ndim () const {
            return A.size();
          }
          const ssize_t& stride (int axis) const {
            return A[axis].stride;
          }
          ssize_t& stride (int axis) {
            return A[axis].stride;
          }
      };

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




    void Header::sanitise ()
    {
      debug ("sanitising header...");

      if (ndim() < 3) {
        info ("image contains fewer than 3 dimensions - adding extra dimensions");
        set_ndim (3);
      }

      {
        AxesWrapper wrap (axes_);
        DataSet::Stride::sanitise (wrap);
        DataSet::Stride::symbolise (wrap);
      }


      if (!finite (vox (0)) || !finite (vox (1)) || !finite (vox (2))) {
        error ("invalid voxel sizes - resetting to sane defaults");
        set_vox (0, 1.0);
        set_vox (1, 1.0);
        set_vox (2, 1.0);
      }

      if (transform().is_set()) {
        if (transform().rows() != 4 || transform().columns() != 4) {
          transform_.clear();
          error ("transform matrix is not 4x4 - resetting to sane defaults");
        }
        else {
          for (size_t i = 0; i < 3; i++) {
            for (size_t j = 0; j < 4; j++) {
              if (!finite (transform_ (i,j))) {
                transform_.clear();
                error ("transform matrix contains invalid entries - resetting to sane defaults");
                break;
              }
            }
            if (!transform().is_set()) break;
          }
        }
      }

      if (!transform().is_set())
        DataSet::Transform::set_default (transform_, *this);

      transform_ (3,0) = transform_ (3,1) = transform_ (3,2) = 0.0;
      transform_ (3,3) = 1.0;

      Math::Permutation perm (3);
      Math::absmax (transform_.row (0).sub (0,3), perm[0]);
      Math::absmax (transform_.row (1).sub (0,3), perm[1]);
      Math::absmax (transform_.row (2).sub (0,3), perm[2]);

      disambiguate_permutation (perm);

      assert (perm[0] != perm[1] && perm[1] != perm[2] && perm[2] != perm[0]);

      bool flip [3];
      flip[perm[0]] = transform_ (0,perm[0]) < 0.0;
      flip[perm[1]] = transform_ (1,perm[1]) < 0.0;
      flip[perm[2]] = transform_ (2,perm[2]) < 0.0;

      if (perm[0] != 0 || perm[1] != 1 || perm[2] != 2 ||
          flip[0] || flip[1] || flip[2]) {
        // axes in transform need to be realigned to MRtrix coordinate system:
        Math::Matrix<float> M (transform_);

        Math::Vector<float> translation = M.column (3).sub (0,3);
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

        for (size_t i = 0; i < 3; ++i) {
          Math::Vector<float> row = M.row (i).sub (0,3);
          perm.apply (row);
          if (flip[i])
            axes_[i].stride = -axes_[i].stride;
        }

        transform_.swap (M);

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




    void Header::merge (const Header& H)
    {

      if (dtype_ != H.dtype_)
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

        if (vox (n) != H.vox (n))
          error ("WARNING: voxel dimensions differ between image files for \"" + name() + "\"");
      }

      for (std::vector<File::Entry>::const_iterator item = H.files_.begin(); item != H.files_.end(); ++item)
        files_.push_back (*item);

      for (std::map<std::string, std::string>::const_iterator item = H.begin(); item != H.end(); ++item)
        if (std::find (begin(), end(), *item) == end())
          insert (*item);

      for (std::vector<std::string>::const_iterator item = H.comments_.begin(); item != H.comments_.end(); ++item)
        if (std::find (comments_.begin(), comments_.end(), *item) == comments_.end())
          comments_.push_back (*item);

      if (!transform().is_set() && H.transform().is_set())
        transform_ = H.transform();

      if (!DW_scheme().is_set() && H.DW_scheme().is_set())
        DW_scheme_ = H.DW_scheme();
    }







    void Header::open (const std::string& image_name)
    {
      if (image_name.empty())
        throw Exception ("no name supplied to open image!");

      readwrite_ = false;

      try {
        info ("opening image \"" + image_name + "\"...");

        ParsedNameList list;
        std::vector<int> num = list.parse_scan_check (image_name);

        const Format::Base** format_handler = Format::handlers;
        std::vector< RefPtr<ParsedName> >::iterator item = list.begin();
        name_ = (*item)->name();

        for (; *format_handler; format_handler++)
          if ( (*format_handler)->read (*this))
            break;

        if (!*format_handler)
          throw Exception ("unknown format for image \"" + name() + "\"");

        format_ = (*format_handler)->description;

        while (++item != list.end()) {
          Header header (*this);
          header.name_ = (*item)->name();
          if (! (*format_handler)->read (header))
            throw Exception ("image specifier contains mixed format files");
          merge (header);
        }

        if (num.size()) {
          int a = 0, n = 0;
          for (size_t i = 0; i < ndim(); i++)
            if (stride (i))
              ++n;
          axes_.resize (n + num.size());

          for (std::vector<int>::const_iterator item = num.begin(); item != num.end(); ++item) {
            while (stride (a)) ++a;
            axes_[a].dim = *item;
            axes_[a].stride = ++n;
          }
        }

        sanitise();
        if (!handler_)
          handler_ = new Handler::Default (*this, false);

        name_ = image_name;
      }
      catch (Exception& E) {
        throw Exception (E, "error opening image \"" + image_name + "\"");
      }

    }





    void Header::create (const std::string& image_name)
    {
      if (image_name.empty())
        throw Exception ("no name supplied to open image!");

      readwrite_ = true;

      try {
        info ("creating image \"" + image_name + "\"...");

        sanitise();

        NameParser parser;
        parser.parse (image_name);
        std::vector<int> Pdim (parser.ndim());

        int Hdim [ndim()];
        for (size_t i = 0; i < ndim(); ++i)
          Hdim[i] = dim (i);

        name_ = image_name;

        const Format::Base** format_handler = Format::handlers;
        for (; *format_handler; format_handler++)
          if ( (*format_handler)->check (*this, ndim() - Pdim.size()))
            break;

        if (!*format_handler)
          throw Exception ("unknown format for image \"" + image_name + "\"");

        format_ = (*format_handler)->description;

        dtype_.set_byte_order_native();
        int a = 0;
        for (size_t n = 0; n < Pdim.size(); ++n) {
          while (stride (a)) a++;
          Pdim[n] = Hdim[a];
        }
        parser.calculate_padding (Pdim);

        Header header (*this);
        std::vector<int> num (Pdim.size());

        if (image_name != "-")
          name_ = parser.name (num);

        (*format_handler)->create (*this);

        while (get_next (num, Pdim)) {
          header.name_ = parser.name (num);
          (*format_handler)->create (header);
          merge (header);
        }

        if (Pdim.size()) {
          int a = 0, n = 0;
          for (size_t i = 0; i < ndim(); ++i)
            if (stride (i))
              ++n;

          set_ndim (n + Pdim.size());

          for (std::vector<int>::const_iterator item = Pdim.begin(); item != Pdim.end(); ++item) {
            while (stride (a)) ++a;
            set_dim (a, *item);
            set_stride (a, ++n);
          }
        }

        sanitise();
        if (!handler_)
          handler_ = new Handler::Default (*this, false);

        name_ = image_name;
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
        desc += isnan (vox (i)) ? "?" : str (vox (i));
      }




      desc += "\n  Dimension labels:  ";

      for (i = 0; i < ndim(); i++)
        desc += (i ? "                     " : "") + str (i) + ". "
                + (description (i).size() ? description (i) : "undefined") + " ("
                + (units (i).size() ? units (i) : "?") + ")\n";



      desc += std::string ("  Data type:         ") + (datatype().description() ? datatype().description() : "invalid") + "\n"
              "  Data strides:      [ ";


      std::vector<ssize_t> strides (DataSet::Stride::get (*this));
      DataSet::Stride::symbolise (strides);
      for (i = 0; i < ndim(); i++)
        desc += stride (i) ? str (strides[i]) + " " : "? ";



      desc += "]\n"
              "  Data scaling:      offset = " + str (data_offset()) + ", multiplier = " + str (data_scale()) + "\n"
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



    std::ostream& operator<< (std::ostream& stream, const Header& H)
    {
      stream << H.description();
      return stream;
    }

  }
}
