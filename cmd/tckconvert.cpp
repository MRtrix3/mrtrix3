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
#include "file/name_parser.h"
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
  + Argument ("input", "the input track file.").type_text ()
  + Argument ("output", "the output track file.").type_file_out ();

  OPTIONS
  + Option ("scanner2voxel",
      "if specified, the properties of this image will be used to convert "
      "track point positions from real (scanner) coordinates into voxel coordinates.")
    + Argument ("reference").type_image_in ()

  + Option ("scanner2image",
      "if specified, the properties of this image will be used to convert "
      "track point positions from real (scanner) coordinates into image coordinates (in mm).")
  +    Argument ("reference").type_image_in ();

}


class VTKWriter: public WriterInterface<float>
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

    bool operator() (const Streamline<float>& tck) {
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




class ASCIIReader: public ReaderInterface<float>
{
public:
    ASCIIReader(const std::string& file) {
        VAR(file);
        auto num = list.parse_scan_check(file);
        VAR(num);
        VAR(list.size());
        VAR(list[0].name());
    }

    bool operator() (Streamline<float>& tck) {
        tck.clear();
        if (item < list.size()) {
            auto t = load_matrix<float>(list[item].name());
            for (size_t i = 0; i < t.rows(); i++)
                tck.push_back(Eigen::Vector3f(t.row(i)));
            item++;
            return true;
        }
        return false;
    }

    ~ASCIIReader() { }

private:
    File::ParsedName::List list;
    size_t item = 0;

};


class ASCIIWriter: public WriterInterface<float>
{
public:
    ASCIIWriter(const std::string& file) {
        count.push_back(0);
        parser.parse(file);
        parser.calculate_padding({1000000});
    }

    bool operator() (const Streamline<float>& tck) {
      std::string name = parser.name(count);
      File::OFStream out (name);
      for (auto i = tck.begin(); i != tck.end(); ++i)
        out << (*i) [0] << " " << (*i) [1] << " " << (*i) [2] << "\n";
      out.close();
      count[0]++;
      return true;
    }

    ~ASCIIWriter() { }

private:
    File::NameParser parser;
    std::vector<int> count;

};




bool has_suffix(const std::string &str, const std::string &suffix)
{
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}





void run ()
{
    // Reader
    Properties properties;
    std::unique_ptr<ReaderInterface<float> > reader;
    if (has_suffix(argument[0], ".tck")) {
        reader.reset( new Reader<float>(argument[0], properties) );
    }
    else if (has_suffix(argument[0], ".txt")) {
        reader.reset( new ASCIIReader(argument[0]) );
    }
    else {
        throw Exception("Unsupported input file type.");
    }

    // Writer
    std::unique_ptr<WriterInterface<float> > writer;
    if (has_suffix(argument[1], ".tck")) {
        writer.reset( new Writer<float>(argument[1], properties) );
    }
    else if (has_suffix(argument[1], ".vtk")) {
        writer.reset( new VTKWriter(argument[1]) );
    }
    else if (has_suffix(argument[1], ".txt")) {
        writer.reset( new ASCIIWriter(argument[1]) );
    }
    else {
        throw Exception("Unsupported output file type.");
    }
    
    
    // Tranform matrix
    transform_type T;
    T.setIdentity();
    size_t nopts = 0;
    auto opt = get_options("scanner2voxel");
    if (opt.size()) {
        auto header = Header::open(opt[0][0]);
        T = Transform(header).scanner2voxel;
        nopts++;
    }
    opt = get_options("scanner2image");
    if (opt.size()) {
        auto header = Header::open(opt[0][0]);
        T = Transform(header).scanner2image;
        nopts++;
    }
    if (nopts > 1) {
        throw Exception("Transform options are mutually exclusive.");
    }

    
    // Copy
    Streamline<float> tck;
    while ( (*reader)(tck) )
    {
        for (auto& pos : tck) {
            pos = T.cast<float>() * pos;
        }
        (*writer)(tck);
    }

}

