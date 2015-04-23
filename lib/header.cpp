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
#include "header.h"
#include "stride.h"
#include "transform.h"
#include "image_io/default.h"
#include "file/name_parser.h"
#include "formats/list.h"
#include "math/permutation.h"

namespace MR
{

  bool Header::do_not_realign_transform = false;



  void Header::merge (const Header& H)
  {
    if (ndim() != H.ndim())
      throw Exception ("dimension mismatch between image files for \"" + name() + "\"");

    for (size_t n = 0; n < ndim(); ++n) {
      if (size(n) != H.size(n))
        throw Exception ("dimension mismatch between image files for \"" + name() + "\"");

      if (stride(n) != H.stride(n))
        throw Exception ("data strides differs image files for \"" + name() + "\"");

      if (std::isfinite(voxsize(n)) && std::isfinite(H.voxsize(n)) && voxsize(n) != H.voxsize(n))
        WARN ("voxel dimensions differ between image files for \"" + name() + "\"");
    }

    if (!transform().is_set() && H.transform().is_set())
      transform() = H.transform();

    if (datatype() != H.datatype())
      throw Exception ("data types differ between image files for \"" + name() + "\"");

    if (intensity_offset() != H.intensity_offset() || intensity_scale() != H.intensity_scale())
      throw Exception ("scaling coefficients differ between image files for \"" + name() + "\"");

    for (const auto& item : H.keyval())
      if (keyval().find (item.first) == keyval().end())
        keyval().insert (item);
  }







  const Header Header::open (const std::string& image_name)
  {
    if (image_name.empty())
      throw Exception ("no name supplied to open image!");

    Header H;

    try {
      INFO ("opening image \"" + image_name + "\"...");

      File::ParsedName::List list;
      std::vector<int> num = list.parse_scan_check (image_name);

      const Formats::Base** format_handler = Formats::handlers;
      size_t item = 0;
      H.name() = list[item].name();

      for (; *format_handler; format_handler++) {
        if ( (H.io = (*format_handler)->read (H)) )
          break;
      }

      if (!*format_handler)
        throw Exception ("unknown format for image \"" + H.name() + "\"");
      assert (H.io);

      H.io->format = (*format_handler)->description;

      while (++item < list.size()) {
        Header header (H);
        std::unique_ptr<ImageIO::Base> io_handler;
        header.name() = list[item].name();
        if (!(io_handler = (*format_handler)->read (header)))
          throw Exception ("image specifier contains mixed format files");
        assert (io_handler);
        H.merge (header);
        H.io->merge (*io_handler);
      }

      if (num.size()) {
        int a = 0, n = 0;
        for (size_t i = 0; i < H.ndim(); i++)
          if (H.stride(i))
            ++n;
        H.set_ndim (n + num.size());

        for (size_t i = 0; i < num.size(); ++i) {
          while (H.stride(a)) ++a;
          H.size(a) = num[num.size()-1-i];
          H.stride(a) = ++n;
        }
        H.name() = image_name;
      }

      H.sanitise();
      if (!do_not_realign_transform) 
        H.realign_transform();
    }
    catch (Exception& E) {
      throw Exception (E, "error opening image \"" + image_name + "\"");
    }

    return H;
  }





  const Header Header::create (const std::string& image_name, const Header& template_header)
  {
    if (image_name.empty())
      throw Exception ("no name supplied to open image!");

    Header H (template_header);

    try {
      INFO ("creating image \"" + image_name + "\"...");

      H.keyval()["mrtrix_version"] = App::mrtrix_version;
      if (App::project_version)
        H.keyval()["project_version"] = App::project_version;

      H.sanitise();

      File::NameParser parser;
      parser.parse (image_name);
      std::vector<int> Pdim (parser.ndim());

      std::vector<int> Hdim (H.ndim());
      for (size_t i = 0; i < H.ndim(); ++i)
        Hdim[i] = H.size(i);

      H.name() = image_name;

      const Formats::Base** format_handler = Formats::handlers;
      for (; *format_handler; format_handler++)
        if ((*format_handler)->check (H, H.ndim() - Pdim.size()))
          break;

      if (!*format_handler)
        throw Exception ("unknown format for image \"" + image_name + "\"");

      H.datatype().set_byte_order_native();
      int a = 0;
      for (size_t n = 0; n < Pdim.size(); ++n) {
        while (a < int(H.ndim()) && H.stride(a))
          a++;
        Pdim[n] = Hdim[a];
      }
      parser.calculate_padding (Pdim);

      Header header (H);
      std::vector<int> num (Pdim.size());

      if (image_name != "-")
        H.name() = parser.name (num);

      H.io = (*format_handler)->create (H);
      assert (H.io);
      H.io->format = (*format_handler)->description;

      while (get_next (num, Pdim)) {
        header.name() = parser.name (num);
        std::shared_ptr<ImageIO::Base> io_handler ((*format_handler)->create (header));
        assert (io_handler);
        H.merge (header);
        H.io->merge (*io_handler);
      }

      if (Pdim.size()) {
        int a = 0, n = 0;
        for (size_t i = 0; i < H.ndim(); ++i)
          if (H.stride(i))
            ++n;

        H.set_ndim (n + Pdim.size());

        for (size_t i = 0; i < Pdim.size(); ++i) {
          while (H.stride(a)) 
            ++a;
          H.size(a) = Pdim[i];
          H.stride(a) = ++n;
        }

        H.name() = image_name;
      }

      H.io->set_image_is_new (true);
      H.io->set_readwrite (true);

      H.sanitise();
    }
    catch (Exception& E) {
      throw Exception (E, "error creating image \"" + image_name + "\"");
    }

    return H;
  }






  std::ostream& operator<< (std::ostream& stream, const Header& H) 
  {
    stream << "\"" << H.name() << "\", " << H.datatype().specifier() << ", size [ ";
    for (size_t n = 0; n < H.ndim(); ++n) stream << H.size(n) << " ";
    stream << "], voxel size [ ";
    for (size_t n = 0; n < H.ndim(); ++n) stream << H.voxsize(n) << " "; 
    stream << "], strides [ ";
    for (size_t n = 0; n < H.ndim(); ++n) stream << H.stride(n) << " "; 
    stream << "]";
    return stream;
  }





  std::string Header::description() const
  {
    std::string desc (
        "************************************************\n"
        "Image:               \"" + name() + "\"\n"
        "************************************************\n");

    desc += "  Dimensions:        ";
    size_t i;
    for (i = 0; i < ndim(); i++) {
      if (i) desc += " x ";
      desc += str (size(i));
    }

    desc += "\n  Voxel size:        ";
    for (i = 0; i < ndim(); i++) {
      if (i) desc += " x ";
      desc += std::isnan (voxsize(i)) ? "?" : str (voxsize(i));
    }
    desc += "\n";

    desc += "  Data strides:      [ ";
    auto strides (Stride::get (*this));
    Stride::symbolise (strides);
    for (i = 0; i < ndim(); i++)
      desc += stride(i) ? str (strides[i]) + " " : "? ";
    desc += "]\n";

    if (io) {
      desc += std::string("  Format:            ") + (io->format ? io->format : "undefined") + "\n";
      desc += std::string ("  Data type:         ") + ( datatype().description() ? datatype().description() : "invalid" ) + "\n";
      desc += "  Intensity scaling: offset = " + str (intensity_offset()) + ", multiplier = " + str (intensity_scale()) + "\n";
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

    for (const auto& p : keyval()) {
      std::string key = "  " + p.first + ": ";
      if (key.size() < 20) 
        key.resize (20, ' ');
      desc += key;
      for (const auto value : split (p.second)) {
        desc += key + value + "\n";
        desc = "                     ";
      }
    }

    return desc;
  }




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
      return std::numeric_limits<size_t>::max();
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



  void Header::sanitise_voxel_sizes ()
  {
    if (ndim() < 3) {
      INFO ("image contains fewer than 3 dimensions - adding extra dimensions");
      set_ndim (3);
    }

    if (!std::isfinite (voxsize(0)) || !std::isfinite (voxsize(1)) || !std::isfinite (voxsize(2))) {
      WARN ("invalid voxel sizes - resetting to sane defaults");
      default_type mean_vox_size = 0.0;
      size_t num_valid_vox = 0;
      for (size_t i = 0; i < 3; ++i) {
        if (std::isfinite(voxsize(i))) {
          ++num_valid_vox; 
          mean_vox_size += voxsize(i);
        }
      }
      mean_vox_size /= num_valid_vox;
      for (size_t i = 0; i < 3; ++i) 
        if (!std::isfinite(voxsize(i))) 
          voxsize(i) = mean_vox_size;
    }
  }







  void Header::sanitise_transform ()
  {
    if (transform().is_set()) {
      if (transform().rows() != 4 || transform().columns() != 4) {
        transform().clear();
        WARN ("transform matrix is not 4x4 - resetting to sane defaults");
      }
      else {
        for (size_t i = 0; i < 3; i++) {
          for (size_t j = 0; j < 4; j++) {
            if (!std::isfinite (transform() (i,j))) {
              transform().clear();
              WARN ("transform matrix contains invalid entries - resetting to sane defaults");
              break;
            }
          }
          if (!transform().is_set()) break;
        }
      }
    }

    if (!transform().is_set())
      Transform::set_default (transform(), *this);

    transform() (3,0) = transform() (3,1) = transform() (3,2) = 0.0;
    transform() (3,3) = 1.0;
  }




  void Header::realign_transform ()
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
    flip[perm[0]] = transform() (0,perm[0]) < 0.0;
    flip[perm[1]] = transform() (1,perm[1]) < 0.0;
    flip[perm[2]] = transform() (2,perm[2]) < 0.0;

    // check if image is already near-axial, return if true:
    if (perm[0] == 0 && perm[1] == 1 && perm[2] == 2 &&
        !flip[0] && !flip[1] && !flip[2])
      return;

    Math::Matrix<default_type> M (transform());
    Math::Vector<default_type> translation = M.column (3).sub (0,3);

    // modify translation vector:
    for (size_t i = 0; i < 3; ++i) {
      if (flip[i]) {
        const default_type length = (size(i)-1) * voxsize(i);
        Math::Vector<default_type> axis = M.column (i);
        for (size_t n = 0; n < 3; ++n) {
          axis[n] = -axis[n];
          translation[n] -= length*axis[n];
        }
      }
    }

    // switch and/or invert rows if needed:
    for (size_t i = 0; i < 3; ++i) {
      Math::Vector<default_type> row = M.row (i).sub (0,3);
      perm.apply (row);
      if (flip[i])
        stride(i) = -stride(i);
    }

    // copy back transform:
    transform() = std::move (M);

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
