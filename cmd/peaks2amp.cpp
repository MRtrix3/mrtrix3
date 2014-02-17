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
#include "point.h"
#include "math/vector.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "image/loop.h"


using namespace std;
using namespace MR;
using namespace App;


void usage ()
{
  DESCRIPTION
  + "convert peak directions image to amplitudes.";

  ARGUMENTS
  + Argument ("directions", "the input directions image. Each volume corresponds to the x, y & z "
                             "component of each direction vector in turn.").type_image_in ()

  + Argument ("amplitudes", "the output amplitudes image.").type_image_out ();
}



void run ()
{
  Image::Buffer<float> dir_buf (argument[0]);
  Image::Buffer<float>::voxel_type dir_vox (dir_buf);

  Image::Header header (argument[0]);
  header.dim(3) = header.dim(3)/3;

  Image::Buffer<float> amp_buf (argument[1], header);
  Image::Buffer<float>::voxel_type amp_vox (amp_buf);

  Image::LoopInOrder loop (dir_vox, "converting directions to amplitudes...", 0, 3);

  for (loop.start (dir_vox, amp_vox); loop.ok(); loop.next (dir_vox, amp_vox)) {
    Math::Vector<float> dir (3);
    dir_vox[3] = 0;
    amp_vox[3] = 0;
    while (dir_vox[3] < dir_vox.dim(3)) {
      dir[0] = dir_vox.value(); ++dir_vox[3];
      dir[1] = dir_vox.value(); ++dir_vox[3];
      dir[2] = dir_vox.value(); ++dir_vox[3];

      float amplitude = 0.0;
      if (std::isfinite (dir[0]) && std::isfinite (dir[1]) && std::isfinite (dir[2]))
        amplitude = Math::norm (dir);

      amp_vox.value() = amplitude;
      ++amp_vox[3];
    }
  }
}
