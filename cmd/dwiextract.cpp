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
#include "progressbar.h"
#include "image/voxel.h"
#include "dwi/gradient.h"
#include "image/loop.h"
#include "image/buffer.h"
#include "image/buffer_preload.h"


using namespace MR;
using namespace App;
using namespace std;

typedef float value_type;

void usage ()
{

  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
    + "Extract either diffusion-weighted volumes or b=0 volumes from an image containing both";

  ARGUMENTS
    + Argument ("input", "the input DW image.").type_image_in ()
    + Argument ("output", "the output image (diffusion-weighted volumes by default.").type_image_out ();

  OPTIONS
    + Option ("bzero", "output b=0 volumes instead of the diffusion weighted volumes.")
    + DWI::GradOption;
}

void run() {
  std::vector<ssize_t> strides (4, 0);
  strides[3] = 1;
  Image::BufferPreload<float> data_in (argument[0], strides);
  Image::BufferPreload<float>::voxel_type voxel_in (data_in);

  Math::Matrix<value_type> grad (DWI::get_DW_scheme<float> (data_in));
  std::vector<int> bzeros, dwis;
  DWI::guess_DW_directions (dwis, bzeros, grad);
  INFO ("found " + str(dwis.size()) + " diffusion-weighted directions");

  Image::Header header (data_in);
  Options opt = get_options ("bzero");
  if (opt.size()) {
    if (!bzeros.size())
      throw Exception ("no b=0 images found in image \"" + data_in.name() + "\"");
    if (bzeros.size() == 1)
      header.set_ndim(3);
    else
      header.dim(3) = bzeros.size();
    header.DW_scheme().clear();
  } else {
    header.dim(3) = dwis.size();
    Math::Matrix<value_type> dwi_grad (dwis.size(), 4);
    for (size_t dir = 0; dir < dwis.size(); dir++)
      dwi_grad.row(dir)= grad.row(dwis[dir]);
    header.DW_scheme() = dwi_grad;
  }

  Image::Buffer<value_type> data_out(argument[1], header);
  Image::Buffer<value_type>::voxel_type voxel_out (data_out);

  Image::Loop outer ("extracting volumes...", 0, 3 );

  if (opt.size()) {
    for (outer.start (voxel_out, voxel_in); outer.ok(); outer.next (voxel_out, voxel_in)) {
      for (size_t i = 0; i < bzeros.size(); i++) {
        voxel_in[3] = bzeros[i];
        if (bzeros.size() > 1)
          voxel_out[3] = i;
        voxel_out.value() = voxel_in.value();
      }
    }
  } else {
    for (outer.start (voxel_out, voxel_in); outer.ok(); outer.next (voxel_out, voxel_in)) {
      for (size_t i = 0; i < dwis.size(); i++) {
        voxel_in[3] = dwis[i];
        if (dwis.size() > 1)
          voxel_out[3] = i;
        voxel_out.value() = voxel_in.value();
      }
    }
  }
}
