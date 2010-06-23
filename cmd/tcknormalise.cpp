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
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"

using namespace MR; 
using namespace MR::DWI; 
using namespace std; 

SET_VERSION_DEFAULT;
SET_AUTHOR (NULL);
SET_COPYRIGHT (NULL);

DESCRIPTION = {
  "apply a normalisation map to a tracks file.",
  NULL
};

ARGUMENTS = {
  Argument ("tracks", "track file", "the input track file.").type_file (),
  Argument ("transform", "transform image", "the image containing the transform.").type_image_in(),
  Argument ("output", "output image", "the output fraction image").type_image_out(),
  Argument::End
};



OPTIONS = { Option::End };




EXECUTE {
  Tractography::Properties properties;
  Tractography::Reader file;
  file.open (argument[0].get_string(), properties);

  Image::Header header = argument[1].get_image();
  Image::Voxel<float> vox (header);

  Tractography::Writer writer;
  writer.create (argument[2].get_string(), properties);

  std::vector<Point> tck;

  DataSet::Interp::Linear<Image::Voxel<float> > interp (vox);
  ProgressBar progress ("normalising tracks...");

  while (file.next (tck)) {
    for (std::vector<Point>::iterator i = tck.begin(); i != tck.end(); ++i) {
      interp.scanner (*i);
      vox[3] = 0; (*i)[0] = interp.value();
      vox[3] = 1; (*i)[1] = interp.value();
      vox[3] = 2; (*i)[2] = interp.value();
    }
    writer.append (tck);
    writer.total_count++;
    ++progress;
  }

}

