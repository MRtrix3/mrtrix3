/*

    This source has been copied from track_info and then been modified so that
    it converts the mrtrix Tracks to a vtk readable format.
    the source has been rewritten by
    Philip Broser 1/06/2011 Univeristy of Tuebingen, philip.broser@me.com


    The original copyright reads:

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


    Modification of an original contribution from Philip Brozer.

*/

#include "app.h"
#include "command.h"
#include "file/ofstream.h"
#include "image/header.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"

using namespace MR;
using namespace MR::App;


void usage () 
{
  AUTHOR = "Philip Broser (philip.broser@me.com), J-Donald Tournier (jdtournier@gmail.com)";

  DESCRIPTION 
    + "convert a track file to a vtk format, cave: coordinates are in XYZ coordinates not reference";

  ARGUMENTS 
    + Argument ("in.tck", "the input track file.").type_file_in ()
    + Argument ("out.vtk", "the output vtk file name (use .vtk as suffix)").type_file_out ();

  OPTIONS 
    + Option ("voxel", 
        "if specified, the properties of this image will be used to convert "
        "track point positions from real (scanner) coordinates into voxel coordinates.")
    +    Argument ("reference").type_image_in ()

    + Option ("image", 
        "if specified, the properties of this image will be used to convert "
        "track point positions from real (scanner) coordinates into image coordinates (in mm).")
    +    Argument ("reference").type_image_in ();
}





void run () {
  std::unique_ptr<Image::Transform> transform;
  bool to_voxel = false;

  auto opt = get_options ("voxel");
  if (opt.size()) {
    to_voxel = true;
    Image::Header header (opt[0][0]);
    transform.reset (new Image::Transform (header));
  }

  opt = get_options ("image");
  if (opt.size()) {
    if (transform)
      throw Exception ("options \"-voxel\" and \"-image\" are mutually exclusive");
    to_voxel = false;
    Image::Header header (opt[0][0]);
    transform.reset (new Image::Transform (header));
  }

  DWI::Tractography::Properties properties;
  DWI::Tractography::Reader<float> file (argument[0], properties);
  const size_t count = to<size_t>(properties["count"]);

  // create and write header of VTK output file:
  std::string VTKFileName (argument[1]);
  File::OFStream VTKout (VTKFileName);
  
  VTKout << 
    "# vtk DataFile Version 1.0\n"
    "Data values for Tracks\n"
    "ASCII\n"
    "DATASET POLYDATA\n"
    "POINTS ";
  // keep track of offset to write proper value later:
  size_t offset_num_points = VTKout.tellp();
  VTKout << "XXXXXXXXXX float\n";


  DWI::Tractography::Streamline<float> tck;
  std::vector<std::pair<size_t,size_t>> track_list;
  size_t current_index = 0;

  {
    ProgressBar progress ("writing track data to VTK file", count);
    // write out points, and build index of tracks:
    while (file (tck)) {

      size_t start_index = current_index;
      current_index += tck.size();
      track_list.push_back (std::pair<size_t,size_t> (start_index, current_index));

      for (auto pos : tck) {
        if (transform) {
          if (to_voxel) pos = transform->scanner2voxel (pos);
          else pos = transform->scanner2image (pos);
        }
        VTKout << pos[0] << " " << pos[1] << " " << pos[2] << "\n";
      }

      ++progress;
    }
  }

  // write out list of tracks:
  VTKout << "LINES " << track_list.size() << " " << track_list.size() + current_index << "\n";
  for (const auto track : track_list) {
    VTKout << track.second - track.first << " " << track.first;
    for (size_t i = track.first + 1; i < track.second; ++i)
      VTKout << " " << i;
    VTKout << "\n";
  };

  // write back total number of points:
  VTKout.seekp (offset_num_points);
  std::string num_points (str (current_index));
  num_points.resize (10, ' ');
  VTKout.write (num_points.c_str(), 10);

  VTKout.close();
}

