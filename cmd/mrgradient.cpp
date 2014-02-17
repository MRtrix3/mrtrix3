#include "command.h"
#include "image/buffer.h"
#include "image/buffer_preload.h"
#include "image/buffer_scratch.h"
#include "image/voxel.h"
#include "image/filter/gaussian_smooth.h"
#include "image/filter/gradient.h"
#include "progressbar.h"


using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
  + "compute the image gradient along the x, y, and z axes of a 3D or 4D image."

  + "If the input file is 4D, then the output gradient image will be 5D. The 4th "
    "dimension will contain the x,y,z components of each input volume (defined by the 5th dimension).";

  ARGUMENTS
  + Argument ("input", "input image.").type_image_in ()
  + Argument ("output", "the output gradient image.").type_image_out ();

  OPTIONS
  + Option ("stdev", "the standard deviation of the Gaussian kernel used to "
            "smooth the input image (in mm). The image is smoothed to reduced large "
            "spurious gradients caused by noise. Use this option to override "
            "the default stdev of 1 voxel. This can be specified either as a single "
            "value to be used for all 3 axes, or as a comma-separated list of "
            "3 values, one for each axis.")
  + Argument ("sigma").type_sequence_float()

  + Option ("scanner", "compute the gradient with respect to the scanner coordinate "
            "frame of reference.");
}


void run () {

  Image::BufferPreload<float> input_data (argument[0]);
  Image::BufferPreload<float>::voxel_type input_voxel (input_data);

  Image::Filter::GaussianSmooth<> smooth_filter (input_voxel);
  std::vector<float> stdev;
  Options opt = get_options ("stdev");
  if (opt.size()) {
    stdev = parse_floats (opt[0][0]);
    for (size_t i = 0; i < stdev.size(); ++i)
      if (stdev[i] < 0.0)
        throw Exception ("the Gaussian stdev values cannot be negative");
    if (stdev.size() != 1 && stdev.size() != 3)
      throw Exception ("unexpected number of elements specified in Gaussian stdev");
  } else {
    stdev.resize (input_data.ndim(), 0.0);
    for (size_t dim = 0; dim < 3; dim++)
      stdev[dim] = input_data.vox (dim);
  }
  smooth_filter.set_stdev(stdev);

  Image::Filter::Gradient gradient_filter (input_voxel);

  opt = get_options("scanner");
  if(opt.size()) {
    gradient_filter.compute_wrt_scanner (true);
  } else {
    gradient_filter.compute_wrt_scanner (false);
  }

  Image::Header smooth_header (input_data);
  smooth_header.info() = smooth_filter.info();

  Image::BufferScratch<float> smoothed_data (smooth_header);
  Image::BufferScratch<float>::voxel_type smoothed_voxel (smoothed_data);

  Image::Header gradient_header (input_data);
  gradient_header.info() = gradient_filter.info();

  Image::Buffer<float> gradient_data (argument[1], gradient_header);
  Image::Buffer<float>::voxel_type gradient_voxel (gradient_data);

  ProgressBar progress("computing image gradient...");
  smooth_filter (input_voxel, smoothed_voxel);
  gradient_filter (smoothed_voxel, gradient_voxel);
}
