/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include <map>
#include <string>

#include "command.h"
#include "header.h"
#include "phase_encoding.h"
#include "types.h"
#include "file/json.h"
#include "dwi/gradient.h"


using namespace MR;
using namespace App;



const OptionGroup GradImportOptions = DWI::GradImportOptions();
const OptionGroup GradExportOptions = DWI::GradExportOptions();

const OptionGroup FieldExportOptions = OptionGroup ("Options for exporting image header fields")

    + Option ("property", "any text properties embedded in the image header under the "
        "specified key (use 'all' to list all keys found)").allow_multiple()
    +   Argument ("key").type_text()

    + Option ("json_keyval", "export header key/value entries to a JSON file")
    +   Argument ("file").type_file_out()

    + Option ("json_all", "export all header contents to a JSON file")
    +   Argument ("file").type_file_out();



void usage ()
{

  AUTHOR = "J-Donald Tournier (d.tournier@brain.org.au) and Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Display image header information, or extract specific information from the header";

  DESCRIPTION
    + "By default, all information contained in each image header will be printed to the console in "
      "a reader-friendly format."

    + "Alternatively, command-line options may be used to extract specific details from the header(s); "
      "these are printed to the console in a format more appropriate for scripting purposes or "
      "piping to file. If multiple options and/or images are provided, the requested header fields "
      "will be printed in the order in which they appear in the help page, with all requested details "
      "from each input image in sequence printed before the next image is processed."

    + "The command can also write the diffusion gradient table from a single input image to file; "
      "either in the MRtrix or FSL format (bvecs/bvals file pair; includes appropriate diffusion "
      "gradient vector reorientation)";

  ARGUMENTS
    + Argument ("image", "the input image(s).").allow_multiple().type_image_in();

  OPTIONS
    +   Option ("all", "print all properties, rather than the first and last 2 of each.")
    +   Option ("format", "image file format")
    +   Option ("ndim", "number of image dimensions")
    +   Option ("size", "image size along each axis")
    +   Option ("spacing", "voxel spacing along each image dimension")
    +   Option ("datatype", "data type used for image data storage")
    +   Option ("stride", "data strides i.e. order and direction of axes data layout")
    +   Option ("offset", "image intensity offset")
    +   Option ("multiplier", "image intensity multiplier")
    +   Option ("transform", "the voxel to image transformation")

    + NoRealignOption

    + FieldExportOptions

    + GradImportOptions
    + Option ("raw_dwgrad",
        "do not modify the gradient table from what was found in the image headers. This skips the "
        "validation steps normally performed within MRtrix applications (i.e. do not verify that "
        "the number of entries in the gradient table matches the number of volumes in the image, "
        "do not scale b-values by gradient norms, do not normalise gradient vectors)")

    + GradExportOptions
    +   Option ("dwgrad", "the diffusion-weighting gradient table, as stored in the header "
          "(i.e. without any interpretation, scaling of b-values, or normalisation of gradient vectors)")
    +   Option ("shellvalues", "list the average b-value of each shell")
    +   Option ("shellcounts", "list the number of volumes in each shell")

    + PhaseEncoding::ExportOptions
    + Option ("petable", "print the phase encoding table");

}







void print_dimensions (const Header& header)
{
  std::string buffer;
  for (size_t i = 0; i < header.ndim(); ++i) {
    if (i) buffer += " ";
    buffer += str (header.size (i));
  }
  std::cout << buffer << "\n";
}

void print_spacing (const Header& header)
{
  std::string buffer;
  for (size_t i = 0; i < header.ndim(); ++i) {
    if (i) buffer += " ";
    buffer += str (header.spacing (i));
  }
  std::cout << buffer << "\n";
}

void print_strides (const Header& header)
{
  std::string buffer;
  vector<ssize_t> strides (Stride::get (header));
  Stride::symbolise (strides);
  for (size_t i = 0; i < header.ndim(); ++i) {
    if (i) buffer += " ";
    buffer += header.stride (i) ? str (strides[i]) : "?";
  }
  std::cout << buffer << "\n";
}

void print_shells (const Header& header, const bool shellvalues, const bool shellcounts)
{
  DWI::Shells dwshells (DWI::parse_DW_scheme (header));
  if (shellvalues) {
    for (size_t i = 0; i < dwshells.count(); i++)
      std::cout << dwshells[i].get_mean() << " ";
    std::cout << "\n";
  }
  if (shellcounts) {
    for (size_t i = 0; i < dwshells.count(); i++)
      std::cout << dwshells[i].count() << " ";
    std::cout << "\n";
  }
}

void print_transform (const Header& header)
{
  Eigen::IOFormat fmt (Eigen::FullPrecision, 0, " ", "\n", "", "", "", "\n");
  Eigen::Matrix<default_type, 4, 4> matrix;
  matrix.topLeftCorner<3,4>() = header.transform().matrix();
  matrix.row(3) << 0.0, 0.0, 0.0, 1.0;
  std::cout << matrix.format (fmt);
}

void print_properties (const Header& header, const std::string& key, const size_t indent = 0)
{
  if (lowercase (key) == "all") {
    for (const auto& it : header.keyval()) {
      std::cout << it.first << ": ";
      print_properties (header, it.first, it.first.size()+2);
    }
  }
  else {
    const auto values = header.keyval().find (key);
    if (values != header.keyval().end()) {
      auto lines = split (values->second, "\n");
      INFO ("showing property " + key + ":");
      std::cout << lines[0] << "\n";
      for (size_t i = 1; i != lines.size(); ++i) {
        lines[i].insert (0, indent, ' ');
        std::cout << lines[i] << "\n";
      }
    } else {
      WARN ("no \"" + key + "\" entries found in \"" + header.name() + "\"");
    }
  }
}

template <class JSON>
void keyval2json (const Header& header, JSON& json)
{
  for (const auto& kv : header.keyval()) {
    // Text entries that in fact contain matrix / vector data will be
    //   converted to numerical matrices / vectors and written as such
    try {
      const auto M = parse_matrix (kv.second);
      if (M.rows() == 1 && M.cols() == 1)
        throw Exception ("Single scalar value rather than a matrix");
      for (ssize_t row = 0; row != M.rows(); ++row) {
        vector<default_type> data (M.cols());
        for (ssize_t i = 0; i != M.cols(); ++i)
          data[i] = M (row, i);
        if (json.find (kv.first) == json.end())
          json[kv.first] = { data };
        else
          json[kv.first].push_back (data);
      }
    } catch (...) {
      if (json.find (kv.first) == json.end()) {
        json[kv.first] = kv.second;
      } else if (json[kv.first] != kv.second) {
        // If the value for this key differs between images, turn the JSON entry into an array
        if (json[kv.first].is_array())
          json[kv.first].push_back (kv.second);
        else
          json[kv.first] = { json[kv.first], kv.second };
      }
    }
  }
}

void header2json (const Header& header, nlohmann::json& json)
{
  // Capture _all_ header fields, not just the optional key-value pairs
  json["name"] = header.name();
  vector<size_t> size (header.ndim());
  vector<default_type> spacing (header.ndim());
  for (size_t axis = 0; axis != header.ndim(); ++axis) {
    size[axis] = header.size (axis);
    spacing[axis] = header.spacing (axis);
  }
  json["size"] = size;
  json["spacing"] = spacing;
  vector<ssize_t> strides (Stride::get (header));
  Stride::symbolise (strides);
  json["stride"] = strides;
  json["format"] = header.format();
  json["datatype"] = header.datatype().specifier();
  json["intensity_offset"] = header.intensity_offset();
  json["intensity_scale"] = header.intensity_scale();
  const transform_type& T (header.transform());
  json["transform"] = { { T(0,0), T(0,1), T(0,2), T(0,3) },
                        { T(1,0), T(1,1), T(1,2), T(1,3) },
                        { T(2,0), T(2,1), T(2,2), T(2,3) },
                        {    0.0,    0.0,    0.0,    1.0 } };
  // Load key-value entries into a nested keyval.* member
  keyval2json (header, json["keyval"]);
}






void run ()
{
  auto check_option_group = [](const App::OptionGroup& g) { for (auto o: g) if (get_options (o.id).size()) return true; return false; };

  const bool export_grad = check_option_group (GradExportOptions);
  const bool export_pe   = check_option_group (PhaseEncoding::ExportOptions);

  if (export_grad && argument.size() > 1)
    throw Exception ("can only export DW gradient table to file if a single input image is provided");
  if (export_pe && argument.size() > 1)
    throw Exception ("can only export phase encoding table to file if a single input image is provided");

  std::unique_ptr<nlohmann::json> json_keyval (get_options ("json_keyval").size() ? new nlohmann::json : nullptr);
  std::unique_ptr<nlohmann::json> json_all    (get_options ("json_all").size() ? new nlohmann::json : nullptr);

  if (get_options ("norealign").size())
    Header::do_not_realign_transform = true;

  const bool format      = get_options("format")        .size();
  const bool ndim        = get_options("ndim")          .size();
  const bool size        = get_options("size")          .size();
  const bool spacing     = get_options("spacing")       .size();
  const bool datatype    = get_options("datatype")      .size();
  const bool stride      = get_options("stride")        .size();
  const bool offset      = get_options("offset")        .size();
  const bool multiplier  = get_options("multiplier")    .size();
  const auto properties  = get_options("property");
  const bool transform   = get_options("transform")     .size();
  const bool dwgrad      = get_options("dwgrad")        .size();
  const bool shellvalues = get_options("shellvalues")   .size();
  const bool shellcounts = get_options("shellcounts")   .size();
  const bool raw_dwgrad  = get_options("raw_dwgrad")    .size();
  const bool petable     = get_options("petable")       .size();

  const bool print_full_header = !(format || ndim || size || spacing || datatype || stride ||
      offset || multiplier || properties.size() || transform ||
      dwgrad || export_grad || shellvalues || shellcounts || export_pe || petable ||
      json_keyval || json_all);

  for (size_t i = 0; i < argument.size(); ++i) {
    auto header = Header::open (argument[i]);
    if (raw_dwgrad)
      DWI::set_DW_scheme (header, DWI::get_DW_scheme (header));
    else if (export_grad || check_option_group (GradImportOptions) || dwgrad || shellvalues || shellcounts)
      DWI::set_DW_scheme (header, DWI::get_valid_DW_scheme (header, true));

    if (format)     std::cout << header.format() << "\n";
    if (ndim)       std::cout << header.ndim() << "\n";
    if (size)       print_dimensions (header);
    if (spacing)    print_spacing (header);
    if (datatype)   std::cout << (header.datatype().specifier() ? header.datatype().specifier() : "invalid") << "\n";
    if (stride)     print_strides (header);
    if (offset)     std::cout << header.intensity_offset() << "\n";
    if (multiplier) std::cout << header.intensity_scale() << "\n";
    if (transform)  print_transform (header);
    if (dwgrad)     std::cout << DWI::get_DW_scheme (header) << "\n";
    if (shellvalues || shellcounts) print_shells (header, shellvalues, shellcounts);
    if (petable)    std::cout << PhaseEncoding::get_scheme (header) << "\n";

    for (size_t n = 0; n < properties.size(); ++n)
      print_properties (header, properties[n][0]);

    DWI::export_grad_commandline (header);
    PhaseEncoding::export_commandline (header);

    if (json_keyval)
      keyval2json (header, *json_keyval);

    if (json_all)
      header2json (header, *json_all);

    if (print_full_header)
      std::cout << header.description (get_options ("all").size());
  }

  if (json_keyval) {
    auto opt = get_options ("json_keyval");
    assert (opt.size());
    File::OFStream out (opt[0][0]);
    out << json_keyval->dump(4) << "\n";
  }

  if (json_all) {
    auto opt = get_options ("json_all");
    assert (opt.size());
    File::OFStream out (opt[0][0]);
    out << json_all->dump(4) << "\n";
  }

}

