/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 08/10/2012.

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
#include "app.h"
#include "progressbar.h"
#include "image/loop.h"
#include "image/voxel.h"
#include "image/buffer.h"
#include "image/transform.h"
#include "image/interp/linear.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/writer.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;
using namespace std;

typedef float value_type;


void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
  + "Map dixel values to track points and output data as a track associated scalar file.";


  ARGUMENTS
  + Argument ("dixel", "the input 4D dixel image").type_image_in()

  + Argument ("tracks", "The input tracks to map to dixels").type_file()

  + Argument ("scalars", "the tracks used to define orientations of "
                        "interest and spatial neighbourhoods.").type_file();

  OPTIONS
  + Option ("directions", "the directions associated with the dixel image (if not supplied within the dixel image header).")
  + Argument ("file", "a list of directions [az el] generated using the gendir command.").type_file();

}

int bin_direction (Math::Matrix<float> dirs, Point<float> dir) {
  int closest_dir = 0;
  float smallest_angle = M_PI;

  for (uint d = 0; d < dirs.rows(); d++) {
    float dot_prod = dirs(d,0) * dir[0] + dirs(d,1) * dir[1] + dirs(d,2) * dir[2];
    float angle = acos(dot_prod);
    if (angle < smallest_angle) {
      closest_dir = d;
      smallest_angle = angle;
    }
  }
  return closest_dir;
}

void write_scalars (std::vector<value_type>& values, DWI::Tractography::Writer<value_type>& writer) {
  std::vector<Point<float> > scalars;
  for (size_t i = 0; i < values.size(); i += 3) {
    Point<float> point;
    point[0] = values[i];
    if (i + 1 < values.size())
      point[1] = values[i + 1];
    else
      point[1] = NAN;
    if (i + 2 < values.size())
      point[2] = values[i + 2];
    else
      point[2] = NAN;
    scalars.push_back(point);
  }
  writer.append (scalars);
}


void run() {

  Image::Header header (argument[0]);
  Image::Buffer<value_type> dixel_buffer (header);
  Image::Buffer<value_type>::voxel_type dixel_voxel (dixel_buffer);

  Math::Matrix<value_type> directions;;
  Options opt = get_options ("directions");
  if (opt.size()) {
    directions.load(opt[0][0]);
  } else {
    if (!header["directions"].size())
      throw Exception ("no dixel directions have been specified.");
    std::vector<value_type> dir_vector;
    std::vector<std::string > lines = split (header["directions"], "\n", true);
    for (size_t l = 0; l < lines.size(); l++) {
      std::vector<value_type> v (parse_floats(lines[l]));
      dir_vector.insert (dir_vector.end(), v.begin(), v.end());
    }
    directions.resize (dir_vector.size() / 2, 2);
    for (size_t i = 0; i < dir_vector.size(); i += 2) {
      directions(i / 2, 0) = dir_vector[i];
      directions(i / 2, 1) = dir_vector[i + 1];
    }
  }

  Math::Matrix<value_type> vert (directions.rows(), 3);
  for (unsigned int d = 0; d < directions.rows(); d++) {
    vert(d,0) = sin (directions(d,1)) * cos (directions(d,0));
    vert(d,1) = sin (directions(d,1)) * sin (directions(d,0));
    vert(d,2) = cos (directions(d,1));
  }

  DWI::Tractography::Reader<value_type> tck_reader;
  DWI::Tractography::Properties properties;
  tck_reader.open (argument[1], properties);

  int num_tracks = properties["count"].empty() ? 0 : to<int> (properties["count"]);
  if (!num_tracks)
    throw Exception ("error with track count in input file");

  Image::Interp::Linear<Image::Buffer<value_type>::voxel_type> interp (dixel_voxel);
  DWI::Tractography::Writer<value_type> tck_writer (argument[2], properties);

  ProgressBar progress ("colouring tracks...", num_tracks);

  std::vector<Point<float> > tck;
  Point<float> tangent;
  while (tck_reader.next (tck)) {
    std::vector<float> stats (tck.size(), 0.0);
    for (size_t p = 0; p < tck.size(); ++p) {
      if (!interp.scanner (tck[p])) {
        if (p == 0)
          tangent = tck[p+1] - tck[p];
        else if (p == tck.size() - 1)
          tangent = tck[p] - tck[p-1];
        else
          tangent = tck[p+1] - tck[p-1];
        tangent.normalise();
        interp[3] = bin_direction (vert, tangent);
        stats[p] = interp.value();
      }
    }
    write_scalars(stats, tck_writer);
    progress++;
  }
  tck_reader.close();
  tck_writer.close();
}
