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

#include <set>
#include "command.h"
#include "image.h"
#include "filter/resize.h"
#include "filter/reslice.h"
#include "interp/nearest.h"
#include "interp/linear.h"
#include "interp/cubic.h"
#include "interp/sinc.h"
#include "progressbar.h"
#include "algo/copy.h"
#include "adapter/regrid.h"


using namespace MR;
using namespace App;

const char* interp_choices[] = { "nearest", "linear", "cubic", "sinc", NULL };
const char* operation_choices[] = { "regrid", "crop", "pad", NULL };


void usage ()
{
  AUTHOR = "Max Pietsch (maximilian.pietsch@kcl.ac.uk) & David Raffelt (david.raffelt@florey.edu.au) & Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Modify the grid of an image without interpolation (cropping or padding) or by regridding to an image grid with modified orientation, location and or resolution. The image content remains in place in real world coordinates.";

  DESCRIPTION
  + "- regrid: This operation performs changes of the voxel grid that require interpolation of the image such as changing the resolution or location and orientation of the voxel grid. "
    "If the image is down-sampled, the appropriate smoothing is automatically applied using Gaussian smoothing unless nearest neighbour interpolation is selected or oversample is changed explicitly. The resolution can only be changed for spatial dimensions. "
  + "- crop: The image extent after cropping, can be specified either manually for each axis dimensions, or via a mask or reference image. "
    "The image can be cropped to the extent of a mask. "
    "This is useful for axially-acquired brain images, where the image size can be reduced by a factor of 2 by removing the empty space on either side of the brain. Note that cropping does not extend the image beyond the original FOV unless explicitly specified (via -crop_unbound or negative -axis extent)."
  + "- pad: Analogously to cropping, padding increases the FOV of an image without image interpolation. Pad and crop can be performed simultaneously by specifying signed specifier argument values to the -axis option."
  + "This command encapsulates and extends the functionality of the superseded commands 'mrpad', 'mrcrop' and 'mrresize'. Note the difference in -axis convention used for 'mrcrop' and 'mrpad' (see -axis option description).";

  EXAMPLES
  + Example ("Crop and pad the first axis",
             "mrgrid in.mif crop -axis 0 10,-5 out.mif",
             "This removes 10 voxels on the lower and pads with 5 on the upper bound, which is equivalent to "
             "padding with the negated specifier (mrgrid in.mif pad -axis 0 -10,5 out.mif).")

  + Example ("Right-pad the image to the number of voxels of a reference image",
             "mrgrid in.mif pad -as ref.mif -all_axes -axis 3 0,0 out.mif -fill nan",
             "This pads the image on the upper bound of all axes except for the volume dimension. "
             "The headers of in.mif and ref.mif are ignored and the output image uses NAN values to fill in voxels outside the original range of in.mif.")

  + Example ("Regrid and interpolate to match the voxel grid of a reference image",
             "mrgrid in.mif regrid -template ref.mif -scale 1,1,0.5 out.mif -fill nan",
             "The -template instructs to regrid in.mif to match the voxel grid of ref.mif (voxel size, grid orientation and voxel centres). "
             "The -scale option overwrites the voxel scaling factor yielding voxel sizes in the third dimension that are "
             "twice as coarse as those of the template image.");


  ARGUMENTS
  + Argument ("input", "input image to be regridded.").type_image_in ()
  + Argument ("operation", "the operation to be performed, one of: " + join(operation_choices, ", ") + ".").type_choice (operation_choices)
  + Argument ("output", "the output image.").type_image_out ();

  OPTIONS
  + OptionGroup ("Regridding options (involves image interpolation, applied to spatial axes only)")
    + Option   ("template", "match the input image grid (voxel spacing, image size, header transformation) to that of a reference image. "
                 "The image resolution relative to the template image can be changed with one of -size, -voxel, -scale." )
    + Argument ("image").type_image_in ()

    + Option   ("size", "define the size (number of voxels) in each spatial dimension for the output image. "
                        "This should be specified as a comma-separated list.")
    + Argument ("dims").type_sequence_int()

    + Option   ("voxel", "define the new voxel size for the output image. "
                "This can be specified either as a single value to be used for all spatial dimensions, "
                "or as a comma-separated list of the size for each voxel dimension.")
    + Argument ("size").type_sequence_float()

    + Option   ("scale", "scale the image resolution by the supplied factor. "
                "This can be specified either as a single value to be used for all dimensions, "
                "or as a comma-separated list of scale factors for each dimension.")
    + Argument ("factor").type_sequence_float()

    + Option ("interp", "set the interpolation method to use when reslicing (choices: nearest, linear, cubic, sinc. Default: cubic).")
    + Argument ("method").type_choice (interp_choices)

    + Option ("oversample",
        "set the amount of over-sampling (in the target space) to perform when regridding. This is particularly "
        "relevant when downsamping a high-resolution image to a low-resolution image, to avoid aliasing artefacts. "
        "This can consist of a single integer, or a comma-separated list of 3 integers if different oversampling "
        "factors are desired along the different axes. Default is determined from ratio of voxel dimensions (disabled "
        "for nearest-neighbour interpolation).")
    + Argument ("factor").type_sequence_int()

  + OptionGroup ("Pad and crop options (no image interpolation is performed, header transformation is adjusted)")
    + Option   ("as", "pad or crop the input image on the upper bound to match the specified reference image grid. "
                "This operation ignores differences in image transformation between input and reference image.")
    + Argument ("reference image").type_image_in ()

    + Option   ("uniform", "pad or crop the input image by a uniform number of voxels on all sides")
    + Argument ("number").type_integer ()

    + Option   ("mask",  "crop the input image according to the spatial extent of a mask image. "
                "The mask must share a common voxel grid with the input image but differences in image transformations are "
                "ignored. Note that even though only 3 dimensions are cropped when using a mask, the bounds are computed by "
                "checking the extent for all dimensions. "
                "Note that by default a gap of 1 voxel is left at all edges of the image to allow valid trilinear interpolation. "
                "This gap can be modified with the -uniform option but by default it does not extend beyond the FOV unless -crop_unbound is used.")
    + Argument ("image", "the mask image. ").type_image_in()

    + Option   ("crop_unbound", "Allow padding beyond the original FOV when cropping.")

    + Option   ("axis", "pad or crop the input image along the provided axis (defined by index). The specifier argument "
                "defines the number of voxels added or removed on the lower or upper end of the axis (-axis index delta_lower,delta_upper) "
                "or acts as a voxel selection range (-axis index start:stop). In both modes, values are relative to the input image "
                "(overriding all other extent-specifying options). Negative delta specifier values trigger the inverse operation "
                "(pad instead of crop and vice versa) and negative range specifier trigger padding. "
                "Note that the deprecated commands 'mrcrop' and 'mrpad' used range-based and delta-based -axis indices, respectively."
                ).allow_multiple()
    + Argument ("index").type_integer (0)
    + Argument ("spec").type_text()

    + Option   ("all_axes", "Crop or pad all, not just spatial axes.")

  + OptionGroup ("General options")
    + Option ("fill", "Use number as the out of bounds value. nan, inf and -inf are valid arguments. (Default: 0.0)")
    + Argument ("number").type_float ()

  + Stride::Options
  + DataType::options();
}


void run () {
  auto input_header = Header::open (argument[0]);

  const int op = argument[1];

  // Out of bounds value
  default_type out_of_bounds_value = 0.0;
  auto opt = get_options ("fill");
  if (opt.size())
    out_of_bounds_value = opt[0][0];

  if (op == 0) { // regrid
    INFO("operation: " + str(operation_choices[op]));
    Filter::Resize regrid_filter (input_header);
    regrid_filter.set_out_of_bounds_value(out_of_bounds_value);
    size_t resize_option_count = 0;
    size_t template_option_count = 0;

    int interp = 2;  // cubic
    opt = get_options ("interp");
    if (opt.size()) {
      interp = opt[0][0];
    }

    // over-sampling
    vector<uint32_t> oversample = Adapter::AutoOverSample;
    opt = get_options ("oversample");
    if (opt.size()) {
      oversample = parse_ints<uint32_t> (opt[0][0]);
    }

    Header template_header;
    opt = get_options ("template");
    if (opt.size()) {
      template_header = Header::open(opt[0][0]);
      if (template_header.ndim() < 3)
        throw Exception ("the template image requires at least 3 spatial dimensions");
      add_line (regrid_filter.keyval()["comments"], std::string ("regridded to template image \"" + template_header.name() + "\""));
      for (auto i=0; i<3; ++i) {
        regrid_filter.spacing(i) = template_header.spacing(i);
        regrid_filter.size(i) = template_header.size(i);
      }
      regrid_filter.set_transform(template_header.transform());
      ++template_option_count;
    }

    regrid_filter.set_interp_type (interp);
    regrid_filter.set_oversample (oversample);

    vector<default_type> scale;
    opt = get_options ("scale");
    if (opt.size()) {
      scale = parse_floats (opt[0][0]);
      if (scale.size() == 1)
        scale.resize (3, scale[0]);
      regrid_filter.set_scale_factor (scale);
      ++resize_option_count;
    }

    vector<uint32_t> image_size;
    opt = get_options ("size");
    if (opt.size()) {
      image_size = parse_ints<uint32_t> (opt[0][0]);
      regrid_filter.set_size (image_size);
      ++resize_option_count;
    }

    vector<default_type> voxel_size;
    opt = get_options ("voxel");
    if (opt.size()) {
      voxel_size = parse_floats (opt[0][0]);
      if (voxel_size.size() == 1)
        voxel_size.resize (3, voxel_size[0]);
      regrid_filter.set_voxel_size (voxel_size);
      ++resize_option_count;
    }

    if (!resize_option_count and !template_option_count)
      throw Exception ("please use either the -scale, -voxel, -resolution or -template option to regrid the image");
    if (resize_option_count > 1)
      throw Exception ("only a single method can be used to resize the image (image resolution, voxel size or scale factor)");

    Header output_header (regrid_filter);
    Stride::set_from_command_line (output_header);
    if (interp == 0)
      output_header.datatype() = DataType::from_command_line (input_header.datatype());
    else
      output_header.datatype() = DataType::from_command_line (DataType::from<float> ());
    auto output = Image<float>::create (argument[2], output_header);

    auto input = input_header.get_image<float>();
    regrid_filter (input, output);

  } else { // crop or pad
    const bool do_crop = op == 1;
    std::string message = do_crop ? "cropping image" : "padding image";
    INFO("operation: " + str(operation_choices[op]));

    if (get_options ("crop_unbound").size() && !do_crop)
      throw Exception("-crop_unbound only applies only to the crop operation");

    const size_t nd = get_options ("nd").size() ? input_header.ndim() : 3;

    vector<vector<ssize_t>> bounds (input_header.ndim(), vector<ssize_t> (2));
    for (size_t axis = 0; axis < input_header.ndim(); axis++) {
      bounds[axis][0] = 0;
      bounds[axis][1] = input_header.size (axis) - 1;
    }

    size_t crop_pad_option_count = 0;

    opt = get_options ("mask");
    if (opt.size()) {
      if (!do_crop) throw Exception("padding with -mask option is not supported");
      INFO("cropping to mask");
      ++crop_pad_option_count;
      auto mask = Image<bool>::open (opt[0][0]);
      check_dimensions (input_header, mask, 0, 3);

      for (size_t axis = 0; axis != 3; ++axis) {
        bounds[axis][0] = input_header.size (axis);
        bounds[axis][1] = 0;
      }

      struct BoundsCheck { NOMEMALIGN
        vector<vector<ssize_t>>& overall_bounds;
        vector<vector<ssize_t>> bounds;
        BoundsCheck (vector<vector<ssize_t>>& overall_bounds) : overall_bounds (overall_bounds), bounds (overall_bounds) { }
        ~BoundsCheck () {
          for (size_t axis = 0; axis != 3; ++axis) {
            overall_bounds[axis][0] = std::min (bounds[axis][0], overall_bounds[axis][0]);
            overall_bounds[axis][1] = std::max (bounds[axis][1], overall_bounds[axis][1]);
          }
        }
        void operator() (const decltype(mask)& m) {
          if (m.value()) {
            for (size_t axis = 0; axis != 3; ++axis) {
              bounds[axis][0] = std::min (bounds[axis][0], m.index(axis));
              bounds[axis][1] = std::max (bounds[axis][1], m.index(axis));
            }
          }
        }
      };

      ThreadedLoop (mask).run (BoundsCheck (bounds), mask);
      for (size_t axis = 0; axis != 3; ++axis) {
        if ((input_header.size (axis) - 1) != bounds[axis][1] or bounds[axis][0] != 0)
          INFO ("cropping to mask changes axis " + str(axis) + " extent from 0:" + str(input_header.size (axis) - 1) +
            " to " + str(bounds[axis][0]) + ":" + str(bounds[axis][1]));
      }
      if (!get_options ("uniform").size()) {
        INFO ("uniformly padding around mask by 1 voxel");
        // margin of 1 voxel around mask
        for (size_t axis = 0; axis != 3; ++axis) {
          bounds[axis][0] -= 1;
          bounds[axis][1] += 1;
        }
      }
    }

    opt = get_options ("as");
    if (opt.size()) {
      if (crop_pad_option_count)
        throw Exception (str(operation_choices[op]) + " can be performed using either a mask or a template image");
      ++crop_pad_option_count;

      Header template_header = Header::open(opt[0][0]);

      for (size_t axis = 0; axis != nd; ++axis) {
        if (axis >= template_header.ndim()) {
          if (do_crop)
            bounds[axis][1] = 0;
        }  else {
          if (do_crop)
            bounds[axis][1] = std::min (bounds[axis][1], template_header.size(axis) - 1);
          else
            bounds[axis][1] = std::max (bounds[axis][1], template_header.size(axis) - 1);
        }
      }
    }

    opt = get_options ("uniform");
    if (opt.size()) {
      ++crop_pad_option_count;
      ssize_t val = opt[0][0];
      INFO ("uniformly " + str(do_crop ? "cropping" : "padding") + " by " + str(val) + " voxels");
      for (size_t axis = 0; axis < nd; axis++) {
        bounds[axis][0] += do_crop ? val : -val;
        bounds[axis][1] += do_crop ? -val : val;
      }
    }

    if (do_crop && !get_options ("crop_unbound").size()) {
      opt = get_options ("axis");
      std::set<size_t> ignore;
      for (size_t i = 0; i != opt.size(); ++i)
        ignore.insert(opt[i][0]);
      for (size_t axis = 0; axis != 3; ++axis) {
        if (bounds[axis][0] < 0 || bounds[axis][1] > input_header.size(axis) - 1) {
          if (ignore.find(axis) == ignore.end())
            INFO ("operation: crop without -crop_unbound: restricting padding on axis " + str(axis) + " to valid FOV " +
            str(std::max<ssize_t> (0, bounds[axis][0])) + ":" + str(std::min<ssize_t> (bounds[axis][1], input_header.size(axis) - 1)));
          bounds[axis][0] = std::max<ssize_t> (0, bounds[axis][0]);
          bounds[axis][1] = std::min<ssize_t> (bounds[axis][1], input_header.size(axis) - 1);
        }
      }
    }

    opt = get_options ("axis"); // overrides image bounds set by other options
    for (size_t i = 0; i != opt.size(); ++i) {
      ++crop_pad_option_count;
      const size_t axis  = opt[i][0];
      if (axis  >= input_header.ndim())
        throw Exception ("-axis " + str(axis) + " larger than image dimensions (" + str(input_header.ndim()) + ")");
      std::string spec = str(opt[i][1]);
      std::string::size_type start = 0, end;
      end = spec.find_first_of(":", start);
      if (end == std::string::npos) { // spec = delta_lower,delta_upper
        vector<int> delta; // 0: not changed, > 0: pad, < 0: crop
        try { delta = parse_ints<int> (opt[i][1]); }
        catch (Exception& E) { Exception (E, "-axis " + str(axis) + ": can't parse delta specifier \"" + spec + "\""); }
        if (delta.size() != 2)
          throw Exception ("-axis " + str(axis) + ": can't parse delta specifier \"" + spec + "\"");
        bounds[axis][0] = do_crop ? delta[0] : -delta[0];
        bounds[axis][1] = input_header.size (axis) - 1 + (do_crop ? -delta[1] : delta[1]);
      } else { // spec = delta_lower:delta_upper
        std::string token (strip (spec.substr (start, end-start)));
        try { bounds[axis][0] = std::stoi(token); }
        catch (Exception& E) { throw Exception (E, "-axis " + str(axis) + ": can't parse integer sequence specifier \"" + spec + "\""); }
        token = strip (spec.substr (end+1));
        if (lowercase (token) == "end" || token.size() == 0)
          bounds[axis][1] = input_header.size(axis) - 1;
        else {
          try { bounds[axis][1]  = std::stoi(token); }
            catch (Exception& E) { throw Exception (E, "-axis " + str(axis) + ": can't parse integer sequence specifier \"" + spec + "\""); }
        }
      }
    }

    for (size_t axis = 0; axis != 3; ++axis) {
      if (bounds[axis][1] < bounds[axis][0])
        throw Exception ("axis " + str(axis) + " is empty: (" + str(bounds[axis][0]) + ":" + str(bounds[axis][1]) + ")");
    }

    if (crop_pad_option_count == 0)
      throw Exception ("no crop or pad option supplied");

    vector<ssize_t> from (input_header.ndim());
    vector<ssize_t> size (input_header.ndim());
    for (size_t axis = 0; axis < input_header.ndim(); axis++) {
      from[axis] = bounds[axis][0];
      size[axis] = bounds[axis][1] - from[axis] + 1;
    }

    size_t changed_axes = 0;
    for (size_t axis = 0; axis < nd; axis++) {
      if (bounds[axis][0] != 0 || input_header.size (axis) != size[axis]) {
        changed_axes++;
        CONSOLE("changing axis " + str(axis) + " extent from 0:" + str(input_header.size (axis) - 1) +
             " (n="+str(input_header.size (axis))+") to " + str(bounds[axis][0]) + ":" + str(bounds[axis][1]) +
             " (n=" + str(size[axis]) + ")");
      }
    }
    if (!changed_axes)
      WARN ("no axes were changed");

    auto input = input_header.get_image<float>();

    auto regridded = Adapter::make<Adapter::Regrid> (input, from, size, out_of_bounds_value);
    Header output_header (regridded);
    output_header.datatype() = DataType::from_command_line (DataType::from<float> ());
    Stride::set_from_command_line (output_header);

    auto output = Image<float>::create (argument[2], output_header);
    threaded_copy_with_progress_message (message.c_str(), regridded, output);
  }
}
