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

#include "app.h"
#include "header.h"
#include "stride.h"
#include "transform.h"
#include "image_io/default.h"
#include "image_io/scratch.h"
#include "file/name_parser.h"
#include "formats/list.h"

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

      if (std::isfinite(spacing(n)) && std::isfinite(H.spacing(n)) && spacing(n) != H.spacing(n))
        WARN ("voxel dimensions differ between image files for \"" + name() + "\"");
    }

    if ((transform().matrix() - H.transform().matrix()).cwiseAbs().maxCoeff() > 1.0e-6)
      WARN ("transform matrices differ between image files for \"" + name() + "\"");;

    if (datatype() != H.datatype())
      throw Exception ("data types differ between image files for \"" + name() + "\"");

    if (intensity_offset() != H.intensity_offset() || intensity_scale() != H.intensity_scale())
      throw Exception ("scaling coefficients differ between image files for \"" + name() + "\"");

    for (const auto& item : H.keyval())
      if (keyval().find (item.first) == keyval().end())
        keyval().insert (item);
  }







  Header Header::open (const std::string& image_name)
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

      H.format_ = (*format_handler)->description;

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
        H.axes_.resize (n + num.size());

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





  Header Header::create (const std::string& image_name, const Header& template_header)
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
      const std::vector<ssize_t> strides (Stride::get_symbolic (H));
      for (; *format_handler; format_handler++)
        if ((*format_handler)->check (H, H.ndim() - Pdim.size()))
          break;
      const std::vector<ssize_t> strides_aftercheck (Stride::get_symbolic (H));
      if (! std::equal(strides.begin(), strides.end(), strides_aftercheck.begin())) {
        WARN ("output strides "+str(strides_aftercheck)+" are different from specified strides "+str(strides));
      }
      if (!*format_handler) {
        const std::string basename = Path::basename (image_name);
        const size_t extension_index = basename.find_last_of (".");
        if (extension_index == std::string::npos)
          throw Exception ("unknown format for image \"" + image_name + "\" (no file extension specified)");
        else
          throw Exception ("unknown format for image \"" + image_name + "\" (unsupported file extension: " + basename.substr (extension_index) + ")");
      }

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
      H.format_ = (*format_handler)->description;


      auto get_next = [](decltype(num)& pos, const decltype(Pdim)& limits) {
        size_t axis = 0;
        while (axis < limits.size()) {
          pos[axis]++;
          if (pos[axis] < limits[axis]) 
            return true;
          pos[axis] = 0;
          axis++;
        }
        return false;
      };

      while (get_next (num, Pdim)) {
        header.name() = parser.name (num);
        std::shared_ptr<ImageIO::Base> io_handler ((*format_handler)->create (header));
        assert (io_handler);
        H.merge (header);
        H.io->merge (*io_handler);
      }

      if (Pdim.size()) {
        int a = 0, n = 0;
        ssize_t next_stride = 0;
        for (size_t i = 0; i < H.ndim(); ++i) {
          if (H.stride(i)) {
            ++n;
            next_stride = std::max (next_stride, std::abs (H.stride(i)));
          }
        }

        H.axes_.resize (n + Pdim.size());

        for (size_t i = 0; i < Pdim.size(); ++i) {
          while (H.stride(a)) 
            ++a;
          H.size(a) = Pdim[i];
          H.stride(a) = ++next_stride;
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






  Header Header::scratch (const Header& template_header, const std::string& label) 
  {
    Header H (template_header);
    H.name() = label;
    H.reset_intensity_scaling();
    H.sanitise();
    H.format_ = "scratch image";
    H.io = std::unique_ptr<ImageIO::Scratch> (new ImageIO::Scratch (H)); 
    return H;
  }








  std::ostream& operator<< (std::ostream& stream, const Header& H) 
  {
    stream << "\"" << H.name() << "\", " << H.datatype().specifier() << ", size [ ";
    for (size_t n = 0; n < H.ndim(); ++n) stream << H.size(n) << " ";
    stream << "], voxel size [ ";
    for (size_t n = 0; n < H.ndim(); ++n) stream << H.spacing(n) << " "; 
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
      desc += std::isnan (spacing(i)) ? "?" : str (spacing(i), 6);
    }
    desc += "\n";

    desc += "  Data strides:      [ ";
    auto strides (Stride::get (*this));
    Stride::symbolise (strides);
    for (i = 0; i < ndim(); i++)
      desc += stride(i) ? str (strides[i]) + " " : "? ";
    desc += "]\n";

    if (io) {
      desc += std::string("  Format:            ") + (format() ? format() : "undefined") + "\n";
      desc += std::string ("  Data type:         ") + ( datatype().description() ? datatype().description() : "invalid" ) + "\n";
      desc += "  Intensity scaling: offset = " + str (intensity_offset()) + ", multiplier = " + str (intensity_scale()) + "\n";
    }


    desc += "  Transform:         ";
    for (size_t i = 0; i < 3; i++) {
      if (i) desc +=  "                     ";
      for (size_t j = 0; j < 4; j++) {
        char buf[14], buf2[14];
        snprintf (buf, 14, "%.4g", transform() (i,j));
        snprintf (buf2, 14, "%12.10s", buf);
        desc += buf2;
      }
      desc += "\n";
    }

    for (const auto& p : keyval()) {
      std::string key = "  " + p.first + ": ";
      if (key.size() < 21) 
        key.resize (21, ' ');
      const auto entries = split_lines (p.second);
      if (entries.size() > 5) 
        desc += key + "[ " + str (entries.size()) + " entries ]\n";
      else {
        for (const auto value : entries) {
          desc += key + value + "\n";
          key = "                     ";
        }
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

    void disambiguate_permutation (size_t permutation[3])
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
      axes_.resize (3);
    }

    if (!std::isfinite (spacing(0)) || !std::isfinite (spacing(1)) || !std::isfinite (spacing(2))) {
      WARN ("invalid voxel sizes - resetting to sane defaults");
      default_type mean_vox_size = 0.0;
      size_t num_valid_vox = 0;
      for (size_t i = 0; i < 3; ++i) {
        if (std::isfinite(spacing(i))) {
          ++num_valid_vox; 
          mean_vox_size += spacing(i);
        }
      }
      mean_vox_size /= num_valid_vox;
      for (size_t i = 0; i < 3; ++i) 
        if (!std::isfinite(spacing(i))) 
          spacing(i) = mean_vox_size;
    }
  }







  void Header::sanitise_transform ()
  {
    if (!transform().matrix().allFinite()) {
      WARN ("transform matrix contains invalid entries - resetting to sane defaults");
      transform() = Transform::get_default (*this);
    }
  }




  void Header::realign_transform ()
  {
    // find which row of the transform is closest to each scanner axis:
    size_t perm [3];
    decltype(transform().matrix().topLeftCorner<3,3>())::Index index;
    transform().matrix().topLeftCorner<3,3>().row(0).cwiseAbs().maxCoeff (&index); perm[0] = index;
    transform().matrix().topLeftCorner<3,3>().row(1).cwiseAbs().maxCoeff (&index); perm[1] = index;
    transform().matrix().topLeftCorner<3,3>().row(2).cwiseAbs().maxCoeff (&index); perm[2] = index;
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

    auto M (transform());
    auto translation = M.translation();

    // modify translation vector:
    for (size_t i = 0; i < 3; ++i) {
      if (flip[i]) {
        const default_type length = (size(i)-1) * spacing(i);
        auto axis = M.matrix().col (i);
        for (size_t n = 0; n < 3; ++n) {
          axis[n] = -axis[n];
          translation[n] -= length*axis[n];
        }
      }
    }

    // switch and/or invert rows if needed:
    for (size_t i = 0; i < 3; ++i) {
      auto row = M.matrix().row(i).head<3>();
      row = Eigen::RowVector3d (row[perm[0]], row[perm[1]], row[perm[2]]);

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
