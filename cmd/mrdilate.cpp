#include "command.h"
#include "image/header.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "image/filter/dilate.h"


using namespace std;
using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (d.tournier@brain.org.au)";

  DESCRIPTION
  + "dilate a mask (i.e. binary) image";

  ARGUMENTS
  + Argument ("input", "input mask image to be dilated.").type_image_in ()
  + Argument ("output", "the output image.").type_image_out ();

  OPTIONS
  + Option ("npass", "the number of passes (default: 1).")
  + Argument ("number", "the number of passes.").type_integer (1, 1, 1000);
}

void run() {

  Image::Buffer<bool> input_data (argument[0]);
  Image::Buffer<bool>::voxel_type input_voxel (input_data);

  Image::Filter::Dilate dilate_filter (input_voxel);

  Image::Header output_header (input_data);
  output_header.info() = dilate_filter.info();
  Image::Buffer<bool> output_data (argument[1], output_header);
  Image::Buffer<bool>::voxel_type output_voxel (output_data);

  Options opt = get_options ("npass");
  dilate_filter.set_npass(static_cast<unsigned int> (opt.size() ? opt[0][0] : 1));

  dilate_filter (input_voxel, output_voxel);
}
