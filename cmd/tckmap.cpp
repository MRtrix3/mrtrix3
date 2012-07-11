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

#include "app.h"
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

#include "dwi/tractography/mapping/common.h"
#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/voxel.h"
#include "dwi/tractography/mapping/writer.h"



#define MAX_TRACKS_READ_FOR_HEADER 1000000
#define MAX_VOXEL_STEP_RATIO 0.333




MRTRIX_APPLICATION

using namespace MR;
using namespace App;

using namespace MR::DWI;
using namespace MR::DWI::Tractography;
using namespace MR::DWI::Tractography::Mapping;



void usage () {

AUTHOR = "Robert E. Smith (r.smith@brain.org.au) and J-Donald Tournier (d.tournier@brain.org.au)";

DESCRIPTION 
  + "Use track data as a form of contrast for producing a high-resolution image.";

ARGUMENTS 
  + Argument ("tracks", "the input track file.").type_file ()
  + Argument ("output", "the output track density image").type_image_out();

OPTIONS
  + Option ("template",
      "an image file to be used as a template for the output (the output image "
      "will have the same transform and field of view).")
    + Argument ("image").type_image_in()

  + Option ("vox", 
      "provide either an isotropic voxel size (in mm), or comma-separated list "
      "of 3 voxel dimensions.")
    + Argument ("size").type_sequence_float()

  + Option ("contrast",
      "define the desired form of contrast for the output image\n"
      "Options are: tdi, endpoint, length, invlength, scalar_map, scalar_map_count, fod_amp, curvature (default: tdi)")
    + Argument ("type").type_choice (contrasts)

  + Option ("image",
      "provide the scalar image map for generating images with 'scalar_map' contrast, or the spherical harmonics image for 'fod_amp' contrast")
    + Argument ("image").type_image_in()

  + Option ("stat_vox",
      "define the statistic for choosing the final voxel intensities for a given contrast "
      "type given the individual values from the tracks passing through each voxel\n"
      "Options are: sum, min, mean, max (default: sum)")
    + Argument ("type").type_choice (statistics)

  + Option ("stat_tck",
      "define the statistic for choosing the contribution to be made by each streamline as a "
      "function of the samples taken along their lengths\n"
      "Only has an effect for 'scalar_map', 'fod_amp' and 'curvature' contrast types\n"
      "Options are: sum, min, mean, median, max, gaussian, fmri_min, fmri_mean, fmri_max, fmri_prod (default: mean)")
    + Argument ("type").type_choice (statistics)

  + Option ("fwhm_tck",
      "when using gaussian-smoothed per-track statistic, specify the "
      "desired full-width half-maximum of the Gaussian smoothing kernel (in mm)")
    + Argument ("value").type_float (1e-6, 10.0, 1e6)

  + Option ("colour", "perform track mapping in directionally-encoded colour space")

  + Option ("datatype", 
      "specify output image data type.")
    + Argument ("spec").type_choice (DataType::identifiers)

  + Option ("resample", 
      "resample the tracks at regular intervals using Hermite interpolation\n"
      "(If omitted, an appropriate interpolation will be determined automatically)")
    + Argument ("factor").type_integer (1, 1, std::numeric_limits<int>::max())

  + Option ("dump", "dump the scratch buffer contents directly to a .mih / .dat file pair, rather than memory-mapping the output file")

  + Option ("map_zero", "if a streamline has zero contribution based on the contrast & statistic, typically it is not mapped; "
                        "use this option to still contribute to the map even if this is the case "
                        "(these non-contributing voxels can then influence the mean value in each voxel of the map)");
}




void generate_header (Image::Header& header, Tractography::Reader<float>& file, const std::vector<float>& voxel_size) 
{

  std::vector<Point<float> > tck;
  size_t track_counter = 0;

  Point<float> min_values ( INFINITY,  INFINITY,  INFINITY);
  Point<float> max_values (-INFINITY, -INFINITY, -INFINITY);

  {
    ProgressBar progress ("creating new template image...", 0);
    while (file.next(tck) && track_counter++ < MAX_TRACKS_READ_FOR_HEADER) {
      for (std::vector<Point<float> >::const_iterator i = tck.begin(); i != tck.end(); ++i) {
        min_values[0] = std::min (min_values[0], (*i)[0]);
        max_values[0] = std::max (max_values[0], (*i)[0]);
        min_values[1] = std::min (min_values[1], (*i)[1]);
        max_values[1] = std::max (max_values[1], (*i)[1]);
        min_values[2] = std::min (min_values[2], (*i)[2]);
        max_values[2] = std::max (max_values[2], (*i)[2]);
      }
      ++progress;
    }
  }

  min_values -= Point<float> (3.0*voxel_size[0], 3.0*voxel_size[1], 3.0*voxel_size[2]);
  max_values += Point<float> (3.0*voxel_size[0], 3.0*voxel_size[1], 3.0*voxel_size[2]);

  header.name() = "tckmap image header";
  header.set_ndim (3);

  for (size_t i = 0; i != 3; ++i) {
    header.dim(i) = Math::ceil((max_values[i] - min_values[i]) / voxel_size[i]);
    header.vox(i) = voxel_size[i];
    header.stride(i) = i+1;
    //header.set_units (i, Image::Axis::millimeters);
  }

  //header.set_description (0, Image::Axis::left_to_right);
  //header.set_description (1, Image::Axis::posterior_to_anterior);
  //header.set_description (2, Image::Axis::inferior_to_superior);

  Math::Matrix<float>& M (header.transform());
  M.allocate (4,4);
  M.identity();
  M(0,3) = min_values[0];
  M(1,3) = min_values[1];
  M(2,3) = min_values[2];

}





void oversample_header (Image::Header& header, const std::vector<float>& voxel_size) 
{
  inform ("oversampling header...");

  for (size_t i = 0; i != 3; ++i) {
    header.transform()(i, 3) += 0.5 * (voxel_size[i] - header.vox(i));
    header.dim(i) = Math::ceil(header.dim(i) * header.vox(i) / voxel_size[i]);
    header.vox(i) = voxel_size[i];
  }
}




template <class Cont>
MapWriterBase<Cont>* make_writer (Image::Header& H, const std::string& name, const bool dump, const stat_t stat_vox)
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
  Tractography::Reader<float> file;
  file.open (argument[0], properties);

  const size_t num_tracks = properties["count"]    .empty() ? 0   : to<size_t> (properties["count"]);
  const float  step_size  = properties["step_size"].empty() ? 1.0 : to<float>  (properties["step_size"]);

  std::vector<float> voxel_size;
  Options opt = get_options("vox");
  if (opt.size())
    voxel_size = opt[0][0];

  if (voxel_size.size() == 1)
    voxel_size.assign (3, voxel_size.front());
  else if (!voxel_size.empty() && voxel_size.size() != 3)
    throw Exception ("voxel size must either be a single isotropic value, or a list of 3 comma-separated voxel dimensions");

  if (!voxel_size.empty())
    inform ("creating image with voxel dimensions [ " + str(voxel_size[0]) + " " + str(voxel_size[1]) + " " + str(voxel_size[2]) + " ]");

  Image::Header header;
  opt = get_options ("template");
  if (opt.size()) {
    Image::Header template_header (opt[0][0]);
    header = template_header;
    if (!voxel_size.empty())
      oversample_header (header, voxel_size);
  }
  else {
    if (voxel_size.empty())
      throw Exception ("please specify either a template image or the desired voxel size");
    generate_header (header, file, voxel_size);
    file.close();
    file.open (argument[0], properties);
  }

  header.set_ndim (3);

  opt = get_options ("contrast");
  contrast_t contrast = opt.size() ? contrast_t(int(opt[0][0])) : TDI;

  opt = get_options ("stat_vox");
  stat_t stat_vox = opt.size() ? stat_t(int(opt[0][0])) : SUM;

  opt = get_options ("stat_tck");
  stat_t stat_tck = opt.size() ? stat_t(int(opt[0][0])) : MEAN;

  if (stat_vox == MEDIAN)
    throw Exception ("Sorry, can't calculate median values for each voxel - would take too much memory");

  float gaussian_fwhm_tck = 0.0, gaussian_denominator_tck = 0.0;
  opt = get_options ("fwhm_tck");
  if (opt.size()) {
    if (stat_tck != GAUSSIAN) {
      inform ("Overriding per-track statistic to Gaussian as a full-width half-maximum has been provided.");
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
    header.stride (3) = 1;
    header.stride (0) = 2;
    header.stride (1) = 3;
    header.stride (2) = 4;
  }

  // Deal with erroneous statistics & provide appropriate messages
  switch (contrast) {

    case TDI:
      if (stat_vox != SUM && stat_vox != MEAN) {
        inform ("Cannot use voxel statistic other than 'sum' or 'mean' for TDI generation - ignoring");
        stat_vox = SUM;
      }
      if (stat_tck != MEAN)
        inform ("Cannot use track statistic other than default for TDI generation - ignoring");
      stat_tck = MEAN;
      break;

    case ENDPOINT:
      if (stat_vox != SUM && stat_vox != MEAN) {
        inform ("Cannot use voxel statistic other than 'sum' or 'mean' for endpoint map generation - ignoring");
        stat_vox = SUM;
      }
      if (stat_tck != MEAN)
        inform ("Cannot use track statistic other than default for endpoint map generation - ignoring");
      stat_tck = MEAN;
      break;

    case LENGTH:
      if (stat_tck != MEAN)
        inform ("Cannot use track statistic other than default for length-weighted TDI generation - ignoring");
      stat_tck = MEAN;
      break;

    case INVLENGTH:
      if (stat_tck != MEAN)
        inform ("Cannot use track statistic other than default for inverse-length-weighted TDI generation - ignoring");
      stat_tck = MEAN;
      break;

    case SCALAR_MAP:
    case SCALAR_MAP_COUNT:
      break;

    case FOD_AMP:
      if (stat_tck == FMRI_MIN || stat_tck == FMRI_MEAN || stat_tck == FMRI_MAX || stat_tck == FMRI_PROD)
        throw Exception ("Sorry; can't use FMRI-based track statistics with FOD_AMP contrast");

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
      inform ("Can't manually set datatype for directionally-encoded colour processing - overriding to Float32");
    } else {
      header.datatype() = DataType::parse (opt[0][0]);
      manual_datatype = true;
    }
  }

  header.datatype().set_byte_order_native();

  for (Tractography::Properties::iterator i = properties.begin(); i != properties.end(); ++i)
    header.comments().push_back (i->first + ": " + i->second);
  for (std::multimap<std::string,std::string>::const_iterator i = properties.roi.begin(); i != properties.roi.end(); ++i)
    header.comments().push_back ("ROI: " + i->first + " " + i->second);
  for (std::vector<std::string>::iterator i = properties.comments.begin(); i != properties.comments.end(); ++i)
    header.comments().push_back ("comment: " + *i);

  size_t resample_factor;
  opt = get_options ("resample");
  if (opt.size()) {
    resample_factor = opt[0][0];
    inform ("track interpolation factor manually set to " + str(resample_factor));
  }
  else if (step_size) {
    resample_factor = Math::ceil<size_t> (step_size / (minvalue (header.vox(0), header.vox(1), header.vox(2)) * MAX_VOXEL_STEP_RATIO));
    inform ("track interpolation factor automatically set to " + str(resample_factor));
  }
  else {
    resample_factor = 1;
    inform ("track interpolation off; no track step size information in header");
  }

  const bool dump = get_options ("dump").size();

  Math::Matrix<float> interp_matrix (gen_interp_matrix<float> (resample_factor));

  std::string msg = str("Generating ") + (colour ? "colour " : "") + "image with ";
  switch (contrast) {
    case TDI:              msg += "density";                    break;
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
    case SUM:    msg += "summed";  break;
    case MIN:    msg += "minimum"; break;
    case MEAN:   msg += "mean";    break;
    case MAX:    msg += "maximum"; break;
    default:     msg += "ERROR";   break;
  }
  msg += " per-voxel statistic";
  if (contrast == SCALAR_MAP || contrast == SCALAR_MAP_COUNT || contrast == FOD_AMP || contrast == CURVATURE) {
    msg += " and ";
    switch (stat_tck) {
      case SUM:       msg += "summed";  break;
      case MIN:       msg += "minimum"; break;
      case MEAN:      msg += "mean";    break;
      case MEDIAN:    msg += "median";  break;
      case MAX:       msg += "maximum"; break;
      case GAUSSIAN:  msg += "gaussian (FWHM " + str (gaussian_fwhm_tck) + "mm)"; break;
      case FMRI_MIN:  msg += "fMRI (minimum)"; break;
      case FMRI_MEAN: msg += "fMRI (mean)"; break;
      case FMRI_MAX:  msg += "fMRI (maximum)"; break;
      case FMRI_PROD: msg += "fMRI (product)"; break;
      default:        msg += "ERROR";   break;
    }
    msg += " per-track statistic";
  }
  inform (msg);

  switch (contrast) {
    case TDI:              header.comments().push_back ("track density image"); break;
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
      MapWriterColour<SetVoxelDEC> writer (header, argument[1], dump, stat_vox);
      TrackMapperTWI <SetVoxelDEC> mapper (header, interp_matrix, map_zero, step_size, contrast, stat_tck);
      Thread::run_queue (loader, 1, TrackAndIndex(), mapper, 0, SetVoxelDEC(), writer, 1);
    } else {
      Ptr< MapWriterBase<SetVoxel> > writer (make_writer<SetVoxel> (header, argument[1], dump, stat_vox));
      TrackMapperTWI    <SetVoxel>   mapper (header, interp_matrix, map_zero, step_size, contrast, stat_tck);
      Thread::run_queue (loader, 1, TrackAndIndex(), mapper, 0, SetVoxel(), *writer, 1);
    }

  } else if (contrast == CURVATURE && stat_tck == GAUSSIAN) {

    if (!manual_datatype) {
      header.datatype() = DataType::Float32;
      header.datatype().set_byte_order_native();
    }

    if (colour) {
      MapWriterColour<SetVoxelDECFactor> writer (header, argument[1], dump, stat_vox);
      TrackMapperTWI <SetVoxelDECFactor> mapper (header, interp_matrix, map_zero, step_size, contrast, stat_tck, gaussian_denominator_tck);
      Thread::run_queue (loader, 1, TrackAndIndex(), mapper, 0, SetVoxelDECFactor(), writer, 1);
    } else {
      Ptr< MapWriterBase<SetVoxelFactor> > writer (make_writer<SetVoxelFactor> (header, argument[1], dump, stat_vox));
      TrackMapperTWI    <SetVoxelFactor>   mapper (header, interp_matrix, map_zero, step_size, contrast, stat_tck, gaussian_denominator_tck);
      Thread::run_queue (loader, 1, TrackAndIndex(), mapper, 0, SetVoxelFactor(), *writer, 1);
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
    if ((contrast == SCALAR_MAP || contrast == SCALAR_MAP_COUNT) && !(input_image.ndim() == 3 || (input_image.ndim() == 4 && input_image.dim(3) == 1)))
      throw Exception ("Use of 'scalar_map' contrast option requires a 3-dimensional image; your image is " + str(input_image.ndim()) + "D");
    if (contrast == FOD_AMP && input_image.ndim() != 4)
      throw Exception ("Use of 'fod_amp' contrast option requires a 4-dimensional image; your image is " + str(input_image.ndim()) + "D");

    if (!manual_datatype && (input_image.datatype() != DataType::Bit))
      header.datatype() = input_image.datatype();

    if (colour) {

      if (stat_tck == GAUSSIAN) {
        MapWriterColour     <SetVoxelDECFactor> writer (header, argument[1], dump, stat_vox);
        TrackMapperTWIImage <SetVoxelDECFactor> mapper (header, interp_matrix, map_zero, step_size, contrast, stat_tck, gaussian_denominator_tck, input_image);
        Thread::run_queue (loader, 1, TrackAndIndex(), mapper, 0, SetVoxelDECFactor(), writer, 1);
      } else {
        MapWriterColour     <SetVoxelDEC> writer (header, argument[1], dump, stat_vox);
        TrackMapperTWIImage <SetVoxelDEC> mapper (header, interp_matrix, map_zero, step_size, contrast, stat_tck, 0.0, input_image);
        Thread::run_queue (loader, 1, TrackAndIndex(), mapper, 0, SetVoxelDEC(), writer, 1);
      }

    } else {

      if (stat_tck == GAUSSIAN) {
        Ptr< MapWriterBase  <SetVoxelFactor> > writer (make_writer<SetVoxelFactor> (header, argument[1], dump, stat_vox));
        TrackMapperTWIImage <SetVoxelFactor>   mapper (header, interp_matrix, map_zero, step_size, contrast, stat_tck, gaussian_denominator_tck, input_image);
        Thread::run_queue (loader, 1, TrackAndIndex(), mapper, 0, SetVoxelFactor(), *writer, 1);
      } else {
        Ptr< MapWriterBase  <SetVoxel> > writer (make_writer<SetVoxel> (header, argument[1], dump, stat_vox));
        TrackMapperTWIImage <SetVoxel>   mapper (header, interp_matrix, map_zero, step_size, contrast, stat_tck, 0.0, input_image);
        Thread::run_queue (loader, 1, TrackAndIndex(), mapper, 0, SetVoxel(), *writer, 1);
      }

    }

  } else {
    throw Exception ("Undefined contrast mechanism for output image");
  }

}


