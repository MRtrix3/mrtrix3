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


#include "command.h"
#include "image/buffer_preload.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "image/filter/connected_components.h"
#include "math/matrix.h"

#include <cmath>


using namespace MR;
using namespace App;
using namespace std;


void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
  + "Connected component labelling of a binary input image of n-dimensions. Each connected "
    "component is labelled with a unique integer in increasing order of component size. \n\n"
    "By default this filter will assume that neighbours (along all dimensions) have contiguous "
    "indices (in space, time or whatever you feel like). Alternatively an adjacency matrix can "
    "be used to define neighbouring indices for a particular dimension. If the 4th dimension "
    "contains volumes that correspond to equally distributed directions, then the adjacency "
    "can be defined using a set of directions and an angular threshold. This permits clustering "
    "over both the spatial and orientations domains";

  ARGUMENTS
  + Argument ("image_in",
              "the binary image to be labelled").type_image_in()
  + Argument ("image_out",
              "the labelled output image").type_image_out();

  OPTIONS
  + Option ("directions", "the list of directions associated with each 3D volume, generated using the gendir command")
  + Argument ("file").type_file ()

  + Option ("angle", "the angular threshold used to define neighbouring directions (in degrees)")
  + Argument ("value").type_float (0, 15, 90)

  + Option ("ignore",
            "specify one or more axes to ignore. For example, to perform connected "
            "components separately on each 3D image within a 4D volume then ignore axis 3")
  + Argument ("axes").type_sequence_int()

  + Option ("largest",
            "only retain the largest component")

  + Option ("connectivity",
            "use 26 neighbourhood connectivity (Default: 6)");
}


void run ()
{
  Image::Header input_header (argument[0]);
  Image::Buffer<bool> input_data (input_header);
  Image::Buffer<bool>::voxel_type input_voxel (input_data);

  Image::Filter::ConnectedComponents connected_filter(input_voxel);
  Image::Header header (input_data);
  header.info() = connected_filter.info();
  Image::Buffer<bool> output_data (argument[1], header);
  Image::Buffer<bool>::voxel_type output_vox (output_data);

  Options opt = get_options ("angle");
  float angular_threshold = 15.0;
  if (opt.size())
    angular_threshold = opt[0][0];

  opt = get_options("directions");
  Math::Matrix<float> dirs;
  Math::Matrix<float> adjacency;
  if (opt.size()) {
    dirs.load (opt[0][0]);
    connected_filter.set_directions(dirs, angular_threshold);
  }

  opt = get_options ("ignore");
  std::vector<int> axes;
  if (opt.size()) {
    axes = opt[0][0];
    for (size_t i = 0; i < axes.size(); i++) {
      if (axes[i] >= static_cast<int> (input_header.ndim()) || axes[i] < 0)
        throw Exception ("axis supplied to option -ignore is out of bounds");
      connected_filter.set_ignore_dim (true, axes[i]);
    }
  }

  opt = get_options ("largest");
  if (opt.size())
    connected_filter.set_largest_only (true);

  opt = get_options ("connectivity");
  if (opt.size())
    connected_filter.set_26_connectivity(true);

  connected_filter(input_voxel, output_vox);
}

