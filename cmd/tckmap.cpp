/* Copyright (c) 2008-2017 the MRtrix3 contributors
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


#include <vector>
#include <set>

#include "command.h"
#include "progressbar.h"
#include "memory.h"

#include "image.h"
#include "thread_queue.h"

#include "dwi/gradient.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/weights.h"

#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/mapping.h"
#include "dwi/tractography/mapping/voxel.h"
#include "dwi/tractography/mapping/writer.h"

#include "dwi/tractography/mapping/gaussian/mapper.h"
#include "dwi/tractography/mapping/gaussian/voxel.h"




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

    + Option ("dec",
        "perform track mapping in directionally-encoded colour (DEC) space")

    + Option ("dixel",
        "map streamlines to dixels within each voxel; requires either a number of dixels "
        "(references an internal direction set), or a path to a text file containing a "
        "set of directions stored as azimuth/elevation pairs")
      + Argument ("path").type_text()

    + Option ("tod",
        "generate a Track Orientation Distribution (TOD) in each voxel; need to specify the maximum "
        "spherical harmonic degree lmax to use when generating Apodised Point Spread Functions")
      + Argument ("lmax").type_integer (2, 20);





const OptionGroup TWIOption = OptionGroup ("Options for the TWI image contrast properties")

  + Option ("contrast",
      "define the desired form of contrast for the output image\n"
      "Options are: " + join(contrasts, ", ") + " (default: tdi)")
    + Argument ("type").type_choice (contrasts)

  + Option ("image",
      "provide the scalar image map for generating images with 'scalar_map' / 'scalar_map_count' contrast, or the spherical harmonics image for 'fod_amp' contrast")
    + Argument ("image").type_image_in()

  + Option ("vector_file",
      "provide the vector data file for generating images with 'vector_file' contrast")
    + Argument ("path").type_file_in()

  + Option ("stat_vox",
      "define the statistic for choosing the final voxel intensities for a given contrast "
      "type given the individual values from the tracks passing through each voxel. \n"
      "Options are: " + join(voxel_statistics, ", ") + " (default: sum)")
    + Argument ("type").type_choice (voxel_statistics)

  + Option ("stat_tck",
      "define the statistic for choosing the contribution to be made by each streamline as a "
      "function of the samples taken along their lengths. \n"
      "Only has an effect for 'scalar_map', 'fod_amp' and 'curvature' contrast types. \n"
      "Options are: " + join(track_statistics, ", ") + " (default: mean)")
    + Argument ("type").type_choice (track_statistics)

  + Option ("fwhm_tck",
      "when using gaussian-smoothed per-track statistic, specify the "
      "desired full-width half-maximum of the Gaussian smoothing kernel (in mm)")
    + Argument ("value").type_float (1e-6)

  + Option ("map_zero",
      "if a streamline has zero contribution based on the contrast & statistic, typically it is not mapped; "
      "use this option to still contribute to the map even if this is the case "
      "(these non-contributing voxels can then influence the mean value in each voxel of the map)");




const OptionGroup MappingOption = OptionGroup ("Options for the streamline-to-voxel mapping mechanism")

  + Option ("upsample",
      "upsample the tracks by some ratio using Hermite interpolation before mappping\n"
      "(If omitted, an appropriate ratio will be determined automatically)")
    + Argument ("factor").type_integer (1)

  + Option ("precise",
      "use a more precise streamline mapping strategy, that accurately quantifies the length through each voxel "
      "(these lengths are then taken into account during TWI calculation)")

  + Option ("ends_only",
      "only map the streamline endpoints to the image");








void usage () {

AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au) and J-Donald Tournier (jdtournier@gmail.com)";

DESCRIPTION
  + "Use track data as a form of contrast for producing a high-resolution image."

  + "Note: if you run into limitations with RAM usage, make sure you output the "
    "results to a .mif file or .mih / .dat file pair - this will avoid the allocation "
    "of an additional buffer to store the output for write-out.";

REFERENCES 
  + "* For TDI or DEC TDI:\n"
  "Calamante, F.; Tournier, J.-D.; Jackson, G. D. & Connelly, A. " // Internal
  "Track-density imaging (TDI): Super-resolution white matter imaging using whole-brain track-density mapping. "
  "NeuroImage, 2010, 53, 1233-1243"

  + "* If using -contrast length and -stat_vox mean:\n"
  "Pannek, K.; Mathias, J. L.; Bigler, E. D.; Brown, G.; Taylor, J. D. & Rose, S. E. "
  "The average pathlength map: A diffusion MRI tractography-derived index for studying brain pathology. "
  "NeuroImage, 2011, 55, 133-141"

  + "* If using -dixel option with TDI contrast only:\n"
  "Smith, R.E., Tournier, J-D., Calamante, F., Connelly, A. " // Internal
  "A novel paradigm for automated segmentation of very large whole-brain probabilistic tractography data sets. "
  "In proc. ISMRM, 2011, 19, 673"

  + "* If using -dixel option with any other contrast:\n"
  "Pannek, K., Raffelt, D., Salvado, O., Rose, S. " // Internal
  "Incorporating directional information in diffusion tractography derived maps: angular track imaging (ATI). "
  "In Proc. ISMRM, 2012, 20, 1912"
  
  + "* If using -tod option:\n"
  "Dhollander, T., Emsell, L., Van Hecke, W., Maes, F., Sunaert, S., Suetens, P. " // Internal
  "Track Orientation Density Imaging (TODI) and Track Orientation Distribution (TOD) based tractography. "
  "NeuroImage, 2014, 94, 312-336"

  + "* If using other contrasts / statistics:\n"
  "Calamante, F.; Tournier, J.-D.; Smith, R. E. & Connelly, A. " // Internal
  "A generalised framework for super-resolution track-weighted imaging. "
  "NeuroImage, 2012, 59, 2494-2503"

  + "* If using -precise mapping option:\n"
  "Smith, R. E.; Tournier, J.-D.; Calamante, F. & Connelly, A. " // Internal
  "SIFT: Spherical-deconvolution informed filtering of tractograms. "
  "NeuroImage, 2013, 67, 298-312 (Appendix 3)";

ARGUMENTS
  + Argument ("tracks", "the input track file.").type_file_in()
  + Argument ("output", "the output track-weighted image").type_image_out();

OPTIONS
  + OutputHeaderOption
  + OutputDimOption
  + TWIOption
  + MappingOption
  + Tractography::TrackWeightsInOption;

}






MapWriterBase* make_writer (Header& H, const std::string& name, const vox_stat_t stat_vox, const writer_dim dim)
{
  MapWriterBase* writer = nullptr;
  const uint8_t dt = uint8_t(H.datatype()()) & DataType::Type;
  if (dt == DataType::Bit)
    writer = new MapWriter<bool>     (H, name, stat_vox, dim);
  else if (dt == DataType::UInt8)
    writer = new MapWriter<uint8_t>  (H, name, stat_vox, dim);
  else if (dt == DataType::UInt16)
    writer = new MapWriter<uint16_t> (H, name, stat_vox, dim);
  else if (dt == DataType::UInt32 || dt == DataType::UInt64)
    writer = new MapWriter<uint32_t> (H, name, stat_vox, dim);
  else if (dt == DataType::Float32 || dt == DataType::Float64)
    writer = new MapWriter<float>    (H, name, stat_vox, dim);
  else
    throw Exception ("Unsupported data type in image header");
  return writer;
}








DataType determine_datatype (const DataType current_dt, const contrast_t contrast, const DataType default_dt, const bool precise)
{
  if (current_dt == DataType::Undefined) {
    return default_dt;
  } else if ((default_dt.is_floating_point() || precise) && !current_dt.is_floating_point()) {
    WARN ("Cannot use non-floating-point datatype with " + str(Mapping::contrasts[contrast]) + " contrast" + (precise ? " and precise mapping" : "") + "; defaulting to " + str(default_dt.specifier()));
    return default_dt;
  } else {
    return current_dt;
  }
}



void run () {

  Tractography::Properties properties;
  Tractography::Reader<float> file (argument[0], properties);

  const size_t num_tracks = properties["count"].empty() ? 0 : to<size_t> (properties["count"]);

  vector<default_type> voxel_size = get_option_value ("vox", vector<default_type>());

  if (voxel_size.size() == 1)
    voxel_size.assign (3, voxel_size.front());
  else if (!voxel_size.empty() && voxel_size.size() != 3)
    throw Exception ("voxel size must either be a single isotropic value, or a list of 3 comma-separated voxel dimensions");

  if (!voxel_size.empty())
    INFO ("creating image with voxel dimensions [ " + str(voxel_size[0]) + " " + str(voxel_size[1]) + " " + str(voxel_size[2]) + " ]");

  Header header;
  auto opt = get_options ("template");
  if (opt.size()) {
    auto template_header = Header::open (opt[0][0]);
    header = template_header;
    header.keyval().clear();
    header.keyval()["twi_template"] = str(opt[0][0]);
    if (!voxel_size.empty())
      oversample_header (header, voxel_size);
  }
  else {
    if (voxel_size.empty())
      throw Exception ("please specify a template image and/or the desired voxel size");
    generate_header (header, argument[0], voxel_size);
  }

  if (header.ndim() > 3) {
    header.ndim() = 3;
    header.sanitise();
  }

  add_line (header.keyval()["comments"], "track-weighted image");
  header.keyval()["tck_source"] = std::string (argument[0]);

  opt = get_options ("contrast");
  const contrast_t contrast = opt.size() ? contrast_t(int(opt[0][0])) : TDI;

  opt = get_options ("stat_vox");
  vox_stat_t stat_vox = opt.size() ? vox_stat_t(int(opt[0][0])) : V_SUM;

  opt = get_options ("stat_tck");
  tck_stat_t stat_tck = opt.size() ? tck_stat_t(int(opt[0][0])) : T_MEAN;


  float gaussian_fwhm_tck = 0.0;
  opt = get_options ("fwhm_tck");
  if (opt.size()) {
    if (stat_tck != GAUSSIAN) {
      WARN ("Overriding per-track statistic to Gaussian as a full-width half-maximum has been provided.");
      stat_tck = GAUSSIAN;
    }
    gaussian_fwhm_tck = opt[0][0];
  } else if (stat_tck == GAUSSIAN) {
    throw Exception ("If using Gaussian per-streamline statistic, need to provide a full-width half-maximum for the Gaussian kernel using the -fwhm option");
  }


  // Determine the dimensionality of the output image
  writer_dim writer_type = GREYSCALE;

  opt = get_options ("dec");
  if (opt.size()) {
    writer_type = DEC;
    header.ndim() = 4;
    header.size (3) = 3;
    header.sanitise();
    Stride::set (header, Stride::contiguous_along_axis (3, header));
  }

  std::unique_ptr<Directions::FastLookupSet> dirs;
  opt = get_options ("dixel");
  if (opt.size()) {
    if (writer_type != GREYSCALE)
      throw Exception ("Options for setting output image dimensionality are mutually exclusive");
    writer_type = DIXEL;
    if (Path::exists (opt[0][0]))
      dirs.reset (new Directions::FastLookupSet (str(opt[0][0])));
    else
      dirs.reset (new Directions::FastLookupSet (to<size_t>(opt[0][0])));
    header.ndim() = 4;
    header.size(3) = dirs->size();
    header.sanitise();
    Stride::set (header, Stride::contiguous_along_axis (3, header));
    // Write directions to image header as diffusion encoding
    Eigen::MatrixXd grad (dirs->size(), 4);
    for (size_t row = 0; row != dirs->size(); ++row) {
      grad (row, 0) = ((*dirs)[row])[0];
      grad (row, 1) = ((*dirs)[row])[1];
      grad (row, 2) = ((*dirs)[row])[2];
      grad (row, 3) = 1.0f;
    }
    set_DW_scheme (header, grad);
  }

  opt = get_options ("tod");
  if (opt.size()) {
    if (writer_type != GREYSCALE)
      throw Exception ("Options for setting output image dimensionality are mutually exclusive");
    writer_type = TOD;
    const size_t lmax = opt[0][0];
    if (lmax % 2)
      throw Exception ("lmax for TODI must be an even number");
    header.ndim() = 4;
    header.size(3) = Math::SH::NforL (lmax);
    header.sanitise();
    Stride::set (header, Stride::contiguous_along_axis (3, header));
  }

  header.keyval()["twi_dimensionality"] = writer_dims[writer_type];


  // Deal with erroneous statistics & provide appropriate messages
  switch (contrast) {

    case TDI:
      if (stat_vox != V_SUM && stat_vox != V_MEAN) {
        WARN ("Cannot use voxel statistic other than 'sum' or 'mean' for TDI generation - ignoring");
        stat_vox = V_SUM;
      }
      if (stat_tck != T_MEAN)
        WARN ("Cannot use track statistic other than default for TDI generation - ignoring");
      stat_tck = T_MEAN;
      break;

    case LENGTH:
      if (stat_tck != T_MEAN)
        WARN ("Cannot use track statistic other than default for length-weighted TDI generation - ignoring");
      stat_tck = T_MEAN;
      break;

    case INVLENGTH:
      if (stat_tck != T_MEAN)
        WARN ("Cannot use track statistic other than default for inverse-length-weighted TDI generation - ignoring");
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

    case VECTOR_FILE:
      if (stat_tck != T_MEAN)
        WARN ("Cannot use track statistic other than default when providing contrast from an external data file - ignoring");
      stat_tck = T_MEAN;
      break;

    default:
      throw Exception ("Undefined contrast mechanism");

  }

  header.keyval()["twi_contrast"] = contrasts[contrast];
  header.keyval()["twi_vox_stat"] = voxel_statistics[stat_vox];
  header.keyval()["twi_tck_stat"] = track_statistics[stat_tck];


  // Figure out how the streamlines will be mapped
  const bool precise = get_options ("precise").size();
  header.keyval()["precise_mapping"] = precise ? "1" : "0";
  const bool ends_only = get_options ("ends_only").size();
  if (ends_only) {
    if (precise)
      throw Exception ("Options -precise and -ends_only are mutually exclusive");
    header.keyval()["endpoints_only"] = "1";
  }

  size_t upsample_ratio = 1;
  opt = get_options ("upsample");
  if (opt.size()) {
    if (ends_only) {
      WARN ("cannot use upsampling if only streamline endpoints are to be mapped");
    } else {
      upsample_ratio = opt[0][0];
      INFO ("track upsampling ratio manually set to " + str(upsample_ratio));
    }
  } else if (!ends_only) {
    // If accurately calculating the length through each voxel traversed, need a higher upsampling ratio
    //   (1/10th of the voxel size was found to give a good quantification of chordal length)
    // For all other applications, making the upsampled step size about 1/3rd of a voxel seems sufficient
    upsample_ratio = determine_upsample_ratio (header, properties, (precise ? 0.1 : 0.333));
    INFO ("track upsampling ratio automatically set to " + str(upsample_ratio));
  }


  // Get header datatype based on user input, or select an appropriate datatype automatically
  header.datatype() = DataType::Undefined;
  if (writer_type == DEC)
    header.datatype() = DataType::Float32;

  opt = get_options ("datatype");
  if (opt.size()) {
    if (writer_type == DEC || writer_type == TOD) {
      WARN ("Can't manually set datatype for " + str(Mapping::writer_dims[writer_type]) + " processing - overriding to Float32");
    } else {
      header.datatype() = DataType::parse (opt[0][0]);
    }
  }

  const bool have_weights = get_options ("tck_weights_in").size();
  if (have_weights && header.datatype().is_integer()) {
    WARN ("Can't use an integer type if streamline weights are provided; overriding to Float32");
    header.datatype() = DataType::Float32;
  }

  DataType default_datatype = DataType::Float32;
  if ((writer_type == GREYSCALE || writer_type == DIXEL) && !have_weights && ((!precise && contrast == TDI) || contrast == SCALAR_MAP_COUNT))
    default_datatype = DataType::UInt32;
  header.datatype() = determine_datatype (header.datatype(), contrast, default_datatype, precise);
  header.datatype().set_byte_order_native();


  // Whether or not to still ,ap streamlines even if the factor is zero
  //   (can still affect output image if voxel-wise statistic is mean)
  const bool map_zero = get_options ("map_zero").size();
  if (map_zero)
    header.keyval()["map_zero"] = "1";



  // Produce a useful INFO message
  std::string msg = std::string("Generating ") + Mapping::writer_dims[writer_type] + " image with ";
  switch (contrast) {
    case TDI:              msg += "density";                    break;
    case LENGTH:           msg += "length";                     break;
    case INVLENGTH:        msg += "inverse length";             break;
    case SCALAR_MAP:       msg += "scalar map";                 break;
    case SCALAR_MAP_COUNT: msg += "scalar-map-thresholded tdi"; break;
    case FOD_AMP:          msg += "FOD amplitude";              break;
    case CURVATURE:        msg += "curvature";                  break;
    case VECTOR_FILE:      msg += "external-file-based";        break;
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


  // Start initialising members for multi-threaded calculation
  TrackLoader loader (file, num_tracks);

  std::unique_ptr<TrackMapperTWI> mapper ((stat_tck == GAUSSIAN) ? (new Gaussian::TrackMapper (header, contrast)) : (new TrackMapperTWI (header, contrast, stat_tck)));
  mapper->set_upsample_ratio      (upsample_ratio);
  mapper->set_map_zero            (map_zero);
  mapper->set_use_precise_mapping (precise);
  mapper->set_map_ends_only       (ends_only);
  if (writer_type == DIXEL)
    mapper->create_dixel_plugin (*dirs);
  if (writer_type == TOD)
    mapper->create_tod_plugin (header.size(3));
  if (contrast == SCALAR_MAP || contrast == SCALAR_MAP_COUNT || contrast == FOD_AMP) {
    opt = get_options ("image");
    if (!opt.size()) {
      if (contrast == SCALAR_MAP || contrast == SCALAR_MAP_COUNT)
        throw Exception ("If using 'scalar_map' or 'scalar_map_count' contrast, must provide the relevant scalar image using -image option");
      else
        throw Exception ("If using 'fod_amp' contrast, must provide the relevant spherical harmonic image using -image option");
    }
    const std::string assoc_image (opt[0][0]);
    const auto H_assoc_image = Header::open (assoc_image);
    if (contrast == SCALAR_MAP || contrast == SCALAR_MAP_COUNT)
      mapper->add_scalar_image (assoc_image);
    else
      mapper->add_fod_image (assoc_image);
    header.keyval()["twi_assoc_image"] = Path::basename (assoc_image);
  } else if (contrast == VECTOR_FILE) {
    opt = get_options ("vector_file");
    if (!opt.size())
      throw Exception ("If using 'vector_file' contrast, must provide the relevant data file using the -vector_file option");
    const std::string path (opt[0][0]);
    mapper->add_vector_data (path);
    header.keyval()["twi_vector_file"] = Path::basename (path);
  }

  std::unique_ptr<MapWriterBase> writer;
  switch (writer_type) {
    case UNDEFINED: throw Exception ("Invalid TWI writer image dimensionality");
    case GREYSCALE: writer.reset (make_writer           (header, argument[1], stat_vox, GREYSCALE)); break;
    case DEC:       writer.reset (new MapWriter<float>  (header, argument[1], stat_vox, DEC));       break;
    case DIXEL:     writer.reset (make_writer           (header, argument[1], stat_vox, DIXEL));     break;
    case TOD:       writer.reset (new MapWriter<float>  (header, argument[1], stat_vox, TOD));       break;
  }

  // Finally get to do some number crunching!
  // Complete branch here for Gaussian track-wise statistic; it's a nightmare to manage, so am
  //   keeping the code as separate as possible
  if (stat_tck == GAUSSIAN) {
    Gaussian::TrackMapper* const mapper_ptr = dynamic_cast<Gaussian::TrackMapper*>(mapper.get());
    mapper_ptr->set_gaussian_FWHM (gaussian_fwhm_tck);
    switch (writer_type) {
      case UNDEFINED: throw Exception ("Invalid TWI writer image dimensionality");
      case GREYSCALE: Thread::run_queue (loader, Thread::batch (Tractography::Streamline<float>()), Thread::multi (*mapper_ptr), Thread::batch (Gaussian::SetVoxel()),    *writer); break;
      case DEC:       Thread::run_queue (loader, Thread::batch (Tractography::Streamline<float>()), Thread::multi (*mapper_ptr), Thread::batch (Gaussian::SetVoxelDEC()), *writer); break;
      case DIXEL:     Thread::run_queue (loader, Thread::batch (Tractography::Streamline<float>()), Thread::multi (*mapper_ptr), Thread::batch (Gaussian::SetDixel()),    *writer); break;
      case TOD:       Thread::run_queue (loader, Thread::batch (Tractography::Streamline<float>()), Thread::multi (*mapper_ptr), Thread::batch (Gaussian::SetVoxelTOD()), *writer); break;
    }
  } else {
    switch (writer_type) {
      case UNDEFINED: throw Exception ("Invalid TWI writer image dimensionality");
      case GREYSCALE: Thread::run_queue (loader, Thread::batch (Tractography::Streamline<float>()), Thread::multi (*mapper), Thread::batch (SetVoxel()),    *writer); break;
      case DEC:       Thread::run_queue (loader, Thread::batch (Tractography::Streamline<float>()), Thread::multi (*mapper), Thread::batch (SetVoxelDEC()), *writer); break;
      case DIXEL:     Thread::run_queue (loader, Thread::batch (Tractography::Streamline<float>()), Thread::multi (*mapper), Thread::batch (SetDixel()),    *writer); break;
      case TOD:       Thread::run_queue (loader, Thread::batch (Tractography::Streamline<float>()), Thread::multi (*mapper), Thread::batch (SetVoxelTOD()), *writer); break;
    }
  }

  writer->finalise();
}


