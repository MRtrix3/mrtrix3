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

#include <map>
#include <string>
#include <vector>

#include "command.h"
#include "image/header.h"
#include "dwi/gradient.h"


using namespace MR;
using namespace App;



const OptionGroup ExtractOption = OptionGroup ("Options to print only specific information from the image header")

    + Option ("format", "image file format")
    + Option ("ndim", "number of image dimensions")
    + Option ("dimensions", "image dimensions along each axis")
    + Option ("vox", "voxel size along each image dimension")
    + Option ("datatype_long", "data type used for image data storage (long description)")
    + Option ("datatype_short", "data type used for image data storage (short specifier)")
    + Option ("stride", "data strides i.e. order and direction of axes data layout")
    + Option ("offset", "image intensity offset")
    + Option ("multiplier", "image intensity multiplier")
    + Option ("comments", "any comments embedded in the image header")
    + Option ("properties", "any text properties embedded in the image header")
    + Option ("transform", "the image transform")
    + Option ("dwgrad", "the diffusion-weighting gradient table, as stored in the header "
        "(i.e. without any interpretation, scaling of b-values, or normalisation of gradient vectors)")
    + Option ("shells", "list the average b-value of each shell")
    + Option ("shellcounts", "list the number of volumes in each shell");

const OptionGroup GradImportOptions = DWI::GradImportOptions();
const OptionGroup GradExportOptions = DWI::GradExportOptions();






void usage ()
{

  AUTHOR = "Robert Smith (r.smith@brain.org.au) and J-Donald Tournier (d.tournier@brain.org.au)";

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
    + Option ("norealign", 
        "do not realign transform to near-default RAS coordinate system (the "
        "default behaviour on image load). This is useful to inspect the transform "
        "and strides as they are actually stored in the header, rather than as "
        "MRtrix interprets them.") 
    + ExtractOption
    + GradImportOptions
    + Option ("validate", 
        "verify that DW scheme matches the image, and sanitise the information as would "
        "normally be done within an MRtrix application (i.e. scaling of b-value by gradient "
        "norm, normalisation of gradient vectors)")
    + GradExportOptions;

}







void print_dimensions (const Image::Header& header)
{
  std::string buffer;
  for (size_t i = 0; i < header.ndim(); ++i) {
    if (i) buffer += " ";
    buffer += str (header.dim (i));
  }
  std::cout << buffer << "\n";
}

void print_vox (const Image::Header& header)
{
  std::string buffer;
  for (size_t i = 0; i < header.ndim(); ++i) {
    if (i) buffer += " ";
    buffer += str (header.vox (i));
  }
  std::cout << buffer << "\n";
}

void print_strides (const Image::Header& header)
{
  std::string buffer;
  std::vector<ssize_t> strides (Image::Stride::get (header));
  Image::Stride::symbolise (strides);
  for (size_t i = 0; i < header.ndim(); ++i) {
    if (i) buffer += " ";
    buffer += header.stride (i) ? str (strides[i]) : "?";
  }
  std::cout << buffer << "\n";
}

void print_comments (const Image::Header& header)
{
  std::string buffer;
  for (std::vector<std::string>::const_iterator i = header.comments().begin(); i != header.comments().end(); ++i)
    buffer += *i + "\n";
  std::cout << buffer;
}

void print_properties (const Image::Header& header)
{
  std::string buffer;
  for (std::map<std::string, std::string>::const_iterator i = header.begin(); i != header.end(); ++i)
    buffer += i->first + ": " + i->second + "\n";
  std::cout << buffer;
}






void run ()
{
  auto check_option_group = [](const App::OptionGroup& g) { for (auto o: g) if (get_options (o.id).size()) return true; return false; };

  bool export_grad = check_option_group (GradExportOptions);

  if (export_grad && argument.size() > 1 )
    throw Exception ("can only export DW gradient table to file if a single input image is provided");

  if (get_options ("norealign").size())
    Image::Header::do_not_realign_transform = true;

  const bool format      = get_options("format")        .size();
  const bool ndim        = get_options("ndim")          .size();
  const bool dimensions  = get_options("dimensions")    .size();
  const bool vox         = get_options("vox")           .size();
  const bool dt_long     = get_options("datatype_long") .size();
  const bool dt_short    = get_options("datatype_short").size();
  const bool stride      = get_options("stride")        .size();
  const bool offset      = get_options("offset")        .size();
  const bool multiplier  = get_options("multiplier")    .size();
  const bool comments    = get_options("comments")      .size();
  const bool properties  = get_options("properties")    .size();
  const bool transform   = get_options("transform")     .size();
  const bool dwgrad      = get_options("dwgrad")        .size();
  const bool shells      = get_options("shells")        .size();
  const bool shellcounts = get_options("shellcounts")   .size();
  const bool validate    = get_options("validate")      .size();

  const bool print_full_header = !(format || ndim || dimensions || vox || dt_long || dt_short || stride || 
      offset || multiplier || comments || properties || transform || dwgrad || export_grad || shells || shellcounts);


  for (size_t i = 0; i < argument.size(); ++i) {
    Image::Header header (argument[i]);
    if (validate) 
      header.DW_scheme() = DWI::get_valid_DW_scheme<float> (header);
    else if (check_option_group (GradImportOptions)) 
      header.DW_scheme() = DWI::get_DW_scheme<float> (header);
    

    if (format)     std::cout << header.format() << "\n";
    if (ndim)       std::cout << header.ndim() << "\n";
    if (dimensions) print_dimensions (header);
    if (vox)        print_vox (header);
    if (dt_long)    std::cout << (header.datatype().description() ? header.datatype().description() : "invalid") << "\n";
    if (dt_short)   std::cout << (header.datatype().specifier() ? header.datatype().specifier() : "invalid") << "\n";
    if (stride)     print_strides (header);
    if (offset)     std::cout << header.intensity_offset() << "\n";
    if (multiplier) std::cout << header.intensity_scale() << "\n";
    if (comments)   print_comments (header);
    if (properties) print_properties (header);
    if (transform)  std::cout << header.transform();
    if (dwgrad)     std::cout << header.DW_scheme();
    if (shells || shellcounts)     { 
      DWI::Shells dwshells (header.DW_scheme()); 
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

    DWI::export_grad_commandline (header);

    if (print_full_header)
      std::cout << header.description();
  }

}

