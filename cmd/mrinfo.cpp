/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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
    + Option ("dwgrad", "the diffusion-weighting gradient table");




const OptionGroup GradExportOption = OptionGroup ("Options to export the diffusion weighting gradient table to file")

    + Option ("export_grad_mrtrix", "export the diffusion-weighted gradient table to file in MRtrix format")
      + Argument ("path").type_text()

    + Option ("export_grad_fsl", "export the diffusion-weighted gradient table to files in FSL (bvecs / bvals) format")
      + Argument ("path").type_text();





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
    + ExtractOption
    + GradExportOption;

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

  const bool format     = get_options("format")        .size();
  const bool ndim       = get_options("ndim")          .size();
  const bool dimensions = get_options("dimensions")    .size();
  const bool vox        = get_options("vox")           .size();
  const bool dt_long    = get_options("datatype_long") .size();
  const bool dt_short   = get_options("datatype_short").size();
  const bool stride     = get_options("stride")        .size();
  const bool offset     = get_options("offset")        .size();
  const bool multiplier = get_options("multiplier")    .size();
  const bool comments   = get_options("comments")      .size();
  const bool properties = get_options("properties")    .size();
  const bool transform  = get_options("transform")     .size();
  const bool dwgrad     = get_options("dwgrad")        .size();

  Options opt = get_options ("export_grad_mrtrix");
  const std::string dw_out_mrtrix = opt.size() ? opt[0][0] : std::string();
  opt = get_options ("export_grad_fsl");
  const std::string dw_out_fsl    = opt.size() ? opt[0][0] : std::string();

  if ((dw_out_mrtrix.size() || dw_out_fsl.size()) && (argument.size() > 1))
    throw Exception ("Can only export gradient table information to file if a single input image is provided");

  const bool print_full_header = (!(format || ndim || dimensions || vox || dt_long || dt_short || stride
                                  || offset || multiplier || comments || properties || transform || dwgrad)
                                  && dw_out_mrtrix.empty() && dw_out_fsl.empty());


  for (size_t i = 0; i < argument.size(); ++i) {
    Image::Header header (argument[i]);

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

    if (dw_out_mrtrix.size()) {
      if (!header.DW_scheme().is_set())
        throw Exception ("no gradient information found within image \"" + header.name() + "\"");
      header.DW_scheme().save (dw_out_mrtrix);
    }

    if (dw_out_fsl.size()) {
      if (!header.DW_scheme().is_set())
        throw Exception ("no gradient information found within image \"" + header.name() + "\"");
      DWI::save_bvecs_bvals (header, dw_out_fsl);
    }

    if (print_full_header)
      std::cout << header.description();
  }

}

