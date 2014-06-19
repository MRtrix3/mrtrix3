/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith and J-Donald Tournier, 2011.

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

#include <fstream>
#include <vector>
#include <set>

#include "command.h"
#include "point.h"
#include "progressbar.h"

#include "image/buffer_preload.h"
#include "image/header.h"
#include "image/voxel.h"
#include "math/matrix.h"
#include "thread/exec.h"
#include "thread/queue.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/weights.h"

#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/mapping.h"
#include "dwi/tractography/mapping/voxel.h"
#include "dwi/tractography/mapping/writer.h"





using namespace MR;
using namespace App;

using namespace MR::DWI;
using namespace MR::DWI::Tractography;
using namespace MR::DWI::Tractography::Mapping;





const OptionGroup OutputHeaderOption = OptionGroup ("Options for the header of the output image")

  + Option ("template",
      "an image file to be used as a template for the output (the output image "
      "will have the same transform and field of view).")
    + Argument ("image").type_image_in()

  + Option ("vox",
      "provide either an isotropic voxel size (in mm), or comma-separated list "
      "of 3 voxel dimensions.")
    + Argument ("size").type_sequence_float()

  + Option ("datatype",
      "specify output image data type.")
    + Argument ("spec").type_choice (DataType::identifiers);





const OptionGroup OutputDimOption = OptionGroup ("Options for the dimensionality of the output image")

    + Option ("colour",
        "perform track mapping in directionally-encoded colour space");





const OptionGroup TWIOption = OptionGroup ("Options for the TWI image contrast properties")

  + Option ("contrast",
      "define the desired form of contrast for the output image\n"
      "Options are: tdi, precise_tdi, endpoint, length, invlength, scalar_map, scalar_map_count, fod_amp, curvature (default: tdi)")
    + Argument ("type").type_choice (contrasts)

  + Option ("image",
      "provide the scalar image map for generating images with 'scalar_map' contrast, or the spherical harmonics image for 'fod_amp' contrast")
    + Argument ("image").type_image_in()

  + Option ("stat_vox",
      "define the statistic for choosing the final voxel intensities for a given contrast "
      "type given the individual values from the tracks passing through each voxel\n"
      "Options are: sum, min, mean, max (default: sum)")
    + Argument ("type").type_choice (voxel_statistics)

  + Option ("stat_tck",
      "define the statistic for choosing the contribution to be made by each streamline as a "
      "function of the samples taken along their lengths\n"
      "Only has an effect for 'scalar_map', 'fod_amp' and 'curvature' contrast types\n"
      "Options are: sum, min, mean, max, median, mean_nonzero, gaussian, ends_min, ends_mean, ends_max, ends_prod (default: mean)")
    + Argument ("type").type_choice (track_statistics)

  + Option ("fwhm_tck",
      "when using gaussian-smoothed per-track statistic, specify the "
      "desired full-width half-maximum of the Gaussian smoothing kernel (in mm)")
    + Argument ("value").type_float (1e-6, 10.0, 1e6)

  + Option ("map_zero",
      "if a streamline has zero contribution based on the contrast & statistic, typically it is not mapped; "
      "use this option to still contribute to the map even if this is the case "
      "(these non-contributing voxels can then influence the mean value in each voxel of the map)");




const OptionGroup ExtraOption = OptionGroup ("Additional options for tckmap")

  + Option ("upsample",
      "upsample the tracks by some ratio using Hermite interpolation before mappping\n"
      "(If omitted, an appropriate ratio will be determined automatically)")
    + Argument ("factor").type_integer (1, 1, std::numeric_limits<int>::max())

  + Option ("dump",
      "dump the scratch buffer contents directly to a .mih / .dat file pair, "
      "rather than memory-mapping the output file");






void usage () {

AUTHOR = "Robert E. Smith (r.smith@brain.org.au) and J-Donald Tournier (d.tournier@brain.org.au)";

DESCRIPTION
  + "Use track data as a form of contrast for producing a high-resolution image.";

REFERENCES = "For TDI or DEC TDI:\n"
             "Calamante, F.; Tournier, J.-D.; Jackson, G. D. & Connelly, A. "
             "Track-density imaging (TDI): Super-resolution white matter imaging using whole-brain track-density mapping. "
             "NeuroImage, 2010, 53, 1233-1243\n\n"
             "If using -contrast length and -stat_vox mean:\n"
             "Pannek, K.; Mathias, J. L.; Bigler, E. D.; Brown, G.; Taylor, J. D. & Rose, S. E. "
             "The average pathlength map: A diffusion MRI tractography-derived index for studying brain pathology. "
             "NeuroImage, 2011, 55, 133-141\n\n"
             "If using other contrasts / statistics:\n"
             "Calamante, F.; Tournier, J.-D.; Smith, R. E. & Connelly, A. "
             "A generalised framework for super-resolution track-weighted imaging. "
             "NeuroImage, 2012, 59, 2494-2503";

ARGUMENTS
  + Argument ("tracks", "the input track file.").type_file ()
  + Argument ("output", "the output track-weighted image").type_image_out();

OPTIONS
  + OutputHeaderOption
  + OutputDimOption
  + TWIOption
  + ExtraOption
  + Tractography::TrackWeightsInOption;

}






template <class Cont>
MapWriterBase<Cont>* make_writer (Image::Header& H, const std::string& name, const bool dump, const vox_stat_t stat_vox)
{
    MapWriterBase<Cont>* writer = NULL;
    const uint8_t dt = H.datatype() ();
    if (dt & DataType::Signed) {
      if ((dt & DataType::Type) == DataType::UInt8)
        writer = new MapWriter<int8_t,   Cont> (H, name, dump, stat_vox);
      else if ((dt & DataType::Type) == DataType::UInt16)
        writer = new MapWriter<int16_t,  Cont> (H, name, dump, stat_vox);
      else if ((dt & DataType::Type) == DataType::UInt32)
        writer = new MapWriter<int32_t,  Cont> (H, name, dump, stat_vox);
      else
        throw Exception ("Unsupported data type in image header");
    } else {
      if ((dt & DataType::Type) == DataType::Bit)
        writer = new MapWriter<bool,     Cont> (H, name, dump, stat_vox);
      else if ((dt & DataType::Type) == DataType::UInt8)
        writer = new MapWriter<uint8_t,  Cont> (H, name, dump, stat_vox);
      else if ((dt & DataType::Type) == DataType::UInt16)
        writer = new MapWriter<uint16_t, Cont> (H, name, dump, stat_vox);
      else if ((dt & DataType::Type) == DataType::UInt32)
        writer = new MapWriter<uint32_t, Cont> (H, name, dump, stat_vox);
      else if ((dt & DataType::Type) == DataType::Float32)
        writer = new MapWriter<float,    Cont> (H, name, dump, stat_vox);
      else if ((dt & DataType::Type) == DataType::Float64)
        writer = new MapWriter<double,   Cont> (H, name, dump, stat_vox);
      else
        throw Exception ("Unsupported data type in image header");
    }
    return writer;
}



void run () {

  Tractography::Properties properties;
  Tractography::Reader<float> file (argument[0], properties);

  const size_t num_tracks = properties["count"].empty() ? 0 : to<size_t> (properties["count"]);

  float step_size = 0.0;
  if (properties.find ("output_step_size") != properties.end())
    step_size = to<float> (properties["output_step_size"]);
  else
    step_size = to<float> (properties["step_size"]);

  std::vector<float> voxel_size;
  Options opt = get_options("vox");
  if (opt.size())
    voxel_size = opt[0][0];

  if (voxel_size.size() == 1)
    voxel_size.assign (3, voxel_size.front());
  else if (!voxel_size.empty() && voxel_size.size() != 3)
    throw Exception ("voxel size must either be a single isotropic value, or a list of 3 comma-separated voxel dimensions");

  if (!voxel_size.empty())
    INFO ("creating image with voxel dimensions [ " + str(voxel_size[0]) + " " + str(voxel_size[1]) + " " + str(voxel_size[2]) + " ]");

  Image::Header header;
  opt = get_options ("template");
  if (opt.size()) {
    Image::Header template_header (opt[0][0]);
    header = template_header;
    header.comments().clear();
    if (!voxel_size.empty())
      oversample_header (header, voxel_size);
  }
  else {
    if (voxel_size.empty())
      throw Exception ("please specify either a template image or the desired voxel size");
    generate_header (header, argument[0], voxel_size);
  }

  header.set_ndim (3);

  opt = get_options ("contrast");
  contrast_t contrast = opt.size() ? contrast_t(int(opt[0][0])) : TDI;

  opt = get_options ("stat_vox");
  vox_stat_t stat_vox = opt.size() ? vox_stat_t(int(opt[0][0])) : V_SUM;

  opt = get_options ("stat_tck");
  tck_stat_t stat_tck = opt.size() ? tck_stat_t(int(opt[0][0])) : T_MEAN;

  float gaussian_fwhm_tck = 0.0, gaussian_denominator_tck = 0.0;
  opt = get_options ("fwhm_tck");
  if (opt.size()) {
    if (stat_tck != GAUSSIAN) {
      INFO ("Overriding per-track statistic to Gaussian as a full-width half-maximum has been provided.");
      stat_tck = GAUSSIAN;
    }
    gaussian_fwhm_tck = opt[0][0];
    const float gaussian_theta_tck = gaussian_fwhm_tck / (2.0 * sqrt (2.0 * log (2.0)));
    gaussian_denominator_tck = 2.0 * gaussian_theta_tck * gaussian_theta_tck;
  } else if (stat_tck == GAUSSIAN) {
    throw Exception ("If using Gaussian per-streamline statistic, need to provide a full-width half-maximum for the Gaussian kernel using the -fwhm option");
  }

  opt = get_options ("colour");
  const bool colour = opt.size();

  opt = get_options ("map_zero");
  const bool map_zero = opt.size();

  if (colour) {
    header.set_ndim (4);
    header.dim(3) = 3;
    //header.set_description (3, "directionally-encoded colour");
    Image::Stride::set (header, Image::Stride::contiguous_along_axis (3, header));
  }

  // Deal with erroneous statistics & provide appropriate messages
  switch (contrast) {

    case TDI:
      if (stat_vox != V_SUM && stat_vox != V_MEAN) {
        INFO ("Cannot use voxel statistic other than 'sum' or 'mean' for TDI generation - ignoring");
        stat_vox = V_SUM;
      }
      if (stat_tck != T_MEAN)
        INFO ("Cannot use track statistic other than default for TDI generation - ignoring");
      stat_tck = T_MEAN;
      break;

    case PRECISE_TDI:
      if (stat_vox != V_SUM) {
        INFO ("Cannot use voxel statistic other than 'sum' for precise TDI generation - ignoring");
        stat_vox = V_SUM;
      }
      if (stat_tck != T_MEAN)
        INFO ("Cannot use track statistic other than default for precise TDI generation - ignoring");
      stat_tck = T_MEAN;
      break;

    case ENDPOINT:
      if (stat_vox != V_SUM && stat_vox != V_MEAN) {
        INFO ("Cannot use voxel statistic other than 'sum' or 'mean' for endpoint map generation - ignoring");
        stat_vox = V_SUM;
      }
      if (stat_tck != T_MEAN)
        INFO ("Cannot use track statistic other than default for endpoint map generation - ignoring");
      stat_tck = T_MEAN;
      break;

    case LENGTH:
      if (stat_tck != T_MEAN)
        INFO ("Cannot use track statistic other than default for length-weighted TDI generation - ignoring");
      stat_tck = T_MEAN;
      break;

    case INVLENGTH:
      if (stat_tck != T_MEAN)
        INFO ("Cannot use track statistic other than default for inverse-length-weighted TDI generation - ignoring");
      stat_tck = T_MEAN;
      break;

    case SCALAR_MAP:
    case SCALAR_MAP_COUNT:
      break;

    case FOD_AMP:
      if (stat_tck == ENDS_MIN || stat_tck == ENDS_MEAN || stat_tck == ENDS_MAX || stat_tck == ENDS_PROD)
        throw Exception ("Can't use endpoint-based track-wise statistics with FOD_AMP contrast");
      break;

    case CURVATURE:
      break;

    default:
      throw Exception ("Undefined contrast mechanism");

  }


  opt = get_options ("datatype");
  bool manual_datatype = false;

  if (colour) {
    header.datatype() = DataType::Float32;
    manual_datatype = true;
  }

  if (opt.size()) {
    if (colour) {
      INFO ("Can't manually set datatype for directionally-encoded colour processing - overriding to Float32");
    } else {
      header.datatype() = DataType::parse (opt[0][0]);
      manual_datatype = true;
    }
  }

  if (get_options ("tck_weights_in").size() || (manual_datatype && header.datatype().is_integer())) {
    if ((manual_datatype && header.datatype().is_integer()))
      INFO ("Can't use an integer type if streamline weights are provided; overriding to Float32");
    header.datatype() = DataType::Float32;
    manual_datatype = true;
  }

  header.datatype().set_byte_order_native();

  for (Tractography::Properties::iterator i = properties.begin(); i != properties.end(); ++i)
    header.comments().push_back (i->first + ": " + i->second);
  for (std::multimap<std::string,std::string>::const_iterator i = properties.roi.begin(); i != properties.roi.end(); ++i)
    header.comments().push_back ("ROI: " + i->first + " " + i->second);
  for (std::vector<std::string>::iterator i = properties.comments.begin(); i != properties.comments.end(); ++i)
    header.comments().push_back ("comment: " + *i);

  size_t upsample_ratio = 1;
  opt = get_options ("upsample");
  if (opt.size()) {
    upsample_ratio = opt[0][0];
    INFO ("track upsampling ratio manually set to " + str(upsample_ratio));
  }
  else if (step_size && std::isfinite (step_size)) {
    // If accurately calculating the length through each voxel traversed, need a higher upsampling ratio
    //   (1/10th of the voxel size was found to give a good quantification of chordal length)
    // For all other applications, making the upsampled step size about 1/3rd of a voxel seems sufficient
    const float voxel_step_ratio = (contrast == PRECISE_TDI) ? 0.1 : 0.333;
    upsample_ratio = Math::ceil<size_t> (step_size / (minvalue (header.vox(0), header.vox(1), header.vox(2)) * voxel_step_ratio));
    INFO ("track upsampling ratio automatically set to " + str(upsample_ratio));
  }
  else {
    if (contrast != PRECISE_TDI)
      WARN ("track upsampling off; track step size information in header is absent or malformed");
  }

  const bool dump = get_options ("dump").size();
  if (dump && Path::has_suffix (argument[1], "mih"))
    throw Exception ("Option -dump only works when outputting to .mih image format");

  std::string msg = str("Generating ") + (colour ? "colour " : "") + "image with ";
  switch (contrast) {
    case TDI:              msg += "density";                    break;
    case PRECISE_TDI:      msg += "density (precise)";          break;
    case ENDPOINT:         msg += "endpoint density";           break;
    case LENGTH:           msg += "length";                     break;
    case INVLENGTH:        msg += "inverse length";             break;
    case SCALAR_MAP:       msg += "scalar map";                 break;
    case SCALAR_MAP_COUNT: msg += "scalar-map-thresholded tdi"; break;
    case FOD_AMP:          msg += "FOD amplitude";              break;
    case CURVATURE:        msg += "curvature";                  break;
    default:               msg += "ERROR";                      break;
  }
  msg += " contrast";
  if (contrast == SCALAR_MAP || contrast == SCALAR_MAP_COUNT || contrast == FOD_AMP || contrast == CURVATURE)
    msg += ", ";
  else
    msg += " and ";
  switch (stat_vox) {
    case V_SUM:  msg += "summed";  break;
    case V_MIN:  msg += "minimum"; break;
    case V_MEAN: msg += "mean";    break;
    case V_MAX:  msg += "maximum"; break;
    default:     msg += "ERROR";   break;
  }
  msg += " per-voxel statistic";
  if (contrast == SCALAR_MAP || contrast == SCALAR_MAP_COUNT || contrast == FOD_AMP || contrast == CURVATURE) {
    msg += " and ";
    switch (stat_tck) {
      case T_SUM:          msg += "summed";  break;
      case T_MIN:          msg += "minimum"; break;
      case T_MEAN:         msg += "mean";    break;
      case T_MAX:          msg += "maximum"; break;
      case T_MEDIAN:       msg += "median";  break;
      case T_MEAN_NONZERO: msg += "mean (nonzero)"; break;
      case GAUSSIAN:       msg += "gaussian (FWHM " + str (gaussian_fwhm_tck) + "mm)"; break;
      case ENDS_MIN:       msg += "endpoints (minimum)"; break;
      case ENDS_MEAN:      msg += "endpoints (mean)"; break;
      case ENDS_MAX:       msg += "endpoints (maximum)"; break;
      case ENDS_PROD:      msg += "endpoints (product)"; break;
      default:             msg += "ERROR";   break;
    }
    msg += " per-track statistic";
  }
  INFO (msg);

  switch (contrast) {
    case TDI:              header.comments().push_back ("track density image"); break;
    case PRECISE_TDI:      header.comments().push_back ("track density image (precise calculation)"); break;
    case ENDPOINT:         header.comments().push_back ("track endpoint density image"); break;
    case LENGTH:           header.comments().push_back ("track density image (weighted by track length)"); break;
    case INVLENGTH:        header.comments().push_back ("track density image (weighted by inverse track length)"); break;
    case SCALAR_MAP:       header.comments().push_back ("track-weighted image (using scalar image)"); break;
    case SCALAR_MAP_COUNT: header.comments().push_back ("track density image (using scalar image thresholding)"); break;
    case FOD_AMP:          header.comments().push_back ("track-weighted image (using FOD amplitude)"); break;
    case CURVATURE:        header.comments().push_back ("track-weighted image (using track curvature)"); break;
  }

  TrackLoader loader (file, num_tracks);

  // Use a branching IF instead of a switch statement to permit scope
  if (contrast == TDI || contrast == ENDPOINT || contrast == LENGTH || contrast == INVLENGTH || (contrast == CURVATURE && stat_tck != GAUSSIAN)) {

    if (!manual_datatype) {
      header.datatype() = (contrast == TDI || contrast == ENDPOINT) ? DataType::UInt32 : DataType::Float32;
      header.datatype().set_byte_order_native();
    }

    if (colour) {
      TrackMapperTWI <SetVoxelDEC> mapper (header, upsample_ratio, map_zero, step_size, contrast, stat_tck);
      MapWriterColour<SetVoxelDEC> writer (header, argument[1], dump, stat_vox);
      Thread::run_queue (loader, Tractography::Streamline<float>(), Thread::multi (mapper), SetVoxelDEC(), writer);
    } else {
      TrackMapperTWI    <SetVoxel>   mapper (header, upsample_ratio, map_zero, step_size, contrast, stat_tck);
      Ptr< MapWriterBase<SetVoxel> > writer (make_writer<SetVoxel> (header, argument[1], dump, stat_vox));
      Thread::run_queue (loader, Tractography::Streamline<float>(), Thread::multi (mapper), SetVoxel(), *writer);
    }

  } else if (contrast == PRECISE_TDI) {

    if (!manual_datatype) {
      header.datatype() = DataType::Float32;
      header.datatype().set_byte_order_native();
    }

    if (colour) {
      TrackMapperTWI <SetVoxelDir> mapper (header, upsample_ratio, map_zero, step_size, contrast, stat_tck);
      MapWriterColour<SetVoxelDir> writer (header, argument[1], dump, stat_vox);
      Thread::run_queue (loader, Tractography::Streamline<float>(), Thread::multi (mapper), SetVoxelDir(), writer);
    } else {
      TrackMapperTWI <       SetVoxelDir> mapper (header, upsample_ratio, map_zero, step_size, contrast, stat_tck);
      MapWriter      <float, SetVoxelDir> writer (header, argument[1], dump, stat_vox);
      Thread::run_queue (loader, Tractography::Streamline<float>(), Thread::multi (mapper), SetVoxelDir(), writer);
    }

  } else if (contrast == CURVATURE && stat_tck == GAUSSIAN) {

    if (!manual_datatype) {
      header.datatype() = DataType::Float32;
      header.datatype().set_byte_order_native();
    }

    if (colour) {
      TrackMapperTWI <SetVoxelDECFactor> mapper (header, upsample_ratio, map_zero, step_size, contrast, stat_tck, gaussian_denominator_tck);
      MapWriterColour<SetVoxelDECFactor> writer (header, argument[1], dump, stat_vox);
      Thread::run_queue (loader, Tractography::Streamline<float>(), Thread::multi (mapper), SetVoxelDECFactor(), writer);
    } else {
      TrackMapperTWI    <SetVoxelFactor>   mapper (header, upsample_ratio, map_zero, step_size, contrast, stat_tck, gaussian_denominator_tck);
      Ptr< MapWriterBase<SetVoxelFactor> > writer (make_writer<SetVoxelFactor> (header, argument[1], dump, stat_vox));
      Thread::run_queue (loader, Tractography::Streamline<float>(), Thread::multi (mapper), SetVoxelFactor(), *writer);
    }

  } else if (contrast == SCALAR_MAP || contrast == SCALAR_MAP_COUNT || contrast == FOD_AMP) {

    opt = get_options ("image");
    if (!opt.size()) {
      if (contrast == SCALAR_MAP || contrast == SCALAR_MAP_COUNT)
        throw Exception ("If using 'scalar_map' or 'scalar_map_count' contrast, must provide the relevant scalar image using -image option");
      else
        throw Exception ("If using 'fod_amp' contrast, must provide the relevant spherical harmonic image using -image option");
    }

    Image::BufferPreload<float> input_image (opt[0][0]);
    if ((contrast == SCALAR_MAP || contrast == SCALAR_MAP_COUNT)) {
      if (!(input_image.ndim() == 3 || (input_image.ndim() == 4 && input_image.dim(3) == 1)))
        throw Exception ("Use of 'scalar_map' contrast option requires a 3-dimensional image; your image is " + str(input_image.ndim()) + "D");
    }

    if (contrast == FOD_AMP && input_image.ndim() != 4)
      throw Exception ("Use of 'fod_amp' contrast option requires a 4-dimensional image; your image is " + str(input_image.ndim()) + "D");

    if (!manual_datatype && (input_image.datatype() != DataType::Bit))
      header.datatype() = input_image.datatype();

    if (colour) {

      if (stat_tck == GAUSSIAN) {
        TrackMapperTWIImage <SetVoxelDECFactor> mapper (header, upsample_ratio, map_zero, step_size, contrast, stat_tck, gaussian_denominator_tck, input_image);
        MapWriterColour     <SetVoxelDECFactor> writer (header, argument[1], dump, stat_vox);
        Thread::run_queue (loader, Tractography::Streamline<float>(), Thread::multi (mapper), SetVoxelDECFactor(), writer);
      } else {
        TrackMapperTWIImage <SetVoxelDEC> mapper (header, upsample_ratio, map_zero, step_size, contrast, stat_tck, 0.0, input_image);
        MapWriterColour     <SetVoxelDEC> writer (header, argument[1], dump, stat_vox);
        Thread::run_queue (loader, Tractography::Streamline<float>(), Thread::multi (mapper), SetVoxelDEC(), writer);
      }

    } else {

      if (stat_tck == GAUSSIAN) {
        TrackMapperTWIImage <SetVoxelFactor>   mapper (header, upsample_ratio, map_zero, step_size, contrast, stat_tck, gaussian_denominator_tck, input_image);
        Ptr< MapWriterBase  <SetVoxelFactor> > writer (make_writer<SetVoxelFactor> (header, argument[1], dump, stat_vox));
        Thread::run_queue (loader, Tractography::Streamline<float>(), Thread::multi (mapper), SetVoxelFactor(), *writer);
      } else {
        TrackMapperTWIImage <SetVoxel>   mapper (header, upsample_ratio, map_zero, step_size, contrast, stat_tck, 0.0, input_image);
        Ptr< MapWriterBase  <SetVoxel> > writer (make_writer<SetVoxel> (header, argument[1], dump, stat_vox));
        Thread::run_queue (loader, Tractography::Streamline<float>(), Thread::multi (mapper), SetVoxel(), *writer);
      }

    }

  } else {
    throw Exception ("Undefined contrast mechanism for output image");
  }

}


