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


    31-10-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * various optimisations to improve performance

*/

#include <fstream>

#include "app.h"
#include "progressbar.h"
#include "get_set.h"
#include "image/misc.h"
#include "image/voxel.h"
#include "image/interp.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"

using namespace MR; 
using namespace MR::DWI; 
using namespace std; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "convert a tracks file into a map of the fraction of tracks to enter each voxel.",
  NULL
};

ARGUMENTS = {
  Argument ("tracks", "track file", "the input track file.").type_file (),
  Argument ("template", "template image", "an image file to be used as a template for the output (the output image wil have the same voxel size and dimensions).").type_image_in(),
  Argument ("output", "output image", "the output fraction image").type_image_out(),
  Argument::End
};


const char* data_type_choices[] = { "FLOAT32", "FLOAT32LE", "FLOAT32BE", "FLOAT64", "FLOAT64LE", "FLOAT64BE", 
    "INT32", "UINT32", "INT32LE", "UINT32LE", "INT32BE", "UINT32BE", 
    "INT16", "UINT16", "INT16LE", "UINT16LE", "INT16BE", "UINT16BE", 
    "CFLOAT32", "CFLOAT32LE", "CFLOAT32BE", "CFLOAT64", "CFLOAT64LE", "CFLOAT64BE", 
    "INT8", "UINT8", "BIT", NULL };

OPTIONS = {
  Option ("count", "output fibre count", "produce an image of the fibre count through each voxel, rather than the fraction."),

  Option ("datatype", "data type", "specify output image data type.")
    .append (Argument ("spec", "specifier", "the data type specifier.").type_choice (data_type_choices)),

  Option::End
};





EXECUTE {

  Image::Header header (argument[1].get_image()->header());

  bool fibre_count = get_options(0).size(); // count

  std::vector<OptBase> opt = get_options (2); // datatype
  if (opt.size()) header.data_type.parse (data_type_choices[opt[0][0].get_int()]);
  else header.data_type = fibre_count ? DataType::UInt32 : DataType::Float32;

  Tractography::Properties properties;
  Tractography::Reader file;
  file.open (argument[0].get_string(), properties);

  header.axes.resize (3);
  header.comments.push_back (std::string ("track ") + (fibre_count ? "count" : "fraction") + " map");
  for (Tractography::Properties::iterator i = properties.begin(); i != properties.end(); ++i) 
    header.comments.push_back (i->first + ": " + i->second);
  for (std::vector<RefPtr<Tractography::ROI> >::iterator i = properties.roi.begin(); i != properties.roi.end(); ++i)
    header.comments.push_back ("ROI: " + (*i)->specification());
  for (std::vector<std::string>::iterator i = properties.comments.begin(); i != properties.comments.end(); ++i)
    header.comments.push_back ("comment: " + *i);

  size_t total_count = properties["total_count"].empty() ? 0 : to<size_t> (properties["total_count"]);
  size_t num_tracks = properties["count"].empty() ? 0 : to<size_t> (properties["count"]);
  size_t count = 0;

  off64_t voxel_count = Image::voxel_count (header, 3);
  int xmax = header.dim(0);
  int ymax = header.dim(1);
  int zmax = header.dim(2);

  size_t yskip = xmax;
  size_t zskip = xmax*ymax;

  size_t* countbuf = new size_t [voxel_count];
  memset (countbuf, 0, voxel_count*sizeof(size_t));

  voxel_count = (voxel_count+7)/8;
  uint8_t* visited = new uint8_t [voxel_count];
  std::vector<Point> tck;

  Image::Interp<Image::Header> interp (header);
  ProgressBar::init (num_tracks, "generating track count image...");

  while (file.next (tck)) {
    memset (visited, 0, voxel_count*sizeof(uint8_t));
    for (std::vector<Point>::iterator i = tck.begin(); i != tck.end(); ++i) {
      Point p (interp.R2P (*i));
      int x = round (p[0]);
      int y = round (p[1]);
      int z = round (p[2]);
      if (x >= 0 && y >= 0 && z >= 0 && x < xmax && y < ymax && z < zmax) {
        size_t offset = x + yskip*y + zskip*z;
        if (!get<bool>(visited, static_cast<size_t>(offset))) {
          put<bool> (true, visited, static_cast<size_t>(offset));
          countbuf[offset]++;
        }
      }
    }
    count++;
    ProgressBar::inc();
  }
  ProgressBar::done();

  delete [] visited;


  Image::Voxel pos (*argument[2].get_image (header));
  if (total_count > 0) count = total_count;
  float multiplier = 1.0;
  if (!fibre_count) multiplier /= float(count);

  ProgressBar::init (xmax*ymax*zmax, "writing track count image...");
  for (pos[2] = 0; pos[2] < zmax; pos[2]++) {
    for (pos[1] = 0; pos[1] < ymax; pos[1]++) {
      for (pos[0] = 0; pos[0] < xmax; pos[0]++) {
        pos.value() = multiplier * countbuf[pos[0] + yskip*pos[1] + zskip*pos[2]];
        ProgressBar::inc();
      }
    }
  }
  ProgressBar::done();

  delete [] countbuf;
}

