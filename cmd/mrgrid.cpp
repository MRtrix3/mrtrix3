/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


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
#include "adapter/subset.h"


using namespace MR;
using namespace App;

const char* interp_choices[] = { "nearest", "linear", "cubic", "sinc", NULL };
const char* operation_choices[] = { "resize", "crop", "pad", "match", NULL };

void usage ()
{
  AUTHOR = "Max Pietsch (maximilian.pietsch@kcl.ac.uk) & David Raffelt (david.raffelt@florey.edu.au) & Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Regrid an image either by resizing (defining the new image resolution, voxel size or a scale factor), "
             "cropping, padding, or matching the image grid of a reference image.";

  DESCRIPTION
  + "resize:\nNote that if the image is 4D, then only the first 3 dimensions can be resized."
  + "Also note that if the image is down-sampled, the appropriate smoothing is automatically applied using Gaussian smoothing.\n\n"
  + "crop:\nExtent of cropping can be determined using either manual setting of axis dimensions, or a computed mask image corresponding to the brain."
  + "If using a mask, a gap of 1 voxel will be left at all 6 edges of the image such that trilinear interpolation upon the resulting images is still valid."
  + "This is useful for axially-acquired brain images, where the image size can be reduced by a factor of 2 by removing the empty space on either side of the brain.\n\n"
  + "pad:\nZero pad an image to increase the FOV."
  + "match:\nProvide a reference image to match its image grid (voxel size, image size and orientation).";

  ARGUMENTS
  + Argument ("input", "input image to be regridded.").type_image_in ()
  + Argument ("operation", "the oparation to be performed, one of: " + join(operation_choices, ", ") + ".").type_choice (operation_choices)
  + Argument ("output", "the output image.").type_image_out ();

  OPTIONS
  + OptionGroup ("Resize options")
    + Option   ("size", "define the new image size for the output image. "
                "This should be specified as a comma-separated list.")
    + Argument ("dims").type_sequence_int()

    + Option   ("voxel", "define the new voxel size for the output image. "
                "This can be specified either as a single value to be used for all dimensions, "
                "or as a comma-separated list of the size for each voxel dimension.")
    + Argument ("size").type_sequence_float()

    + Option   ("scale", "scale the image resolution by the supplied factor. "
                "This can be specified either as a single value to be used for all dimensions, "
                "or as a comma-separated list of scale factors for each dimension.")
    + Argument ("factor").type_sequence_float()

    + Option   ("as", "resize the input image to match the specified template image voxel size.")
    + Argument ("image").type_image_in ()

    + Option   ("interp", "set the interpolation method to use when resizing (choices: nearest, linear, cubic, sinc. Default: cubic).")
    + Argument ("method").type_choice (interp_choices)

  + OptionGroup ("Crop options")
    + Option   ("mask",  "crop the input image according to the spatial extent of a mask image")
    + Argument ("image", "the mask image").type_image_in()

    + Option   ("as", "crop the input image if it exceeds the size of the template image grid.")
    + Argument ("image").type_image_in ()

    + Option   ("axis",  "crop the input image in the provided axis. Overrides mask and template options.").allow_multiple()
    + Argument ("index", "the index of the image axis to be cropped").type_integer (0)
    + Argument ("start", "the first voxel along this axis to be included in the output image").type_integer ()
    + Argument ("end",   "the last voxel along this axis to be included in the output image").type_integer ()

    + Option   ("nd", "Crop all, not just spatial axes.")

  + OptionGroup ("Pad options")
    + Option   ("as", "pad the input image to match the specified template image grid.")
    + Argument ("image").type_image_in ()

    + Option   ("uniform", "pad the input image by a uniform number of voxels on all sides (in 3D)")
    + Argument ("number").type_integer ()

    + Option   ("axis", "pad the input image along the provided axis (defined by index). Lower and upper define "
                "the number of voxels to add to the lower and upper bounds of the axis").allow_multiple()
    + Argument ("index").type_integer (0)
    + Argument ("lower").type_integer ()
    + Argument ("upper").type_integer ()

    + Option   ("value", "pad the input image with value instead of zero.")
    + Argument ("number").type_float (0.0)

    + Option   ("nd", "Pad all, not just spatial axes.")

  + OptionGroup ("Match options")
    + Option   ("template", "match the input image grid (voxel spacing, image size and header transformation) to that of the template image. mandatory option.")
    + Argument ("image").type_image_in ()

    + Option ("interp", "set the interpolation method to use when reslicing (choices: nearest, linear, cubic, sinc. Default: cubic).")
    + Argument ("method").type_choice (interp_choices)

    + Option ("oversample",
        "set the amount of over-sampling (in the target space) to perform when regridding. This is particularly "
        "relevant when downsamping a high-resolution image to a low-resolution image, to avoid aliasing artefacts. "
        "This can consist of a single integer, or a comma-separated list of 3 integers if different oversampling "
        "factors are desired along the different axes. Default is determined from ratio of voxel dimensions (disabled "
        "for nearest-neighbour interpolation).")
    + Argument ("factor").type_sequence_int()

    + Option ("nan", "Use NaN as the out of bounds value (Default: 0.0)")

  + Stride::Options
  + DataType::options();
}


void run () {
  auto input_header = Header::open (argument[0]);

  const int op = argument[1];

  if (op == 0) { // resize
    CONSOLE("operation: " + str(operation_choices[op]));
    Filter::Resize resize_filter (input_header);

    size_t resize_option_count = 0;

    vector<default_type> scale;
    auto opt = get_options ("scale");
    if (opt.size()) {
      scale = parse_floats (opt[0][0]);
      if (scale.size() == 1)
        scale.resize (3, scale[0]);
      resize_filter.set_scale_factor (scale);
      ++resize_option_count;
    }

    vector<default_type> voxel_size;
    opt = get_options ("voxel");
    if (opt.size()) {
      voxel_size = parse_floats (opt[0][0]);
      if (voxel_size.size() == 1)
        voxel_size.resize (3, voxel_size[0]);
      resize_filter.set_voxel_size (voxel_size);
      ++resize_option_count;
    }

    opt = get_options ("as");
    if (opt.size()) {
      Header template_header = Header::open (opt[0][0]);
      vector<default_type> voxel_spacing = {template_header.spacing(0), template_header.spacing(1), template_header.spacing(2)};
      resize_filter.set_voxel_size (voxel_spacing);
      ++resize_option_count;
    }

    vector<int> image_size;
    opt = get_options ("size");
    if (opt.size()) {
      image_size = parse_ints(opt[0][0]);
      resize_filter.set_size (image_size);
      ++resize_option_count;
    }

    int interp = 2;
    opt = get_options ("interp");
    if (opt.size()) {
      interp = opt[0][0];
      resize_filter.set_interp_type (interp);
    }

    if (!resize_option_count)
      throw Exception ("please use either the -scale, -voxel, or -resolution option to resize the image");
    if (resize_option_count != 1)
      throw Exception ("only a single method can be used to resize the image (image resolution, voxel size or scale factor)");

    Header output_header (resize_filter);
    output_header.datatype() = DataType::from_command_line (DataType::from<float> ());
    Stride::set_from_command_line (output_header);
    auto output = Image<float>::create (argument[2], output_header);

    auto input = input_header.get_image<float>();
    resize_filter (input, output);

  } else if (op == 1) { // crop
    CONSOLE("operation: " + str(operation_choices[op]));
    const size_t nd = get_options ("nd").size() ? input_header.ndim() : 3;

    vector<vector<ssize_t>> bounds (input_header.ndim(), vector<ssize_t> (2));
    for (size_t axis = 0; axis < input_header.ndim(); axis++) {
      bounds[axis][0] = 0;
      bounds[axis][1] = input_header.size (axis) - 1;
    }

    size_t crop_option_count = 0;

    auto opt = get_options ("mask");
    if (opt.size()) {
      ++crop_option_count;
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

      // Note that even though only 3 dimensions are cropped when using a mask, the bounds
      // are computed by checking the extent for all dimensions (for example a 4D AFD mask)
      ThreadedLoop (mask).run (BoundsCheck (bounds), mask);

      for (size_t axis = 0; axis != 3; ++axis) {
        if (bounds[axis][0] > bounds[axis][1])
          throw Exception ("mask image is empty; can't use to crop image");
        if (bounds[axis][0])
          --bounds[axis][0];
        if (bounds[axis][1] < mask.size (axis) - 1)
          ++bounds[axis][1];
      }
    }

    opt = get_options ("as");
    if (opt.size()) {
      if (crop_option_count)
        throw Exception ("Crop can be performed using either a mask or a template image");
      ++crop_option_count;

      Header template_header = Header::open(opt[0][0]);

      for (size_t axis = 0; axis != nd; ++axis) {
        if (axis >= template_header.ndim())
          bounds[axis][1] = 0;
        else
          bounds[axis][1] = std::min (bounds[axis][1], template_header.size(axis) - 1);
      }
    }

    opt = get_options ("axis");
    for (size_t i = 0; i != opt.size(); ++i) {
      ++crop_option_count;
      // Manual cropping of axis overrides mask image bounds
      const ssize_t axis  = opt[i][0];
      const ssize_t start = opt[i][1];
      const ssize_t end   = opt[i][2];
      if (axis  >= input_header.ndim())
        throw Exception ("axis " + str(axis) + " larger than image dimensions (" + str(input_header.ndim()) + ")");
      if (start < 0)
        throw Exception ("Start index " + str(start) + " supplied for axis " + str(axis) + " is out of bounds");
      if (end >= input_header.size(axis))
        throw Exception ("End index " + str(end) + " supplied for axis " + str(axis) + " is out of bounds");
      if (end < start)
        throw Exception  ("End index supplied for axis " + str(axis) + " is less than start index");
      bounds[axis][0] = start;
      bounds[axis][1] = end;
    }

    vector<size_t> from (input_header.ndim());
    vector<size_t> size (input_header.ndim());
    for (size_t axis = 0; axis < input_header.ndim(); axis++) {
      from[axis] = bounds[axis][0];
      size[axis] = bounds[axis][1] - from[axis] + 1;
    }

    if (crop_option_count == 0)
      throw Exception ("no crop option supplied");

    for (size_t axis = 0; axis < nd; axis++) {
      INFO("cropping axis " + str(axis) + " lower:" + str(bounds[axis][0]) + " upper:" + str(bounds[axis][1] - input_header.size(axis) + 1));
    }

    auto input = input_header.get_image<float>();

    auto cropped = Adapter::make<Adapter::Subset> (input, from, size);
    Header output_header (cropped);
    output_header.datatype() = DataType::from_command_line (DataType::from<float> ());
    Stride::set_from_command_line (output_header);
    auto output = Image<float>::create (argument[2], output_header);
    threaded_copy_with_progress_message ("cropping image", cropped, output);

  } else if (op == 2) { // pad
    CONSOLE("operation: " + str(operation_choices[op]));

    size_t pad_option_count = 0;

    Header template_header;
    auto opt = get_options ("as");
    if (opt.size()) {
      template_header = Header::open (opt[0][0]);
      if (template_header.ndim() > input_header.ndim()) {
        input_header.ndim() = get_options ("nd").size() ?
          size_t(template_header.ndim()) : std::min<size_t> (input_header.ndim(), 3);
      }
    }

    const size_t nd = get_options ("nd").size() ? input_header.ndim() : 3;

    ssize_t bounds[nd][2];
    for (size_t axis = 0; axis < nd; axis++) {
      bounds[axis][0] = 0;
      bounds[axis][1] = input_header.size(axis) - 1;
    }

    ssize_t padding[nd][2];
    for (size_t axis = 0; axis < nd; axis++) {
      padding[axis][0] = 0;
      padding[axis][1] = 0;
    }

    if (template_header.valid()) {
      ++pad_option_count;
      for (size_t axis = 0; axis < nd; axis++) {
        if (axis >= template_header.ndim())
          padding[axis][1] = 0;
        else
          padding[axis][1] = std::max<ssize_t>(0, template_header.size(axis) - input_header.size(axis));
      }
    }

    opt = get_options ("uniform");
    if (opt.size()) {
      ++pad_option_count;
      ssize_t pad = opt[0][0];
      for (size_t axis = 0; axis < nd; axis++) {
        padding[axis][0] += pad;
        padding[axis][1] += pad;
      }
    }

    opt = get_options ("axis");
    for (size_t i = 0; i != opt.size(); ++i) {
      ++pad_option_count;
      // Manual padding of axis overrides previous padding
      const size_t axis = opt[i][0];
      if (axis  >= input_header.ndim())
        throw Exception ("axis selected larger than image dimensions");
      padding[axis][0] = opt[i][1];
      padding[axis][1] = opt[i][2];
    }

    if (!pad_option_count)
      throw Exception("No padding option supplied.");

    float pad_value = 0.0;
    opt = get_options ("value");
    if (opt.size())
      pad_value = float(opt[0][0]);

    Header output_header (input_header);
    output_header.datatype() = DataType::from_command_line (DataType::from<float> ());
    Stride::set_from_command_line (output_header);

    auto output_transform = input_header.transform();
    for (size_t axis = 0; axis < 3; ++axis) {
      output_header.size (axis) = output_header.size(axis) + padding[axis][0] + padding[axis][1];
      output_transform (axis, 3) += (output_transform (axis, 0) * (bounds[0][0] - padding[0][0]) * input_header.spacing (0))
                                  + (output_transform (axis, 1) * (bounds[1][0] - padding[1][0]) * input_header.spacing (1))
                                  + (output_transform (axis, 2) * (bounds[2][0] - padding[2][0]) * input_header.spacing (2));
    }
    output_header.transform() = output_transform;
    for (size_t axis = 3; axis < nd; ++axis)
      output_header.size (axis) = output_header.size(axis) + padding[axis][0] + padding[axis][1];
    auto output = Image<float>::create (argument[2], output_header);

    for (size_t axis = 0; axis < nd; axis++) {
      INFO("padding axis " + str(axis) + " lower:" + str(padding[axis][0]) + " upper:" + str(padding[axis][1]));
    }

    auto input = input_header.get_image<float>();
    for (auto l = Loop ("padding image... ", output) (output); l; ++l) {
      bool in_bounds = true;
      for (size_t axis = 0; axis < nd; ++axis) {
        input.index(axis) = output.index(axis) - padding[axis][0];
        if (input.index(axis) < 0 || input.index(axis) >= input.size (axis))
          in_bounds = false;
      }
      if (input.ndim() > nd)
        input.index (nd) = output.index (nd);
      if (in_bounds)
        output.value() = input.value();
      else
        output.value() = pad_value;
    }

  } else if (op == 3) { // match
    CONSOLE("operation: " + str(operation_choices[op]));
    Header output_header (input_header);

    auto opt = get_options ("template");
    if (!opt.size())
      throw Exception ("you cannot use the match mode without the -template option");

    Header template_header = Header::open(opt[0][0]);
    for (size_t i = 0; i < 3; ++i) {
      output_header.size(i) = template_header.size(i);
      output_header.spacing(i) = template_header.spacing(i);
    }
    output_header.transform() = template_header.transform();
    add_line (output_header.keyval()["comments"], std::string ("regridded to template image \"" + template_header.name() + "\""));

    // Interpolator
    int interp = 2;  // cubic
    opt = get_options ("interp");
    if (opt.size()) {
      interp = opt[0][0];
    }
    if (interp == 0)
      output_header.datatype() = DataType::from_command_line (input_header.datatype());
    else
      output_header.datatype() = DataType::from_command_line (DataType::from<float> ());
    Stride::set_from_command_line (output_header);

    // over-sampling
    vector<int> oversample = Adapter::AutoOverSample;
    opt = get_options ("oversample");
    if (opt.size()) {
      oversample = opt[0][0];
      if (oversample.size() == 1)
        oversample.resize (3, oversample[0]);
      else if (oversample.size() != 3)
        throw Exception ("-oversample option requires either a single integer, or a comma-separated list of 3 integers");
      for (const auto x : oversample)
        if (x < 1)
          throw Exception ("-oversample factors must be positive integers");
    }
    else if (interp == 0)
      // default for nearest-neighbour is no oversampling
      oversample = { 1, 1, 1 };

    // Out of bounds value
    float out_of_bounds_value = 0.0;
    opt = get_options ("nan");
    if (opt.size()) {
      out_of_bounds_value = NAN;
    }

    auto output = Image<float>::create (argument[2], output_header).with_direct_io();

    transform_type linear_transform;
    linear_transform.setIdentity();

    auto input = input_header.get_image<float>().with_direct_io();
    switch (interp) {
      case 0:
        Filter::reslice<Interp::Nearest> (input, output, linear_transform, oversample, out_of_bounds_value);
        break;
      case 1:
        Filter::reslice<Interp::Linear> (input, output, linear_transform, oversample, out_of_bounds_value);
        break;
      case 2:
        Filter::reslice<Interp::Cubic> (input, output, linear_transform, oversample, out_of_bounds_value);
        break;
      case 3:
        Filter::reslice<Interp::Sinc> (input, output, linear_transform, oversample, out_of_bounds_value);
        break;
      default:
        assert (0);
        break;
    }
  }
}
