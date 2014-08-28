/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "command.h"
#include "image/header.h"
#include "file/nifti1_utils.h"
#include "math/matrix.h"
#include "math/LU.h"


using namespace MR;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "edit transformation matrices."

  + "This is needed in particular to convert the transformation matrix provided "
    "FSL's flirt command to a format usable in MRtrix.";

  ARGUMENTS
  + Argument ("input", "input transformation matrix").type_file_in ()
  + Argument ("from", "the image the input transformation matrix maps from").type_image_in ()
  + Argument ("to", "the image the input transformation matrix maps onto").type_image_in ()
  + Argument ("output", "the output transformation matrix.").type_file_out ();
}


Math::Matrix<float> get_flirt_transform (const Image::Header& header)
{
  std::vector<size_t> axes;
  Math::Matrix<float> nifti_transform = File::NIfTI::adjust_transform (header, axes);
  if (Math::LU::sgndet (nifti_transform) < 0.0) 
    return nifti_transform;

  Math::Matrix<float> coord_switch (4,4);
  coord_switch.identity();
  coord_switch(0,0) = -1.0f;
  coord_switch(0,3) = (header.dim(axes[0])-1) * header.vox(axes[0]);

  Math::Matrix<float> updated_transform;
  return Math::mult (updated_transform, nifti_transform, coord_switch);
}




void run ()
{
  Math::Matrix<float> flirt_transform;
  flirt_transform.load (argument[0]);

  Image::Header src_header (argument[1]);
  Math::Matrix<float> src_flirt_to_scanner = get_flirt_transform (src_header);

  Image::Header dest_header (argument[2]);
  Math::Matrix<float> dest_flirt_to_scanner = get_flirt_transform (dest_header);

  Math::Matrix<float> scanner_to_src_flirt = Math::LU::inv (src_flirt_to_scanner);

  Math::Matrix<float> scanner_to_transformed_dest_flirt;
  Math::mult (scanner_to_transformed_dest_flirt, flirt_transform, scanner_to_src_flirt);

  Math::Matrix<float> output;
  Math::mult (output, dest_flirt_to_scanner, scanner_to_transformed_dest_flirt);

  output.save (argument[3]);
}

