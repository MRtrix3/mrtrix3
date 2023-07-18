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

#include <map>
#include <string>

#include "command.h"
#include "header.h"
#include "phase_encoding.h"
#include "types.h"
#include "file/json.h"
#include "file/json_utils.h"
#include "dwi/gradient.h"
#include "image_io/pipe.h"


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
      "gradient vector reorientation)"

    + "The -dwgrad, -export_* and -shell_* options provide (information about) "
       "the diffusion weighting gradient table after it has been processed by "
       "the MRtrix3 back-end (vectors normalised, b-values scaled by the square "
       "of the vector norm, depending on the -bvalue_scaling option). To see the "
       "raw gradient table information as stored in the image header, i.e. without "
       "MRtrix3 back-end processing, use \"-property dw_scheme\"."

    + DWI::bvalue_scaling_description;

  ARGUMENTS
    + Argument ("image", "the input image(s).").allow_multiple().type_image_in();

  OPTIONS
    +   Option ("all", "print all properties, rather than the first and last 2 of each.")
    +   Option ("name", "print the file system path of the image")
    +   Option ("format", "image file format")
    +   Option ("ndim", "number of image dimensions")
    +   Option ("size", "image size along each axis")
    +   Option ("spacing", "voxel spacing along each image dimension")
    +   Option ("datatype", "data type used for image data storage")
    +   Option ("strides", "data strides i.e. order and direction of axes data layout")
    +   Option ("offset", "image intensity offset")
    +   Option ("multiplier", "image intensity multiplier")
    +   Option ("transform", "the transformation from image coordinates [mm] to scanner / real world coordinates [mm]")

    + FieldExportOptions

    + DWI::GradImportOptions()
    + DWI::bvalue_scaling_option

    + GradExportOptions
    +   Option ("dwgrad", "the diffusion-weighting gradient table, as interpreted by MRtrix3")
    +   Option ("shell_bvalues", "list the average b-value of each shell")
    +   Option ("shell_sizes", "list the number of volumes in each shell")
    +   Option ("shell_indices", "list the image volumes attributed to each b-value shell")

    + PhaseEncoding::ExportOptions
    +   Option ("petable", "print the phase encoding table")

    + OptionGroup ("Handling of piped images")
    +   Option ("nodelete", "don't delete temporary images or images passed to mrinfo via Unix pipes");

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

void print_shells (const Eigen::MatrixXd& grad, const bool shell_bvalues, const bool shell_sizes, const bool shell_indices)
{
  DWI::Shells dwshells (grad);
  if (shell_bvalues) {
    for (size_t i = 0; i < dwshells.count(); i++)
      std::cout << dwshells[i].get_mean() << " ";
    std::cout << "\n";
  }
  if (shell_sizes) {
    for (size_t i = 0; i < dwshells.count(); i++)
      std::cout << dwshells[i].count() << " ";
    std::cout << "\n";
  }
  if (shell_indices) {
    for (size_t i = 0; i < dwshells.count(); i++)
      std::cout << join(dwshells[i].get_volumes(), ",") + " ";
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
  json["strides"] = strides;
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
  File::JSON::write (header, json["keyval"], header.name());
}






void run ()
{
  auto check_option_group = [](const App::OptionGroup& g) {
    for (auto o : g)
      if (get_options (o.id).size())
        return true;
    return false;
  };

  if (get_options("nodelete").size())
    ImageIO::Pipe::delete_piped_images = false;

  const bool export_grad = check_option_group (GradExportOptions);
  const bool export_pe   = check_option_group (PhaseEncoding::ExportOptions);

  if (export_grad && argument.size() > 1)
    throw Exception ("can only export DW gradient table to file if a single input image is provided");
  if (export_pe && argument.size() > 1)
    throw Exception ("can only export phase encoding table to file if a single input image is provided");

  std::unique_ptr<nlohmann::json> json_keyval (get_options ("json_keyval").size() ? new nlohmann::json : nullptr);
  std::unique_ptr<nlohmann::json> json_all    (get_options ("json_all").size() ? new nlohmann::json : nullptr);

  if (json_all && argument.size() > 1)
    throw Exception ("Cannot use -json_all option with multiple input images");

  const bool name          = get_options("name")          .size();
  const bool format        = get_options("format")        .size();
  const bool ndim          = get_options("ndim")          .size();
  const bool size          = get_options("size")          .size();
  const bool spacing       = get_options("spacing")       .size();
  const bool datatype      = get_options("datatype")      .size();
  const bool strides       = get_options("strides")       .size();
  const bool offset        = get_options("offset")        .size();
  const bool multiplier    = get_options("multiplier")    .size();
  const auto properties    = get_options("property");
  const bool transform     = get_options("transform")     .size();
  const bool dwgrad        = get_options("dwgrad")        .size();
  const bool shell_bvalues = get_options("shell_bvalues") .size();
  const bool shell_sizes   = get_options("shell_sizes")   .size();
  const bool shell_indices = get_options("shell_indices") .size();
  const bool petable       = get_options("petable")       .size();

  const bool print_full_header = !(format || ndim || size || spacing || datatype || strides ||
      offset || multiplier || properties.size() || transform ||
      dwgrad || export_grad || shell_bvalues || shell_sizes || shell_indices ||
      export_pe || petable ||
      json_keyval || json_all);

  for (size_t i = 0; i < argument.size(); ++i) {
    const auto header = Header::open (argument[i]);

    if (name)       std::cout << header.name() << "\n";
    if (format)     std::cout << header.format() << "\n";
    if (ndim)       std::cout << header.ndim() << "\n";
    if (size)       print_dimensions (header);
    if (spacing)    print_spacing (header);
    if (datatype)   std::cout << (header.datatype().specifier() ? header.datatype().specifier() : "invalid") << "\n";
    if (strides)    print_strides (header);
    if (offset)     std::cout << header.intensity_offset() << "\n";
    if (multiplier) std::cout << header.intensity_scale() << "\n";
    if (transform)  print_transform (header);
    if (petable)    std::cout << PhaseEncoding::get_scheme (header) << "\n";

    for (size_t n = 0; n < properties.size(); ++n)
      print_properties (header, properties[n][0]);

    Eigen::MatrixXd grad;
    if (export_grad || check_option_group (GradImportOptions) || dwgrad ||
        shell_bvalues || shell_sizes || shell_indices) {
      grad = DWI::get_DW_scheme (header, DWI::get_cmdline_bvalue_scaling_behaviour());

      if (dwgrad) {
        Eigen::IOFormat fmt (Eigen::FullPrecision, 0, " ", "\n", "", "", "", "");
        std::cout << grad.format(fmt) << "\n";
      }
      if (shell_bvalues || shell_sizes || shell_indices)
        print_shells (grad, shell_bvalues, shell_sizes, shell_indices);
    }

    DWI::export_grad_commandline (header);
    PhaseEncoding::export_commandline (header);

    if (json_keyval)
      File::JSON::write (header, *json_keyval, (argument.size() > 1 ? std::string("") : std::string(argument[0])));

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
