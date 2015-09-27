/*
    Copyright 2015 KU Leuven, Dept. Electrical Engineering, ESAT/PSI, Leuven, Belgium

    Written by Daan Christiaens, 26/09/15.

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
#include "file/ofstream.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"


using namespace MR;
using namespace App;
using namespace MR::DWI::Tractography;

void usage ()
{
  AUTHOR = "Daan Christiaens (daan.christiaens@gmail.com), "
           "J-Donald Tournier (jdtournier@gmail.com), "
           "Philip Broser (philip.broser@me.com).";

  DESCRIPTION
  + "Convert between different track file formats."

  + "The program currently supports MRtrix .tck files, "
    "VTK polydata files, and ascii text files.";

  ARGUMENTS
  + Argument ("input", "the input track file.").type_file_in ()
  + Argument ("output", "the output track file.").type_file_out ();

  OPTIONS
  + Option ("scanner2voxel",
      "if specified, the properties of this image will be used to convert "
      "track point positions from real (scanner) coordinates into voxel coordinates.")
    + Argument ("reference").type_image_in ();

}


class VTKWriter
{
public:
    VTKWriter(const std::string& file) : VTKout (file) {
        // create and write header of VTK output file:
        VTKout <<
          "# vtk DataFile Version 1.0\n"
          "Data values for Tracks\n"
          "ASCII\n"
          "DATASET POLYDATA\n"
          "POINTS ";
        // keep track of offset to write proper value later:
        offset_num_points = VTKout.tellp();
        VTKout << "XXXXXXXXXX float\n";
    }

    template <class ValueType>
    bool operator() (const Streamline<ValueType>& tck) {
        // write out points, and build index of tracks:
        size_t start_index = current_index;
        current_index += tck.size();
        track_list.push_back (std::pair<size_t,size_t> (start_index, current_index));

        for (auto pos : tck) {
          VTKout << pos[0] << " " << pos[1] << " " << pos[2] << "\n";
        }
        return true;
    }

    ~VTKWriter() {
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

private:
    File::OFStream VTKout;
    size_t offset_num_points;
    std::vector<std::pair<size_t,size_t>> track_list;
    size_t current_index = 0;

};




void run ()
{
    Properties properties;
    Reader read (argument[0], properties);

    VTKWriter write (argument[1]);

    Streamline<float> tck;
    while (read(tck))
    {
        write(tck);
    }

}

