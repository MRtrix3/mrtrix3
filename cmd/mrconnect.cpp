/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by David Raffelt, 29 May 2012

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
#include "image/buffer_preload.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "image/filter/connected_components.h"
#include "math/matrix.h"

#include <cmath>

MRTRIX_APPLICATION

using namespace MR;
using namespace App;
using namespace std;


void usage ()
{
  AUTHOR = "David Raffelt (d.raffelt@brain.org.au)";

  DESCRIPTION
  + "Connected component labelling of a binary input image of n-dimensions. Each connected "
    "component is labelled with a unique integer in increasing order of component size. \n\n"
    "By default this filter will assume that neighbours (along all dimensions) have contiguous "
    "indices (in space, time or whatever you feel like). Alternatively an adjacency matrix can "
    "be used to define neighbouring indices for a particular dimension. If the 4th dimension "
    "contains volumes that correspond to equally distributed directions, then the adjacency "
    "can be defined using a set of directions and an angular threshold. This permits clustering "
    "over both the spatial and orientations domains";

  ARGUMENTS
  + Argument ("image_in",
              "the binary image to be labelled").type_image_in()
  + Argument ("image_out",
              "the labelled output image").type_image_out();

  OPTIONS
  + Option ("adjacency", "specify a symmetric binary adjacency matrix to define neighbouring indices along the "
            "defined axis. The matrix must be defined with a text file using spaces and lines "
            "to separate elements and rows respectively").allow_multiple()
  + Argument ("axis").type_integer ()
  + Argument ("matrix").type_file ()

  + Option ("directions", "the list of directions associated with each 3D volume, generated using the gendir command")
  + Argument ("file").type_file ()

  + Option ("angle", "the angular threshold used to define neighbouring directions (in degrees)")
  + Argument ("value").type_float (0, 15, 90)

  + Option ("ignore",
            "specify one or more axes to ignore. For example, to perform connected "
            "components separately on each 3D image within a 4D volume then ignore axis 3")
  + Argument ("axes").type_sequence_int()

  + Option ("largest",
            "only retain the largest component");
}


void run ()
{
  Image::Header input_header (argument[0]);

  // Compute neighbouring directions based on angular threshold
  float max_angle = 15;
  Options opt = get_options ("angle");
  if (opt.size())
    max_angle = opt[0][0];

  max_angle = max_angle * M_PI / 180.0;
  Math::Matrix<float> dirs;
  Math::Matrix<float> dirAdjacency;
  opt = get_options("directions");

  if (opt.size()) {
    dirs.load (opt[0][0]);
    Math::Matrix<float> vert (dirs.rows(), 3);
    for (uint d = 0; d < dirs.rows(); d++) {
      vert(d,0) = cos(dirs(d,0)) * sin(dirs(d,1));
      vert(d,1) = sin(dirs(d,0)) * sin(dirs(d,1));
      vert(d,2) = cos(dirs(d,1));
    }

    dirAdjacency.resize(dirs.rows(), dirs.rows(), 0.0);
    dirAdjacency.zero();
    for (uint m = 0; m < dirs.rows(); m++) {
      for (uint n = m + 1; n < dirs.rows(); n++) {
        float angle = Math::acos(Math::dot(vert.row(m), vert.row(n)));
        if (angle > M_PI_2)
          angle = M_PI - angle;
        if (angle < max_angle) {
          dirAdjacency (m,n) = 1;
          dirAdjacency (n,m) = 1;
        } else {
          dirAdjacency (m,n) = 0;
          dirAdjacency (n,m) = 0;
        }
      }
    }
  }

  Image::Buffer<float> input_data (input_header);
  Image::Buffer<float>::voxel_type input_voxel (input_data);

  Image::Filter::ConnectedComponents connected_filter(input_voxel);
  Image::Header header (input_data);
  header.info() = connected_filter.info();

  if (dirAdjacency.is_set())
    connected_filter.set_adjacency_matrix(dirAdjacency, 3);

  opt = get_options ("ignore");
  std::vector<int> axes;
  if (opt.size()) {
    axes = opt[0][0];
    for (size_t i = 0; i < axes.size(); i++) {
      if (axes[i] >= static_cast<int> (input_header.ndim()))
        throw Exception ("axis supplied to option -ignore is out of bounds");
    connected_filter.set_ignore_dim (true, axes[i]);
    }
  }

  opt = get_options ("adjacency");
  for (size_t i = 0; i != opt.size(); ++i) {
    Math::Matrix<float> adjacency;
    adjacency.load (opt[i][1]);
    int axis = opt[i][0];
    if (axis == 3 && dirAdjacency.is_set())
      throw Exception ("Conflicting options. Both directions and an adjacency "
                       "matrix cannot used to define the adjacency of axis 3");
    connected_filter.set_adjacency_matrix(adjacency, axis);
  }

  opt = get_options ("largest");
  if (opt.size())
    connected_filter.set_largest_only (true);

  Image::Buffer<int> output_data (argument[1], header);
  Image::Buffer<int>::voxel_type output_vox (output_data);

  connected_filter(input_voxel, output_vox);
}

