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
