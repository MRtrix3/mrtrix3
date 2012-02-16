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

#include <fstream>

#include "app.h"
#include "progressbar.h"
#include "get_set.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"

using namespace MR;
using namespace MR::DWI;
using namespace App;

MRTRIX_APPLICATION

void usage ()
{
  DESCRIPTION
  + "apply a normalisation map to a tracks file.";

  ARGUMENTS
  + Argument ("tracks", "the input track file.").type_file ()
  + Argument ("transform", "the image containing the transform.").type_image_in()
  + Argument ("output", "the output fraction image").type_image_out();
}







void run ()
{
  Tractography::Properties properties;
  Tractography::Reader<> file;
  file.open (argument[0], properties);

  Image::Buffer<float> data (argument[1]);
  Image::Buffer<float>::voxel_type vox (data);

  Tractography::Writer<> writer;
  writer.create (argument[2], properties);

  std::vector<Point<float> > tck;

  Image::Interp::Linear<Image::Buffer<float>::voxel_type> interp (vox);
  ProgressBar progress ("normalising tracks...");

  while (file.next (tck)) {
    for (std::vector<Point<float> >::iterator i = tck.begin(); i != tck.end(); ++i) {
      interp.scanner (*i);
      vox[3] = 0;
      (*i) [0] = interp.value();
      vox[3] = 1;
      (*i) [1] = interp.value();
      vox[3] = 2;
      (*i) [2] = interp.value();
    }
    writer.append (tck);
    writer.total_count++;
    ++progress;
  }

}

