/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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
#include <iomanip>
#include <set>
#include <sstream>

#include "app.h"
#include "axes.h"
#include "file/name_parser.h"
#include "file/path.h"
#include "formats/list.h"
#include "image_io/default.h"
#include "image_io/scratch.h"
#include "math/math.h"
#include "metadata/phase_encoding.h"
#include "metadata/slice_encoding.h"
#include "mrtrix.h"
#include "stride.h"
#include "transform.h"

#include "dwi/gradient.h"

namespace MR {

bool Header::do_realign_transform = true;

void Header::check(const Header &H) const {
  if (ndim() != H.ndim())
    throw Exception("dimension mismatch between image files for \"" + name() + "\"");

  for (size_t n = 0; n < ndim(); ++n) {
    if (size(n) != H.size(n))
      throw Exception("dimension mismatch between image files for \"" + name() + "\"");

    if (stride(n) != H.stride(n))
      throw Exception("data strides differs image files for \"" + name() + "\"");

    if (std::isfinite(spacing(n)) && std::isfinite(H.spacing(n)) && spacing(n) != H.spacing(n))
      WARN("voxel dimensions differ between image files for \"" + name() + "\"");
  }

  if ((transform().matrix() - H.transform().matrix()).cwiseAbs().maxCoeff() > 1.0e-6)
    WARN("transform matrices differ between image files for \"" + name() + "\"");
  ;

  if (datatype() != H.datatype())
    throw Exception("data types differ between image files for \"" + name() + "\"");

  if (intensity_offset() != H.intensity_offset() || intensity_scale() != H.intensity_scale())
    throw Exception("scaling coefficients differ between image files for \"" + name() + "\"");
}

void Header::merge_keyval(const KeyValues &in) {
  std::map<std::string, std::string> new_keyval;
  std::set<std::string> unique_comments;
  for (const auto &item : keyval()) {
    if (item.first == "comments") {
      new_keyval.insert(item);
      const auto comments = split_lines(item.second);
      for (const auto &c : comments)
        unique_comments.insert(c);
    } else if (item.first != "command_history") {
      new_keyval.insert(item);
    }
  }
  for (const auto &item : in) {
    if (item.first == "comments") {
      const auto comments = split_lines(item.second);
      for (const auto &c : comments) {
        if (unique_comments.find(c) == unique_comments.end()) {
          add_line(new_keyval["comments"], c);
          unique_comments.insert(c);
        }
      }
    } else {
      auto it = keyval().find(item.first);
      if (it == keyval().end() || it->second == item.second) {
        new_keyval.insert(item);
      } else if (item.first == "SliceTiming") {
        new_keyval["SliceTiming"] = Metadata::SliceEncoding::resolve_slice_timing(item.second, it->second);
      } else if (item.first == "dw_scheme") {
        if (item.second == "variable" || it->second == "variable") {
          new_keyval["dw_scheme"] = "variable";
        } else {
          try {
            auto scheme = DWI::resolve_DW_scheme(parse_matrix(item.second), parse_matrix(it->second));
            DWI::set_DW_scheme(new_keyval, scheme);
          } catch (Exception &e) {
            INFO("Unable to merge inconsistent DW gradient tables between headers");
            new_keyval["dw_scheme"] = "variable";
          }
        }
      } else {
        new_keyval[item.first] = "variable";
      }
    }
  }
  std::swap(keyval_, new_keyval);
}

namespace {

std::string short_description(const Header &H) {
  std::vector<std::string> dims;
  for (size_t n = 0; n < H.ndim(); ++n)
    dims.push_back(str(H.size(n)));
  std::vector<std::string> vox;
  for (size_t n = 0; n < H.ndim(); ++n)
    vox.push_back(str(H.spacing(n)));

  return " with dimensions " + join(dims, "x") + ", voxel spacing " + join(vox, "x") + ", datatype " +
         H.datatype().specifier();
}
} // namespace

Header Header::open(const std::string &image_name) {
  if (image_name.empty())
    throw Exception("no name supplied to open image!");

  Header H;

  try {
    INFO("opening image \"" + image_name + "\"...");

    File::ParsedName::List list;
    const auto num = list.parse_scan_check(image_name);

    const Formats::Base **format_handler = Formats::handlers;
    size_t item_index = 0;
    H.name() = list[item_index].name();

    for (; *format_handler; format_handler++) {
      if ((H.io = (*format_handler)->read(H)))
        break;
    }

    if (!*format_handler)
      throw Exception("unknown format for image \"" + H.name() + "\"");
    assert(H.io);

    H.format_ = (*format_handler)->description;

    if (!num.empty()) {

      const Header template_header(H);

      // Convenient to know a priori which loop index corresponds to which image axis
      // This needs to detect unity-sized axes and flag the loop to concatenate data along that axis
      std::vector<size_t> loopindex2axis;
      size_t axis;
      for (axis = 0; axis != H.ndim(); ++axis) {
        if (H.size(axis) == 1) {
          loopindex2axis.push_back(axis);
          if (loopindex2axis.size() == num.size())
            break;
        }
      }
      for (; loopindex2axis.size() < num.size(); ++axis)
        loopindex2axis.push_back(axis);
      std::reverse(loopindex2axis.begin(), loopindex2axis.end());

      // Reimplemented support for [] notation using recursive function calls
      // Note that the very first image header has already been opened before this function is
      //   invoked for the first time; "vector<Header>& this_data" is used to propagate this
      //   data to deeper layers when necessary
      std::function<void(Header &, std::vector<Header> &, const size_t)> import =
          [&](Header &result, std::vector<Header> &this_data, const size_t loop_index) -> void {
        if (loop_index == num.size() - 1) {
          std::vector<std::unique_ptr<ImageIO::Base>> ios;
          if (!this_data.empty())
            ios.push_back(std::move(this_data[0].io));
          for (size_t i = this_data.size(); i != static_cast<size_t>(num[loop_index]); ++i) {
            Header header(template_header);
            std::unique_ptr<ImageIO::Base> io_handler;
            header.name() = list[++item_index].name();
            header.keyval().clear();
            if (!(io_handler = (*format_handler)->read(header)))
              throw Exception("image specifier contains mixed format files");
            assert(io_handler);
            template_header.check(header);
            this_data.push_back(std::move(header));
            ios.push_back(std::move(io_handler));
          }
          result = concatenate(this_data, loopindex2axis[loop_index], false);
          result.io = std::move(ios[0]);
          for (size_t i = 1; i != ios.size(); ++i)
            result.io->merge(*ios[i]);
          return;
        } // End branch for when loop_index is the maximum, ie. innermost loop
        // For each coordinate along this axis, need to concatenate headers from the
        //   next lower axis
        std::vector<Header> nested_data;
        // The nested concatenation may still include the very first header that has already been read;
        //   this needs to be propagated through to the nested call
        if (!this_data.empty()) {
          assert(this_data.size() == 1);
          nested_data.push_back(std::move(this_data[0]));
          this_data.clear();
        }
        for (size_t i = 0; i != static_cast<size_t>(num[loop_index]); ++i) {
          Header temp;
          import(temp, nested_data, loop_index + 1);
          this_data.push_back(std::move(temp));
          nested_data.clear();
        }
        result = concatenate(this_data, loopindex2axis[loop_index], false);
        result.io = std::move(this_data[0].io);
        for (size_t i = 1; i != static_cast<size_t>(num[loop_index]); ++i)
          result.io->merge(*this_data[i].io);
      };

      std::vector<Header> headers;
      headers.push_back(std::move(H));
      import(H, headers, 0);
      H.name() = image_name;
    } // End branching for [] notation

    H.sanitise();
    H.realign_transform();
  } catch (CancelException &e) {
    throw;
  } catch (Exception &E) {
    throw Exception(E, "error opening image \"" + image_name + "\"");
  }

  INFO("image \"" + H.name() + "\" opened" + short_description(H));

  return H;
}

namespace {
inline bool check_strides_match(const std::vector<ssize_t> &a, const std::vector<ssize_t> &b) {
  size_t n = 0;
  for (; n < std::min(a.size(), b.size()); ++n)
    if (a[n] != b[n])
      return false;
  for (size_t i = n; i < a.size(); ++i)
    if (a[i] > 1)
      return false;
  for (size_t i = n; i < b.size(); ++i)
    if (b[i] > 1)
      return false;
  return true;
}

} // namespace

Header Header::create(const std::string &image_name, const Header &template_header, bool add_to_command_history) {
  if (image_name.empty())
    throw Exception("no name supplied to open image!");

  Header H(template_header);
  const auto previous_datatype = H.datatype();

  try {
    INFO("creating image \"" + image_name + "\"...");
    if (add_to_command_history) {
      // Make sure the current command is not concatenated more than once
      const auto command_history = split_lines(H.keyval()["command_history"]);
      if (!(!command_history.empty() && command_history.back() == App::command_history_string))
        add_line(H.keyval()["command_history"], App::command_history_string);
    }

    H.keyval()["mrtrix_version"] = App::mrtrix_version;
    if (!App::project_version.empty())
      H.keyval()["project_version"] = App::project_version;

    H.sanitise();

    File::NameParser parser;
    parser.parse(image_name);
    std::vector<uint32_t> Pdim(parser.ndim());

    std::vector<int> Hdim(H.ndim());
    for (size_t i = 0; i < H.ndim(); ++i)
      Hdim[i] = H.size(i);

    H.name() = image_name;

    const std::vector<ssize_t> strides(Stride::get_symbolic(H));
    const Formats::Base **format_handler = Formats::handlers;
    for (; *format_handler; format_handler++)
      if ((*format_handler)->check(H, H.ndim() - Pdim.size()))
        break;

    if (!*format_handler) {
      const std::string basename = Path::basename(image_name);
      const size_t extension_index = basename.find_last_of(".");
      if (extension_index == std::string::npos)
        throw Exception("unknown format for image \"" + image_name + "\" (no file extension specified)");
      else
        throw Exception("unknown format for image \"" + image_name +
                        "\" (unsupported file extension: " + basename.substr(extension_index) + ")");
    }

    const std::vector<ssize_t> strides_aftercheck(Stride::get_symbolic(H));
    if (!check_strides_match(strides, strides_aftercheck)) {
      INFO("output strides for image " + image_name + " modified to " + str(strides_aftercheck) +
           " - requested strides " + str(strides) + " are not supported in " + (*format_handler)->description +
           " format");
    }

    H.datatype().set_byte_order_native();
    size_t a = 0;
    for (size_t n = 0; n < Pdim.size(); ++n) {
      while (a < H.ndim() && H.stride(a))
        a++;
      Pdim[n] = Hdim[a++];
    }
    parser.calculate_padding(Pdim);

    // FIXME This fails to appropriately assign rows of these schemes to images
    //   if splitting 4D image into 2D images
    const bool split_4d_schemes = (parser.ndim() == 1 && template_header.ndim() == 4);
    Eigen::MatrixXd dw_scheme, pe_scheme;
    try {
      dw_scheme = DWI::parse_DW_scheme(template_header);
    } catch (Exception &) {
      DWI::clear_DW_scheme(H);
    }
    try {
      pe_scheme = Metadata::PhaseEncoding::parse_scheme(template_header.keyval(), template_header);
    } catch (Exception &) {
      Metadata::PhaseEncoding::clear_scheme(H.keyval());
    }
    if (split_4d_schemes) {
      try {
        DWI::check_DW_scheme(template_header, dw_scheme);
        DWI::set_DW_scheme(H, dw_scheme.row(0));
      } catch (Exception &) {
        dw_scheme.resize(0, 0);
        DWI::clear_DW_scheme(H);
      }
      try {
        Metadata::PhaseEncoding::check(pe_scheme, template_header);
        Metadata::PhaseEncoding::set_scheme(H.keyval(), pe_scheme.row(0));
      } catch (Exception &) {
        pe_scheme.resize(0, 0);
        Metadata::PhaseEncoding::clear_scheme(H.keyval());
      }
    }

    Header header(H);
    std::vector<uint32_t> num(Pdim.size());

    if (!is_dash(image_name))
      H.name() = parser.name(num);

    H.io = (*format_handler)->create(H);
    assert(H.io);
    H.format_ = (*format_handler)->description;

    auto get_next = [](decltype(num) &pos, const decltype(Pdim) &limits) {
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
    while (get_next(num, Pdim)) {
      header.name() = parser.name(num);
      ++counter;
      if (split_4d_schemes) {
        if (dw_scheme.rows())
          DWI::set_DW_scheme(header, dw_scheme.row(counter));
        if (pe_scheme.rows())
          Metadata::PhaseEncoding::set_scheme(header.keyval(), pe_scheme.row(counter));
      }
      std::shared_ptr<ImageIO::Base> io_handler((*format_handler)->create(header));
      assert(io_handler);
      H.io->merge(*io_handler);
    }

    if (!Pdim.empty()) {
      int a = 0, n = 0;
      ssize_t next_stride = 0;
      for (size_t i = 0; i < H.ndim(); ++i) {
        if (H.stride(i)) {
          ++n;
          next_stride = std::max(next_stride, MR::abs(H.stride(i)));
        }
      }

      H.axes_.resize(n + Pdim.size());

      for (size_t i = 0; i < Pdim.size(); ++i) {
        while (H.stride(a))
          ++a;
        H.size(a) = Pdim[i];
        H.stride(a) = ++next_stride;
      }

      H.name() = image_name;
    }

    if (split_4d_schemes) {
      DWI::set_DW_scheme(H, dw_scheme);
      Metadata::PhaseEncoding::set_scheme(H.keyval(), pe_scheme);
    }
    H.io->set_image_is_new(true);
    H.io->set_readwrite(true);

    H.sanitise();
  } catch (Exception &E) {
    throw Exception(E, "error creating image \"" + image_name + "\"");
  }

  DataType new_datatype = H.datatype();
  if (new_datatype != previous_datatype) {
    new_datatype.unset_flag(DataType::BigEndian);
    new_datatype.unset_flag(DataType::LittleEndian);
    if (new_datatype != previous_datatype)
      WARN(std::string("requested datatype (") + previous_datatype.specifier() +
           ") not supported - substituting with " + H.datatype().specifier());
  }

  INFO("image \"" + H.name() + "\" created" + short_description(H));

  return H;
}

Header Header::scratch(const Header &template_header, const std::string &label) {
  Header H(template_header);
  H.name() = label;
  H.reset_intensity_scaling();
  H.sanitise();
  H.format_ = "scratch image";
  H.io = std::make_unique<ImageIO::Scratch>(H);
  return H;
}

std::ostream &operator<<(std::ostream &stream, const Header &H) {
  stream << "\"" << H.name() << "\", " << H.datatype().specifier() << ", size [ ";
  for (size_t n = 0; n < H.ndim(); ++n)
    stream << H.size(n) << " ";
  stream << "], voxel size [ ";
  for (size_t n = 0; n < H.ndim(); ++n)
    stream << H.spacing(n) << " ";
  stream << "], strides [ ";
  for (size_t n = 0; n < H.ndim(); ++n)
    stream << H.stride(n) << " ";
  stream << "]";
  return stream;
}

std::string Header::description(bool print_all) const {
  std::string desc("************************************************\n"
                   "Image name:          \"" +
                   name() +
                   "\"\n"
                   "************************************************\n");

  desc += "  Dimensions:        ";
  size_t i;
  for (i = 0; i < ndim(); i++) {
    if (i)
      desc += " x ";
    desc += str(size(i));
  }

  desc += "\n  Voxel size:        ";
  for (i = 0; i < ndim(); i++) {
    if (i)
      desc += " x ";
    desc += std::isnan(spacing(i)) ? "?" : str(spacing(i), 6);
  }
  desc += "\n";

  desc += "  Data strides:      [ ";
  auto strides(Stride::get(*this));
  Stride::symbolise(strides);
  for (i = 0; i < ndim(); i++)
    desc += stride(i) ? str(strides[i]) + " " : "? ";
  desc += "]\n";

  if (io) {
    desc += std::string("  Format:            ") + (format().empty() ? "undefined" : format()) + "\n";
    desc += std::string("  Data type:         ") + datatype().description() + "\n";
    desc +=
        "  Intensity scaling: offset = " + str(intensity_offset()) + ", multiplier = " + str(intensity_scale()) + "\n";
  }

  desc += "  Transform:         ";
  for (size_t i = 0; i < 3; i++) {
    if (i)
      desc += "                     ";
    for (size_t j = 0; j < 4; j++) {
      std::ostringstream oss;
      oss << std::setprecision(4) << std::setw(12) << transform()(i, j);
      desc += oss.str();
    }
    desc += "\n";
  }

  for (const auto &p : keyval()) {
    std::string key = "  " + p.first + ": ";
    if (key.size() < 21)
      key.resize(21, ' ');
    const auto entries = split_lines(p.second);
    if (!entries.empty()) {
      bool shorten = (!print_all && entries.size() > 5);
      desc += key + entries[0] + "\n";
      if (entries.size() > 5) {
        key = "  [" + str(entries.size()) + " entries] ";
        if (key.size() < 21)
          key.resize(21, ' ');
      } else
        key = "                     ";
      for (size_t n = 1; n < (shorten ? size_t(2) : entries.size()); ++n) {
        desc += key + entries[n] + "\n";
        key = "                     ";
      }
      if (!print_all && entries.size() > 5) {
        desc += key + "...\n";
        for (size_t n = entries.size() - 2; n < entries.size(); ++n)
          desc += key + entries[n] + "\n";
      }
    } else {
      desc += key + "(empty)\n";
    }
  }

  return desc;
}

void Header::sanitise_voxel_sizes() {
  if (ndim() < 3) {
    INFO("image contains fewer than 3 dimensions - adding extra dimensions");
    axes_.resize(3);
  }

  if (!std::isfinite(spacing(0)) || !std::isfinite(spacing(1)) || !std::isfinite(spacing(2))) {
    WARN("invalid voxel sizes - resetting to sane defaults");
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

void Header::sanitise_transform() {
  if (!transform().matrix().allFinite()) {
    WARN("transform matrix contains invalid entries - resetting to sane defaults");
    transform() = Transform::get_default(*this);
  }

  // check that cosine vectors are unit length (to some precision):
  bool rescale_cosine_vectors = false;
  for (size_t i = 0; i < 3; ++i) {
    auto length = transform().matrix().col(i).head<3>().norm();
    if (std::fabs(length - 1.0) > 1.0e-6)
      rescale_cosine_vectors = true;
  }

  // if unit length, rescale and modify voxel sizes accordingly:
  if (rescale_cosine_vectors) {
    INFO("non unit cosine vectors detected - normalising and rescaling voxel sizes to match");
    for (size_t i = 0; i < 3; ++i) {
      auto length = transform().matrix().col(i).head(3).norm();
      transform().matrix().col(i).head(3) /= length;
      spacing(i) *= length;
    }
  }
}

void Header::realign_transform() {
  realignment_.orig_transform_ = transform();
  realignment_.applied_transform_ = Realignment::applied_transform_type::Identity();
  realignment_.orig_strides_ = Stride::get(*this);
  realignment_.orig_keyval_ = keyval();

  if (!do_realign_transform)
    return;

  // find which row of the transform is closest to each scanner axis:
  realignment_.shuffle_ = Axes::get_shuffle_to_make_RAS(transform());

  // check if image is already near-axial, return if true:
  if (realignment_.is_identity())
    return;

  auto M(transform());
  auto translation = M.translation();

  // modify translation vector:
  for (size_t i = 0; i < 3; ++i) {
    if (realignment_.flip(i)) {
      const default_type length = (size(i) - 1) * spacing(i);
      auto axis = M.matrix().col(i);
      for (size_t n = 0; n < 3; ++n) {
        axis[n] = -axis[n];
        translation[n] -= length * axis[n];
      }
      realignment_.applied_transform_.row(i) *= -1.0;
    }
  }

  // switch and/or invert rows if needed:
  for (size_t i = 0; i < 3; ++i) {
    auto row_transform = M.matrix().row(i).head<3>();
    row_transform = Eigen::RowVector3d(row_transform[realignment_.permutation(0)],
                                       row_transform[realignment_.permutation(1)],
                                       row_transform[realignment_.permutation(2)]);

    auto col_applied = realignment_.applied_transform_.matrix().col(i);
    col_applied = Eigen::RowVector3i(col_applied[realignment_.permutation(0)],
                                     col_applied[realignment_.permutation(1)],
                                     col_applied[realignment_.permutation(2)]);

    if (realignment_.flip(i))
      stride(i) = -stride(i);
  }

  // copy back transform:
  transform() = std::move(M);

  // switch axes to match:
  const std::array<Axis, 3> a = {axes_[realignment_.permutation(0)],  //
                                 axes_[realignment_.permutation(1)],  //
                                 axes_[realignment_.permutation(2)]}; //
  axes_[0] = a[0];
  axes_[1] = a[1];
  axes_[2] = a[2];

  INFO("Axes and transform of image \"" + name() + "\" altered to approximate RAS coordinate system");

  Metadata::PhaseEncoding::transform_for_image_load(keyval(), *this);
  Metadata::SliceEncoding::transform_for_image_load(keyval(), *this);
}

Header
concatenate(const std::vector<Header> &headers, const size_t axis_to_concat, const bool permit_datatype_mismatch) {
  Exception e("Unable to concatenate " + str(headers.size()) + " images along axis " + str(axis_to_concat) + ": ");

  auto datatype_test = [&](const bool condition) {
    if (condition && !permit_datatype_mismatch) {
      e.push_back("Mismatched data types");
      throw e;
    }
    return condition;
  };

  auto concat_scheme = [](Eigen::MatrixXd &existing, const Eigen::MatrixXd &extra) {
    if (!existing.rows())
      return;
    if (!extra.rows() || (extra.cols() != existing.cols())) {
      existing.resize(0, 0);
      return;
    }
    existing.conservativeResize(existing.rows() + extra.rows(), existing.cols());
    existing.bottomRows(extra.rows()) = extra;
  };

  if (headers.empty())
    return Header();

  size_t global_max_nonunity_dim = 0;
  for (const auto &H : headers) {
    if (axis_to_concat > H.ndim() + 1) {
      e.push_back("Image \"" + H.name() + "\" is only " + str(H.ndim()) + "D");
      throw e;
    }
    ssize_t this_max_nonunity_dim;
    for (this_max_nonunity_dim = H.ndim() - 1; this_max_nonunity_dim >= 0 && H.size(this_max_nonunity_dim) <= 1;
         --this_max_nonunity_dim)
      ;
    global_max_nonunity_dim =
        std::max(global_max_nonunity_dim, static_cast<size_t>(std::max(ssize_t(0), this_max_nonunity_dim)));
  }

  Header result(headers[0]);

  if (axis_to_concat >= result.ndim()) {
    Stride::symbolise(result);
    result.ndim() = axis_to_concat + 1;
    result.size(axis_to_concat) = 1;
    result.stride(axis_to_concat) = axis_to_concat + 1;
    Stride::actualise(result);
  }

  for (size_t axis = 0; axis != result.ndim(); ++axis) {
    if (axis != axis_to_concat && result.size(axis) <= 1) {
      for (const auto &H : headers) {
        if (H.ndim() > axis) {
          result.size(axis) = H.size(axis);
          result.spacing(axis) = H.spacing(axis);
          break;
        }
      }
    }
  }

  // Need an enum to track what we're going to do with these fields,
  //   rather than relying exclusively on Header::merge_keyval()
  enum class scheme_manip_t { ABSENT, MERGE, CONCAT, ERASE };
  Eigen::MatrixXd dw_scheme, pe_scheme;
  scheme_manip_t dwscheme_manip = scheme_manip_t::MERGE;
  scheme_manip_t pescheme_manip = scheme_manip_t::MERGE;
  if (axis_to_concat == 3) {
    try {
      dw_scheme = DWI::get_DW_scheme(result);
      dwscheme_manip = scheme_manip_t::CONCAT;
    } catch (Exception &) {
      dwscheme_manip = scheme_manip_t::ABSENT;
    }
    try {
      pe_scheme = Metadata::PhaseEncoding::get_scheme(result);
      pescheme_manip = pe_scheme.rows() == 0 ? scheme_manip_t::ABSENT : scheme_manip_t::CONCAT;
    } catch (Exception &) {
      pescheme_manip = scheme_manip_t::ERASE;
    }
  }

  for (size_t i = 1; i != headers.size(); ++i) {
    const Header &H(headers[i]);

    // Check that dimensions of image are compatible with concatenation
    for (size_t axis = 0; axis <= global_max_nonunity_dim; ++axis) {
      if (axis != axis_to_concat && axis < H.ndim() && H.size(axis) != result.size(axis)) {
        e.push_back("Images \"" + result.name() + "\" and \"" + H.name() + "\" have inequal sizes along axis " +
                    str(axis_to_concat) + " (" + str(result.size(axis)) + " vs " + str(H.size(axis)) + ")");
        throw e;
      }
    }

    // Expand the image along the axis of concatenation
    result.size(axis_to_concat) += H.ndim() <= axis_to_concat ? 1 : H.size(axis_to_concat);

    if (axis_to_concat == 3) {

      // Create a local copy of the key-value data
      //   in case we need to apply modifications prior to the key-value merge operation
      KeyValues kv(H.keyval());

      // Generate local copies of any schemes
      Eigen::MatrixXd extra_dw;
      Eigen::MatrixXd extra_pe;
      try {
        extra_dw = DWI::parse_DW_scheme(H);
      } catch (Exception &) {
      }
      try {
        extra_pe = Metadata::PhaseEncoding::get_scheme(H);
      } catch (Exception &) {
      }

      switch (dwscheme_manip) {
      case scheme_manip_t::ABSENT:
        if (extra_dw.rows() > 0)
          dwscheme_manip = scheme_manip_t::ERASE;
        break;
      case scheme_manip_t::MERGE:
        assert(false);
        throw Exception("Logic error in header key-value merge of DW scheme");
      case scheme_manip_t::CONCAT:
        if (extra_dw.rows() == 0) {
          dw_scheme.resize(0, 0);
          dwscheme_manip = scheme_manip_t::ERASE;
        } else {
          concat_scheme(dw_scheme, extra_dw);
        }
        break;
      case scheme_manip_t::ERASE:
        break;
      }

      switch (pescheme_manip) {
      case scheme_manip_t::ABSENT:
        if (extra_pe.rows() > 0)
          pescheme_manip = scheme_manip_t::ERASE;
        break;
      case scheme_manip_t::MERGE:
        assert(false);
        throw Exception("Logic error in header key-value merge of PE scheme");
      case scheme_manip_t::CONCAT:
        if (extra_pe.rows() == 0) {
          pe_scheme.resize(0, 0);
          pescheme_manip = scheme_manip_t::ERASE;
        } else {
          concat_scheme(pe_scheme, extra_pe);
        }
        break;
      case scheme_manip_t::ERASE:
        break;
      }

      // Merge with modified key-value contents where these schemes have been removed
      DWI::clear_DW_scheme(kv);
      Metadata::PhaseEncoding::clear_scheme(kv);
      result.merge_keyval(kv);

    } else { // Axis of concatenation is not 3; can do a straight merge
      result.merge_keyval(H.keyval());
    }

    // Resolve discrepancies in datatype;
    //   also throw an exception if such mismatch is not permitted
    if (datatype_test(!result.datatype().is_complex() && H.datatype().is_complex()))
      result.datatype().set_flag(DataType::Complex);
    if (datatype_test(result.datatype().is_integer() && !result.datatype().is_signed() && H.datatype().is_signed()))
      result.datatype().set_flag(DataType::Signed);
    if (datatype_test(result.datatype().is_integer() && H.datatype().is_floating_point()))
      result.datatype() = H.datatype();
    if (datatype_test(result.datatype().bytes() < H.datatype().bytes()))
      result.datatype() = (result.datatype()() & DataType::Attributes) + (H.datatype()() & DataType::Type);
  }

  // If manually concatenating these data along axis 3,
  //   need to finalise after the last header has been processed
  switch (dwscheme_manip) {
  case scheme_manip_t::ABSENT:
  case scheme_manip_t::MERGE:
    break;
  case scheme_manip_t::CONCAT:
    DWI::set_DW_scheme(result, dw_scheme);
    break;
  case scheme_manip_t::ERASE:
    WARN("Erasing diffusion gradient table:"                          //
         " could not reconstruct across concatenated image headers"); //
    DWI::clear_DW_scheme(result);
    break;
  }
  switch (pescheme_manip) {
  case scheme_manip_t::ABSENT:
  case scheme_manip_t::MERGE:
    break;
  case scheme_manip_t::CONCAT:
    Metadata::PhaseEncoding::set_scheme(result.keyval(), pe_scheme);
    break;
  case scheme_manip_t::ERASE:
    WARN("Erasing phase encoding information:"                        //
         " could not reconstruct across concatenated image headers"); //
    Metadata::PhaseEncoding::clear_scheme(result.keyval());
    break;
  }
  return result;
}

Header::Realignment::Realignment() : applied_transform_(applied_transform_type::Identity()), orig_keyval_() {
  orig_transform_.matrix().fill(std::numeric_limits<default_type>::quiet_NaN());
}

} // namespace MR
