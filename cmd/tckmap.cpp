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
#include "image/voxel.h"
#include "dataset/misc.h"
#include "dataset/interp/linear.h"
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



class MapFunc {
  public:
    MapFunc (size_t* buffer, float multiplier, size_t yskip, size_t zskip) : buf (buffer), mult (multiplier), Y (yskip), Z (zskip) { }
    void operator() (Image::Voxel<float>& vox) { vox.value() = mult * buf[vox[0] + Y*vox[1] + Z*vox[2]]; }
  private:
    size_t* buf;
    float mult;
    size_t Y, Z;
};


EXECUTE {

  Image::Header header (argument[1].get_image());

  bool fibre_count = get_options(0).size(); // count

  std::vector<OptBase> opt = get_options (2); // datatype
  if (opt.size()) header.datatype().parse (data_type_choices[opt[0][0].get_int()]);
  else header.datatype() = fibre_count ? DataType::UInt32 : DataType::Float32;

  Tractography::Properties properties;
  Tractography::Reader file;
  file.open (argument[0].get_string(), properties);

  header.axes.ndim() = 3;
  header.comments.push_back (std::string ("track ") + (fibre_count ? "count" : "fraction") + " map");
  for (Tractography::Properties::iterator i = properties.begin(); i != properties.end(); ++i) 
    header.comments.push_back (i->first + ": " + i->second);
  for (std::multimap<std::string,std::string>::const_iterator i = properties.roi.begin(); i != properties.roi.end(); ++i)
    header.comments.push_back ("ROI: " + i->first + " " + i->second);
  for (std::vector<std::string>::iterator i = properties.comments.begin(); i != properties.comments.end(); ++i)
    header.comments.push_back ("comment: " + *i);

  size_t total_count = properties["total_count"].empty() ? 0 : to<size_t> (properties["total_count"]);
  size_t num_tracks = properties["count"].empty() ? 0 : to<size_t> (properties["count"]);
  size_t count = 0;

  off64_t voxel_count = DataSet::voxel_count (header, 3);
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

  DataSet::Interp::Linear<Image::Header> interp (header);
  ProgressBar::init (num_tracks, "generating track count image...");

  while (file.next (tck)) {
    memset (visited, 0, voxel_count*sizeof(uint8_t));
    for (std::vector<Point>::iterator i = tck.begin(); i != tck.end(); ++i) {
      Point p (interp.scanner2voxel (*i));
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


  if (total_count > 0) count = total_count;

  Image::Header map_header = argument[2].get_image (header);
  Image::Voxel<float> vox (map_header);

  float mult = fibre_count ? 1.0 : 1.0/float(count);
  DataSet::LoopInOrder loop (vox, "writing track count image...", 0, 3);
  for (loop.start (vox); loop.ok(); loop.next (vox)) 
    vox.value() = mult * countbuf[vox[0] + yskip*vox[1] + zskip*vox[2]]; 

  delete [] countbuf;
}

