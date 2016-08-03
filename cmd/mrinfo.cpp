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


#include <map>
#include <string>
#include <vector>

#include "command.h"
#include "header.h"
#include "file/json.h"
#include "dwi/gradient.h"


using namespace MR;
using namespace App;



const OptionGroup GradImportOptions = DWI::GradImportOptions();
const OptionGroup GradExportOptions = DWI::GradExportOptions();


void usage ()
{

  AUTHOR = "J-Donald Tournier (d.tournier@brain.org.au) and Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
    + "display header information, or extract specific information from the header."

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
    +   Option ("format", "image file format")
    +   Option ("ndim", "number of image dimensions")
    +   Option ("size", "image size along each axis")
    +   Option ("vox", "voxel size along each image dimension")
    +   Option ("datatype", "data type used for image data storage")
    +   Option ("stride", "data strides i.e. order and direction of axes data layout")
    +   Option ("offset", "image intensity offset")
    +   Option ("multiplier", "image intensity multiplier")
    +   Option ("transform", "the image transform")
    +   Option ("norealign",
          "do not realign transform to near-default RAS coordinate system (the "
          "default behaviour on image load). This is useful to inspect the transform "
          "and strides as they are actually stored in the header, rather than as "
          "MRtrix interprets them.")

    + Option ("property", "any text properties embedded in the image header under the "
        "specified key (use 'all' to list all keys found)").allow_multiple()
    +   Argument ("key").type_text()

    + Option ("json_export", "export header key/value entries to a JSON file")
    +   Argument ("file").type_file_out()

    + GradImportOptions
    + Option ("raw_dwgrad",
        "do not modify the gradient table from what was found in the image headers. This skips the "
        "validation steps normally performed within MRtrix applications (i.e. do not verify that "
        "the number of entries in the gradient table matches the number of volumes in the image, "
        "do not scale b-values by gradient norms, do not normalise gradient vectors)")

    + GradExportOptions
    +   Option ("dwgrad", "the diffusion-weighting gradient table, as stored in the header "
          "(i.e. without any interpretation, scaling of b-values, or normalisation of gradient vectors)")
    +   Option ("shells", "list the average b-value of each shell")
    +   Option ("shellcounts", "list the number of volumes in each shell");

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

void print_vox (const Header& header)
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
  std::vector<ssize_t> strides (Stride::get (header));
  Stride::symbolise (strides);
  for (size_t i = 0; i < header.ndim(); ++i) {
    if (i) buffer += " ";
    buffer += header.stride (i) ? str (strides[i]) : "?";
  }
  std::cout << buffer << "\n";
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






void run ()
{
  auto check_option_group = [](const App::OptionGroup& g) { for (auto o: g) if (get_options (o.id).size()) return true; return false; };

  bool export_grad = check_option_group (GradExportOptions);

  if (export_grad && argument.size() > 1)
    throw Exception ("can only export DW gradient table to file if a single input image is provided");

  std::unique_ptr<nlohmann::json> json (get_options ("json_export").size() ? new nlohmann::json : nullptr);

  if (get_options ("norealign").size())
    Header::do_not_realign_transform = true;

  const bool format      = get_options("format")        .size();
  const bool ndim        = get_options("ndim")          .size();
  const bool size        = get_options("size")          .size();
  const bool vox         = get_options("vox")           .size();
  const bool datatype    = get_options("datatype")      .size();
  const bool stride      = get_options("stride")        .size();
  const bool offset      = get_options("offset")        .size();
  const bool multiplier  = get_options("multiplier")    .size();
  const auto properties  = get_options("property");
  const bool transform   = get_options("transform")     .size();
  const bool dwgrad      = get_options("dwgrad")        .size();
  const bool shells      = get_options("shells")        .size();
  const bool shellcounts = get_options("shellcounts")   .size();
  const bool raw_dwgrad  = get_options("raw_dwgrad")    .size();

  const bool print_full_header = !(format || ndim || size || vox || datatype || stride ||
      offset || multiplier || properties.size() || transform || dwgrad || export_grad || shells || shellcounts);


  for (size_t i = 0; i < argument.size(); ++i) {
    auto header = Header::open (argument[i]);
    if (raw_dwgrad)
      DWI::set_DW_scheme (header, DWI::get_DW_scheme (header));
    else if (export_grad || check_option_group (GradImportOptions) || dwgrad || shells || shellcounts)
      DWI::set_DW_scheme (header, DWI::get_valid_DW_scheme (header, true));

    Eigen::IOFormat fmt(Eigen::FullPrecision, 0, ", ", "\n", "", "", "", "\n");
    if (format)     std::cout << header.format() << "\n";
    if (ndim)       std::cout << header.ndim() << "\n";
    if (size)       print_dimensions (header);
    if (vox)        print_vox (header);
    if (datatype)   std::cout << (header.datatype().specifier() ? header.datatype().specifier() : "invalid") << "\n";
    if (stride)     print_strides (header);
    if (offset)     std::cout << header.intensity_offset() << "\n";
    if (multiplier) std::cout << header.intensity_scale() << "\n";
    if (transform)  std::cout << header.transform().matrix().format(fmt);
    if (dwgrad)     std::cout << DWI::parse_DW_scheme (header) << "\n";
    if (shells || shellcounts)     {
      DWI::Shells dwshells (DWI::parse_DW_scheme (header));
      if (shells) {
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
    for (size_t n = 0; n < properties.size(); ++n)
      print_properties (header, properties[n][0]);

    DWI::export_grad_commandline (header);

    if (json) {
      for (const auto& kv : header.keyval()) {
        if (json->find (kv.first) == json->end()) {
          (*json)[kv.first] = kv.second;
        } else if ((*json)[kv.first] != kv.second) {
          // If the value for this key differs between images, turn the JSON entry into an array
          if ((*json)[kv.first].is_array())
            (*json)[kv.first].push_back (kv.second);
          else
            (*json)[kv.first] = { (*json)[kv.first], kv.second };
        }
      }
    }

    if (print_full_header)
      std::cout << header.description();
  }

  if (json) {
    auto opt = get_options ("json_export");
    assert (opt.size());
    File::OFStream out (opt[0][0]);
    out << json->dump(4) << "\n";
  }

}

