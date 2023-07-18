/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#include "app.h"
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


  bool Header::do_realign_transform = true;



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



  namespace {
    std::string resolve_slice_timing (const std::string& one, const std::string& two)
    {
      if (one == "variable" || two == "variable")
        return "variable";
      vector<std::string> one_split = split (one, ",");
      vector<std::string> two_split = split (two, ",");
      if (one_split.size() != two_split.size()) {
        DEBUG ("Slice timing vectors of inequal length");
        return "invalid";
      }
      // Siemens CSA reports with 2.5ms precision = 0.0025s
      // Allow slice times to vary by 1.5x this amount, but no more
      for (size_t i = 0; i != one_split.size(); ++i) {
        default_type f_one, f_two;
        try {
          f_one = to<default_type> (one_split[i]);
          f_two = to<default_type> (two_split[i]);
        } catch (Exception& e) {
          DEBUG ("Error converting slice timing vector to floating-point");
          return "invalid";
        }
        const default_type diff = abs (f_two - f_one);
        if (diff > 0.00375) {
          DEBUG ("Supra-threshold difference of " + str(diff) + "s in slice times");
          return "variable";
        }
      }
      return one;
    }
  }



  void Header::merge_keyval (const Header& H)
  {
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
      } else {
        auto it = keyval().find (item.first);
        if (it == keyval().end() || it->second == item.second)
          new_keyval.insert (item);
        else if (item.first == "SliceTiming")
          new_keyval["SliceTiming"] = resolve_slice_timing (item.second, it->second);
        else
          new_keyval[item.first] = "variable";
      }
    }
    std::swap (keyval_, new_keyval);
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
      const auto num = list.parse_scan_check (image_name);

      const Formats::Base** format_handler = Formats::handlers;
      size_t item_index = 0;
      H.name() = list[item_index].name();

      for (; *format_handler; format_handler++) {
        if ( (H.io = (*format_handler)->read (H)) )
          break;
      }

      if (!*format_handler)
        throw Exception ("unknown format for image \"" + H.name() + "\"");
      assert (H.io);

      H.format_ = (*format_handler)->description;

      if (num.size()) {

        const Header template_header (H);

        // Convenient to know a priori which loop index corresponds to which image axis
        // This needs to detect unity-sized axes and flag the loop to concatenate data along that axis
        vector<size_t> loopindex2axis;
        size_t axis;
        for (axis = 0; axis != H.ndim(); ++axis) {
          if (H.size (axis) == 1) {
            loopindex2axis.push_back (axis);
            if (loopindex2axis.size() == num.size())
              break;
          }
        }
        for (; loopindex2axis.size() < num.size(); ++axis)
          loopindex2axis.push_back (axis);
        std::reverse (loopindex2axis.begin(), loopindex2axis.end());

        // Reimplemented support for [] notation using recursive function calls
        // Note that the very first image header has already been opened before this function is
        //   invoked for the first time; "vector<Header>& this_data" is used to propagate this
        //   data to deeper layers when necessary
        std::function<void(Header&, vector<Header>&, const size_t)>
        import = [&] (Header& result, vector<Header>& this_data, const size_t loop_index) -> void
        {
          if (loop_index == num.size()-1) {
            vector<std::unique_ptr<ImageIO::Base>> ios;
            if (this_data.size())
              ios.push_back (std::move (this_data[0].io));
            for (size_t i = this_data.size(); i != size_t(num[loop_index]); ++i) {
              Header header (template_header);
              std::unique_ptr<ImageIO::Base> io_handler;
              header.name() = list[++item_index].name();
              header.keyval().clear();
              if (!(io_handler = (*format_handler)->read (header)))
                throw Exception ("image specifier contains mixed format files");
              assert (io_handler);
              template_header.check (header);
              this_data.push_back (std::move (header));
              ios.push_back (std::move (io_handler));
            }
            result = concatenate (this_data, loopindex2axis[loop_index], false);
            result.io = std::move (ios[0]);
            for (size_t i = 1; i != ios.size(); ++i)
              result.io->merge (*ios[i]);
            return;
          } // End branch for when loop_index is the maximum, ie. innermost loop
          // For each coordinate along this axis, need to concatenate headers from the
          //   next lower axis
          vector<Header> nested_data;
          // The nested concatenation may still include the very first header that has already been read;
          //   this needs to be propagated through to the nested call
          if (this_data.size()) {
            assert (this_data.size() == 1);
            nested_data.push_back (std::move (this_data[0]));
            this_data.clear();
          }
          for (size_t i = 0; i != size_t(num[loop_index]); ++i) {
            Header temp;
            import (temp, nested_data, loop_index+1);
            this_data.push_back (std::move (temp));
            nested_data.clear();
          }
          result = concatenate (this_data, loopindex2axis[loop_index], false);
          result.io = std::move (this_data[0].io);
          for (size_t i = 1; i != size_t(num[loop_index]); ++i)
            result.io->merge (*this_data[i].io);
        };

        vector<Header> headers;
        headers.push_back (std::move (H));
        import (H, headers, 0);
        H.name() = image_name;
      } // End branching for [] notation

      H.sanitise();
      if (do_realign_transform)
        H.realign_transform();
    }
    catch (CancelException& e) { throw; }
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
        // Make sure the current command is not concatenated more than once
        const auto command_history = split_lines (H.keyval()["command_history"]);
        if (!(command_history.size() && command_history.back() == App::command_history_string))
          add_line (H.keyval()["command_history"], App::command_history_string);
      }

      H.keyval()["mrtrix_version"] = App::mrtrix_version;
      if (App::project_version)
        H.keyval()["project_version"] = App::project_version;

      H.sanitise();

      File::NameParser parser;
      parser.parse (image_name);
      vector<uint32_t> Pdim (parser.ndim());

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
        Pdim[n] = Hdim[a++];
      }
      parser.calculate_padding (Pdim);


      const bool split_4d_schemes = (parser.ndim() == 1 && template_header.ndim() == 4);
      Eigen::MatrixXd dw_scheme, pe_scheme;
      try {
        dw_scheme = DWI::parse_DW_scheme (template_header);
      } catch (Exception&) {
        DWI::clear_DW_scheme (H);
      }
      try {
        pe_scheme = PhaseEncoding::parse_scheme (template_header);
      } catch (Exception&) {
        PhaseEncoding::clear_scheme (H);
      }
      if (split_4d_schemes) {
        try {
          DWI::check_DW_scheme (template_header, dw_scheme);
          DWI::set_DW_scheme (H, dw_scheme.row (0));
        } catch (Exception&) {
          dw_scheme.resize (0, 0);
          DWI::clear_DW_scheme (H);
        }
        try {
          PhaseEncoding::check (pe_scheme, template_header);
          PhaseEncoding::set_scheme (H, pe_scheme.row (0));
        } catch (Exception&) {
          pe_scheme.resize (0, 0);
          PhaseEncoding::clear_scheme (H);
        }
      }


      Header header (H);
      vector<uint32_t> num (Pdim.size());

      if (!is_dash (image_name))
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

      if (split_4d_schemes) {
        DWI::set_DW_scheme (H, dw_scheme);
        PhaseEncoding::set_scheme (H, pe_scheme);
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
        "Image name:          \"" + name() + "\"\n"
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

    // check that cosine vectors are unit length (to some precision):
    bool rescale_cosine_vectors = false;
    for (size_t i = 0; i < 3; ++i) {
      auto length = transform().matrix().col(i).head<3>().norm();
      if (abs (length-1.0) > 1.0e-6)
        rescale_cosine_vectors = true;
    }

    // if unit length, rescale and modify voxel sizes accordingly:
    if (rescale_cosine_vectors) {
      INFO ("non unit cosine vectors detected - normalising and rescaling voxel sizes to match");
      for (size_t i = 0; i < 3; ++i) {
        auto length = transform().matrix().col(i).head(3).norm();
        transform().matrix().col(i).head(3) /= length;
        spacing(i) *= length;
      }
    }
  }




  void Header::realign_transform ()
  {
    // find which row of the transform is closest to each scanner axis:
    Axes::get_permutation_to_make_axial (transform(), realign_perm_, realign_flip_);

    // check if image is already near-axial, return if true:
    if (realign_perm_[0] == 0 && realign_perm_[1] == 1 && realign_perm_[2] == 2 &&
        !realign_flip_[0] && !realign_flip_[1] && !realign_flip_[2])
      return;

    auto M (transform());
    auto translation = M.translation();

    // modify translation vector:
    for (size_t i = 0; i < 3; ++i) {
      if (realign_flip_[i]) {
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
      row = Eigen::RowVector3d (row[realign_perm_[0]], row[realign_perm_[1]], row[realign_perm_[2]]);

      if (realign_flip_[i])
        stride(i) = -stride(i);
    }

    // copy back transform:
    transform() = std::move (M);

    // switch axes to match:
    Axis a[] = {
      axes_[realign_perm_[0]],
      axes_[realign_perm_[1]],
      axes_[realign_perm_[2]]
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
          new_line[axis] = pe_scheme(row, realign_perm_[axis]);
          if (new_line[axis] && realign_flip_[realign_perm_[axis]])
            new_line[axis] = -new_line[axis];
        }
        pe_scheme.row (row) = new_line;
      }
      PhaseEncoding::set_scheme (*this, pe_scheme);
      INFO ("Phase encoding scheme modified to conform to MRtrix3 internal header transform realignment");
    }

    // If there's any slice encoding direction information present in the
    //   header, that's also necessary to update here
    auto slice_encoding_it = keyval().find ("SliceEncodingDirection");
    if (slice_encoding_it != keyval().end()) {
      const Eigen::Vector3d orig_dir (Axes::id2dir (slice_encoding_it->second));
      Eigen::Vector3d new_dir;
      for (size_t axis = 0; axis != 3; ++axis)
        new_dir[axis] = orig_dir[realign_perm_[axis]] * (realign_flip_[realign_perm_[axis]] ? -1.0 : 1.0);
      slice_encoding_it->second = Axes::dir2id (new_dir);
      INFO ("Slice encoding direction has been modified to conform to MRtrix3 internal header transform realignment");
    }

  }







  Header concatenate (const vector<Header>& headers, const size_t axis_to_concat, const bool permit_datatype_mismatch)
  {
    Exception e ("Unable to concatenate " + str(headers.size()) + " images along axis " + str(axis_to_concat) + ": ");

    auto datatype_test = [&] (const bool condition)
    {
      if (condition && !permit_datatype_mismatch) {
        e.push_back ("Mismatched data types");
        throw e;
      }
      return condition;
    };

    auto concat_scheme = [] (Eigen::MatrixXd& existing, const Eigen::MatrixXd& extra)
    {
      if (!existing.rows())
        return;
      if (!extra.rows() || (extra.cols() != existing.cols())) {
        existing.resize (0, 0);
        return;
      }
      existing.conservativeResize (existing.rows() + extra.rows(), existing.cols());
      existing.bottomRows (extra.rows()) = extra;
    };

    if (headers.empty())
      return Header();

    size_t global_max_nonunity_dim = 0;
    for (const auto& H : headers) {
      if (axis_to_concat > H.ndim() + 1) {
        e.push_back ("Image \"" + H.name() + "\" is only " + str(H.ndim()) + "D");
        throw e;
      }
      ssize_t this_max_nonunity_dim;
      for (this_max_nonunity_dim = H.ndim()-1; this_max_nonunity_dim >= 0 && H.size (this_max_nonunity_dim) <= 1; --this_max_nonunity_dim);
      global_max_nonunity_dim = std::max (global_max_nonunity_dim, size_t(std::max (ssize_t(0), this_max_nonunity_dim)));
    }

    Header result (headers[0]);

    if (axis_to_concat >= result.ndim()) {
      result.ndim() = axis_to_concat + 1;
      result.size(axis_to_concat) = 1;
    }

    result.stride (axis_to_concat) = result.ndim()+1;

    for (size_t axis = 0; axis != result.ndim(); ++axis) {
      if (axis != axis_to_concat && result.size (axis) <= 1) {
        for (const auto& H : headers) {
          if (H.ndim() > axis) {
            result.size(axis) = H.size (axis);
            result.spacing(axis) = H.spacing (axis);
            break;
          }
        }
      }
    }

    Eigen::MatrixXd dw_scheme, pe_scheme;
    if (axis_to_concat == 3) {
      try {
        dw_scheme = DWI::get_DW_scheme (result);
      } catch (Exception&) { }
      try {
        pe_scheme = PhaseEncoding::get_scheme (result);
      } catch (Exception&) { }
    }

    for (size_t i = 1; i != headers.size(); ++i) {
      const Header& H (headers[i]);

      // Check that dimensions of image are compatible with concatenation
      for (size_t axis = 0; axis <= global_max_nonunity_dim; ++axis) {
        if (axis != axis_to_concat && axis < H.ndim() && H.size (axis) != result.size (axis)) {
          e.push_back ("Images \"" + result.name() + "\" and \"" + H.name() + "\" have inequal sizes along axis " + str(axis_to_concat) + " (" + str(result.size (axis)) + " vs " + str(H.size (axis)) + ")");
          throw e;
        }
      }

      // Expand the image along the axis of concatenation
      result.size (axis_to_concat) += H.ndim() <= axis_to_concat ? 1 : H.size (axis_to_concat);

      // Concatenate 4D schemes if necessary
      if (axis_to_concat == 3) {
        try {
          const auto extra_dw = DWI::parse_DW_scheme (H);
          concat_scheme (dw_scheme, extra_dw);
        } catch (Exception&) {
          dw_scheme.resize (0, 0);
        }
        try {
          const auto extra_pe = PhaseEncoding::get_scheme (H);
          concat_scheme (pe_scheme, extra_pe);
        } catch (Exception&) {
          pe_scheme.resize (0, 0);
        }
      }

      // Resolve key-value pairs
      result.merge_keyval (H);

      // Resolve discrepancies in datatype;
      //   also throw an exception if such mismatch is not permitted
      if (datatype_test (!result.datatype().is_complex() && H.datatype().is_complex()))
        result.datatype().set_flag (DataType::Complex);
      if (datatype_test (result.datatype().is_integer() && !result.datatype().is_signed() && H.datatype().is_signed()))
        result.datatype().set_flag (DataType::Signed);
      if (datatype_test (result.datatype().is_integer() && H.datatype().is_floating_point()))
        result.datatype() = H.datatype();
      if (datatype_test (result.datatype().bytes() < H.datatype().bytes()))
        result.datatype() = (result.datatype()() & DataType::Attributes) + (H.datatype()() & DataType::Type);
    }

    if (axis_to_concat == 3) {
      DWI::set_DW_scheme (result, dw_scheme);
      PhaseEncoding::set_scheme (result, pe_scheme);
    }
    return result;
  }



}
