/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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

#include "header.h"

#include <cctype>
#include <set>

#include "axes.h"
#include "mrtrix.h"
#include "phase_encoding.h"
#include "stride.h"
#include "transform.h"
#include "image_io/default.h"
#include "image_io/scratch.h"
#include "file/name_parser.h"
#include "file/path.h"
#include "formats/list.h"

#include "dwi/gradient.h"

namespace MR
{

  const App::Option NoRealignOption
  = App::Option ("norealign",
                 "do not realign transform to near-default RAS coordinate system (the "
                 "default behaviour on image load). This is useful to inspect the image "
                 "and/or header contents as they are actually stored in the header, "
                 "rather than as MRtrix interprets them.");

  bool Header::do_not_realign_transform = false;



  void Header::check (const Header& H) const
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
  }



  void Header::merge (const Header& H, const bool volumes)
  {
    Eigen::MatrixXd dw_scheme, pe_scheme;
    if (volumes) {
      try {
        auto this_dw_scheme = DWI::parse_DW_scheme (*this);
        DWI::validate_DW_scheme (this_dw_scheme, *this, true);
        auto that_dw_scheme = DWI::parse_DW_scheme (H);
        DWI::validate_DW_scheme (that_dw_scheme, H, true);
        if (this_dw_scheme.cols() != that_dw_scheme.cols())
          throw Exception ("Number of columns in DW schemes of headers \"" + name() + "\" and \"" + H.name() + "\" do not match");
        dw_scheme.resize (this_dw_scheme.rows() + that_dw_scheme.rows(), that_dw_scheme.cols());
        if (this_dw_scheme.rows())
          dw_scheme.topRows (this_dw_scheme.rows()) = this_dw_scheme;
        dw_scheme.bottomRows (that_dw_scheme.rows()) = that_dw_scheme;
      } catch (Exception&) { }
      try {
        auto this_pe_scheme = PhaseEncoding::parse_scheme (*this);
        auto that_pe_scheme = PhaseEncoding::parse_scheme (H);
        if (!this_pe_scheme.rows() || !that_pe_scheme.rows())
          throw Exception ("PE scheme needs to be present in both headers");
        if (this_pe_scheme.cols() != that_pe_scheme.cols())
          throw Exception ("Number of columns in PE schemes of headers \"" + name() + "\" and \"" + H.name() + "\" do not match");
        pe_scheme.resize (this_pe_scheme.rows() + that_pe_scheme.rows(), that_pe_scheme.cols());
        pe_scheme.topRows (this_pe_scheme.rows()) = this_pe_scheme;
        pe_scheme.bottomRows (that_pe_scheme.rows()) = that_pe_scheme;
      } catch (Exception&) { }
    }

    std::map<std::string, std::string> new_keyval;
    std::set<std::string> unique_comments;
    for (const auto& item : keyval()) {
      if (item.first == "comments") {
        new_keyval.insert (item);
        const auto comments = split_lines (item.second);
        for (const auto& c : comments)
          unique_comments.insert (c);
      } else if (item.first != "command_history") {
        new_keyval.insert (item);
      }
    }
    for (const auto& item : H.keyval()) {
      if (item.first == "comments") {
        const auto comments = split_lines (item.second);
        for (const auto& c : comments) {
          if (unique_comments.find (c) == unique_comments.end()) {
            add_line (new_keyval["comments"], c);
            unique_comments.insert (c);
          }
        }
      } else if (item.first != "command_history") {
        auto it = keyval().find (item.first);
        if (it == keyval().end() || it->second == item.second)
          new_keyval.insert (item);
        else
          new_keyval[item.first] = "variable";
      }
    }
    std::swap (keyval_, new_keyval);

    if (volumes) {
      DWI::set_DW_scheme (*this, dw_scheme);
      PhaseEncoding::set_scheme (*this, pe_scheme);
    } else {
      const vector<std::string> reserved = { "dw_scheme", "pe_scheme", "PhaseEncodingDirection", "TotalReadoutTime" };
      for (const auto& key : reserved) {
        auto it = keyval_.find (key);
        if (it != keyval_.end() && it->second == "variable")
          keyval_.erase (it);
      }
    }
  }






  namespace {

    std::string short_description (const Header& H)
    {
      vector<std::string> dims;
      for (size_t n = 0; n < H.ndim(); ++n)
        dims.push_back (str(H.size(n)));
      vector<std::string> vox;
      for (size_t n = 0; n < H.ndim(); ++n)
        vox.push_back (str(H.spacing(n)));

      return " with dimensions " + join (dims, "x") + ", voxel spacing " + join (vox, "x") + ", datatype " + H.datatype().specifier();
    }
  }





  Header Header::open (const std::string& image_name)
  {
    if (image_name.empty())
      throw Exception ("no name supplied to open image!");

    Header H;

    try {
      INFO ("opening image \"" + image_name + "\"...");

      File::ParsedName::List list;
      vector<int> num = list.parse_scan_check (image_name);

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

      const bool merge_4d_schemes = (H.ndim() == 3 && num.size() == 1);

      while (++item < list.size()) {
        Header header (H);
        std::unique_ptr<ImageIO::Base> io_handler;
        header.name() = list[item].name();
        header.keyval().clear();
        if (!(io_handler = (*format_handler)->read (header)))
          throw Exception ("image specifier contains mixed format files");
        assert (io_handler);
        H.check (header);
        H.merge (header, merge_4d_schemes);
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

    INFO ("image \"" + H.name() + "\" opened" + short_description (H));

    return H;
  }


  namespace {
    inline bool check_strides_match (const vector<ssize_t>& a, const vector<ssize_t>& b)
    {
      size_t n = 0;
      for (; n < std::min (a.size(), b.size()); ++n)
        if (a[n] != b[n]) return false;
      for (size_t i = n; i < a.size(); ++i)
        if (a[i] > 1) return false;
      for (size_t i = n; i < b.size(); ++i)
        if (b[i] > 1) return false;
      return true;
    }

  }



  Header Header::create (const std::string& image_name, const Header& template_header, bool add_to_command_history)
  {
    if (image_name.empty())
      throw Exception ("no name supplied to open image!");

    Header H (template_header);
    const auto previous_datatype = H.datatype();

    try {
      INFO ("creating image \"" + image_name + "\"...");
      if (add_to_command_history) {

        auto argv_quoted = [] (const std::string& s) -> std::string {
          for (size_t i = 0; i != s.size(); ++i) {
            if (!(isalnum(s[i]) || s[i] == '.' || s[i] == '_' || s[i] == '-' || s[i] == '/')) {

              std::string escaped_string ("\'");
              for (auto c : s) {
                switch (c) {
                  case '\'': escaped_string.append ("\\\'"); break;
                  case '\\': escaped_string.append ("\\\\"); break;
                  default: escaped_string.push_back (c); break;
                }
              }
              escaped_string.push_back ('\'');
              return escaped_string;

            }
          }
          return s;
        };

        std::string cmd = App::argv[0];
        for (int n = 1; n < App::argc; ++n)
          cmd += std::string(" ") + argv_quoted (App::argv[n]);
        cmd += std::string ("  (version=") + App::mrtrix_version;
        if (App::project_version)
          cmd += std::string (", project=") + App::project_version;
        cmd += ")";
        add_line (H.keyval()["command_history"], cmd);

      }

      H.keyval()["mrtrix_version"] = App::mrtrix_version;
      if (App::project_version)
        H.keyval()["project_version"] = App::project_version;

      H.sanitise();

      File::NameParser parser;
      parser.parse (image_name);
      vector<int> Pdim (parser.ndim());

      vector<int> Hdim (H.ndim());
      for (size_t i = 0; i < H.ndim(); ++i)
        Hdim[i] = H.size(i);

      H.name() = image_name;

      const vector<ssize_t> strides (Stride::get_symbolic (H));
      const Formats::Base** format_handler = Formats::handlers;
      for (; *format_handler; format_handler++)
        if ((*format_handler)->check (H, H.ndim() - Pdim.size()))
          break;

      if (!*format_handler) {
        const std::string basename = Path::basename (image_name);
        const size_t extension_index = basename.find_last_of (".");
        if (extension_index == std::string::npos)
          throw Exception ("unknown format for image \"" + image_name + "\" (no file extension specified)");
        else
          throw Exception ("unknown format for image \"" + image_name + "\" (unsupported file extension: " + basename.substr (extension_index) + ")");
      }

      const vector<ssize_t> strides_aftercheck (Stride::get_symbolic (H));
      if (!check_strides_match (strides, strides_aftercheck)) {
        INFO("output strides for image " + image_name + " modified to " + str(strides_aftercheck) +
            " - requested strides " + str(strides) + " are not supported in " + (*format_handler)->description + " format");
      }

      H.datatype().set_byte_order_native();
      int a = 0;
      for (size_t n = 0; n < Pdim.size(); ++n) {
        while (a < int(H.ndim()) && H.stride(a))
          a++;
        Pdim[n] = Hdim[a];
      }
      parser.calculate_padding (Pdim);


      const bool split_4d_schemes = (parser.ndim() == 1 && template_header.ndim() == 4);
      auto dw_scheme = DWI::get_DW_scheme (template_header);
      auto pe_scheme = PhaseEncoding::get_scheme (template_header);
      if (split_4d_schemes) {
        try {
          DWI::check_DW_scheme (template_header, dw_scheme);
          DWI::stash_DW_scheme (H, dw_scheme);
          DWI::set_DW_scheme (H, dw_scheme.row (0));
        } catch (Exception&) {
          dw_scheme.resize (0, 0);
          DWI::clear_DW_scheme (H);
        }
        try {
          PhaseEncoding::check (template_header, pe_scheme);
          PhaseEncoding::set_scheme (H, pe_scheme.row (0));
        } catch (Exception&) {
          pe_scheme.resize (0, 0);
          PhaseEncoding::clear_scheme (H);
        }
      }


      Header header (H);
      vector<int> num (Pdim.size());

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


      size_t counter = 0;
      while (get_next (num, Pdim)) {
        header.name() = parser.name (num);
        ++counter;
        if (split_4d_schemes) {
          if (dw_scheme.rows())
            DWI::set_DW_scheme (header, dw_scheme.row (counter));
          if (pe_scheme.rows())
            PhaseEncoding::set_scheme (header, pe_scheme.row (counter));
        }
        std::shared_ptr<ImageIO::Base> io_handler ((*format_handler)->create (header));
        assert (io_handler);
        H.merge (header, split_4d_schemes);
        H.io->merge (*io_handler);
      }

      if (Pdim.size()) {
        int a = 0, n = 0;
        ssize_t next_stride = 0;
        for (size_t i = 0; i < H.ndim(); ++i) {
          if (H.stride(i)) {
            ++n;
            next_stride = std::max (next_stride, abs (H.stride(i)));
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

    DataType new_datatype = H.datatype();
    if (new_datatype != previous_datatype) {
      new_datatype.unset_flag (DataType::BigEndian);
      new_datatype.unset_flag (DataType::LittleEndian);
      if (new_datatype != previous_datatype)
        WARN (std::string ("requested datatype (") + previous_datatype.specifier() + ") not supported - substituting with " + H.datatype().specifier());
    }

    INFO ("image \"" + H.name() + "\" created" + short_description (H));

    return H;
  }






  Header Header::scratch (const Header& template_header, const std::string& label)
  {
    Header H (template_header);
    H.name() = label;
    H.reset_intensity_scaling();
    H.sanitise();
    H.format_ = "scratch image";
    H.io = make_unique<ImageIO::Scratch> (H);
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





  std::string Header::description (bool print_all) const
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
      if (entries.size()) {
        bool shorten = (!print_all && entries.size() > 5);
        desc += key + entries[0] + "\n";
        if (entries.size() > 5) {
          key = "  [" + str(entries.size()) + " entries] ";
          if (key.size() < 21)
            key.resize (21, ' ');
        }
        else key = "                     ";
        for (size_t n = 1; n < (shorten ? size_t(2) : entries.size()); ++n) {
          desc += key + entries[n] + "\n";
          key = "                     ";
        }
        if (!print_all && entries.size() > 5) {
          desc += key + "...\n";
          for (size_t n = entries.size()-2; n < entries.size(); ++n )
            desc += key + entries[n] + "\n";
        }
      } else {
        desc += key + "(empty)\n";
      }
    }

    return desc;
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
      mean_vox_size = num_valid_vox ? mean_vox_size / num_valid_vox : 1.0;
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
    bool flip[3];
    Axes::get_permutation_to_make_axial (transform(), perm, flip);

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

    INFO ("Axes and transform of image \"" + name() + "\" altered to approximate RAS coordinate system");

    // If there's any phase encoding direction information present in the
    //   header, it's necessary here to update it according to the
    //   flips / permutations that have taken place
    auto pe_scheme = PhaseEncoding::get_scheme (*this);
    if (pe_scheme.rows()) {
      for (ssize_t row = 0; row != pe_scheme.rows(); ++row) {
        Eigen::VectorXd new_line (pe_scheme.row (row));
        for (ssize_t axis = 0; axis != 3; ++axis) {
          new_line[axis] = pe_scheme(row, perm[axis]);
          if (new_line[axis] && flip[axis])
            new_line[axis] = -new_line[axis];
        }
        pe_scheme.row (row) = new_line;
      }
      PhaseEncoding::set_scheme (*this, pe_scheme);
      INFO ("Phase encoding scheme has been modified according to internal header transform realignment");
    }

    // If there's any slice encoding direction information present in the
    //   header, that's also necessary to update here
    auto slice_encoding_it = keyval().find ("SliceEncodingDirection");
    if (slice_encoding_it != keyval().end()) {
      const Eigen::Vector3 orig_dir (Axes::id2dir (slice_encoding_it->second));
      Eigen::Vector3 new_dir;
      for (size_t axis = 0; axis != 3; ++axis)
        new_dir[axis] = orig_dir[perm[axis]] * (flip[axis] ? -1.0 : 1.0);
      slice_encoding_it->second = Axes::dir2id (new_dir);
      INFO ("Slice encoding direction has been modified according to internal header transform realignment");
    }

  }



}
