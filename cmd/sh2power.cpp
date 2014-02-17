#include "command.h"
#include "progressbar.h"
#include "image/voxel.h"
#include "image/buffer.h"
#include "math/matrix.h"
#include "math/SH.h"
#include "image/loop.h"


using namespace MR;
using namespace App;

void usage () {
  DESCRIPTION
    + "compute the power contained within each harmonic degree.";

  ARGUMENTS
    + Argument ("SH", "the input spherical harmonics coefficients image.").type_image_in ()
    + Argument ("power", "the output power image.").type_image_out ();
}


void run () {
  Image::Buffer<float> SH_data (argument[0]);
  Image::Header power_header (SH_data);

  if (power_header.ndim() != 4)
    throw Exception ("SH image should contain 4 dimensions");


  int lmax = Math::SH::LforN (SH_data.dim (3));
  INFO ("calculating spherical harmonic power up to degree " + str (lmax));

  power_header.dim (3) = 1 + lmax/2;
  power_header.datatype() = DataType::Float32;

  Image::Buffer<float>::voxel_type SH (SH_data);

  Image::Buffer<float> power_data (argument[1], power_header);
  Image::Buffer<float>::voxel_type P (power_data);

  Image::LoopInOrder loop (P, "calculating SH power...", 0, 3);
  for (loop.start (P, SH); loop.ok(); loop.next (P, SH)) {
    P[3] = 0;
    for (int l = 0; l <= lmax; l+=2) {
      float power = 0.0;
      for (int m = -l; m <= l; ++m) {
        SH[3] = Math::SH::index (l, m);
        float val = SH.value();
#ifdef USE_NON_ORTHONORMAL_SH_BASIS
        if (m != 0) 
          val *= M_SQRT1_2;
#endif
        power += Math::pow2 (val);
      }
      P.value() = power / float (2*l+1);
      ++P[3];
    }
  }

}
