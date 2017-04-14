/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#include <cstdio>
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

  + "The program currently supports MRtrix .tck files (input/output), "
    "ascii text files (input/output), and VTK polydata files (output only).";

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
  +    Argument ("reference").type_image_in ()

  + Option ("voxel2scanner",
      "if specified, the properties of this image will be used to convert "
      "track point positions from voxel coordinates into real (scanner) coordinates.")
    + Argument ("reference").type_image_in ()

  + Option ("image2scanner",
      "if specified, the properties of this image will be used to convert "
      "track point positions from image coordinates (in mm) into real (scanner) coordinates.")
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
        auto num = list.parse_scan_check(file);
    }

    bool operator() (Streamline<float>& tck) {
        tck.clear();
        if (item < list.size()) {
            auto t = load_matrix<float>(list[item].name());
            for (size_t i = 0; i < size_t(t.rows()); i++)
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


class PLYWriter: public WriterInterface<float>
{
public:
    PLYWriter(const std::string& file) : out(file) {
        vertexFilename = File::create_tempfile(0,".vertex");
        faceFilename = File::create_tempfile(0,".face");
        
        vertexOF.open(vertexFilename);
        faceOF.open(faceFilename);
        num_faces = 0;
        num_vertices = 0;
    }

    bool operator() (const Streamline<float>& tck) {
        // Need at least 5 points, silently ignore...
        if ( tck.size() < 5 ) { return true; }

        auto nSides = 9;
        auto radius = .1;
        Eigen::MatrixXf coords(nSides,2);
        Eigen::MatrixXi faces(nSides,6);
        auto theta = 2.0 * M_PI / float(nSides);
        for ( auto i = 0; i < nSides; i++ ) {
            coords(i,0) = cos((double)i*theta);
            coords(i,1) = sin((double)i*theta);
            // Face offsets
            faces(i,0) = i;
            faces(i,1) = (i+1) % nSides;
            faces(i,2) = i+nSides;
            faces(i,3) = (i+1) % nSides;
            faces(i,4) = (i+1) % nSides + nSides;
            faces(i,5) = i+nSides;
        }

        auto first_point = 1;
        auto last_point = tck.size() - 2;
        for ( auto idx = 1; idx < tck.size() - 1; idx++ ) {
            auto p = tck[idx];
            auto prior = p;
            auto next = p;
            // if ( idx == 0 ) {
            //     // Project the next point backwards
            //     next = tck[idx+1];
            //     prior = p + ( p - tck[idx+2]);
            // } else if ( idx == tck.size() -1 ) {
                // prior = tck[idx-1];
                // next = p + ( p - tck[idx-2] );
            // } else {
                prior = tck[idx-1];
                next = tck[idx+1];
            // }
            auto T = (next - p).normalized();
            auto T2 = (p - prior).normalized();
            auto N = T.cross(T2).normalized();
            auto B = T.cross(N).normalized();

            if ( idx < 2 ) {
                std::cout << "\n\nidx: " << idx << "\n";
                Eigen::IOFormat fmt(Eigen::StreamPrecision, 0, ", ", ", ", "", "", "[", "];");

                std::cout << "p = " << p.format(fmt) << "\n";
                std::cout << "next = " << next.format(fmt) << "\n";
                std::cout << "prior = " << prior.format(fmt) << "\n";
                std::cout << "T = " << T.format(fmt) << "\n";
                std::cout << "N = " << N.format(fmt) << "\n";
                std::cout << "B = " << B.format(fmt) << "\n";
                std::cout << "T2 = " << T2.format(fmt) << "\n";

                Eigen::IOFormat OctaveFmt(Eigen::StreamPrecision, 0, ", ", ";\n", "", "", "[", "]");

                std::cout << " coords = " << coords.format(OctaveFmt) << "\n";
                std::cout << " faces = " << faces.format(OctaveFmt) << "\n";

                auto sidePoint = p + radius * ( N * coords(0,0) + B * coords(0,1));
                std::cout << "sidePoint = " << sidePoint.format(fmt) << "\n";
            }

            
            // have our coordinate frame, now add circles
            for ( auto sideIdx = 0; sideIdx < nSides; sideIdx++ ) {
                auto sidePoint = p + radius * ( N * coords(sideIdx,0) + B * coords(sideIdx,1));
                vertexOF << sidePoint[0] << " "<< sidePoint[1] << " " << sidePoint[2] << " ";
                // Write the color
                vertexOF << (int)( 255 * fabs(N[0])) << " " << (int)( 255 * fabs(N[1])) << " " << (int)( 255 * fabs(N[2])) << "\n";
                if ( idx != last_point ) {
                    faceOF << "3"
                           << " " << num_vertices + faces(sideIdx,0)
                           << " " << num_vertices + faces(sideIdx,1)
                           << " " << num_vertices + faces(sideIdx,2) << "\n";
                    faceOF << "3"
                           << " " << num_vertices + faces(sideIdx,3)
                           << " " << num_vertices + faces(sideIdx,4)
                           << " " << num_vertices + faces(sideIdx,5) << "\n";
                    num_faces += 2;
                }
            }
            // Cap the first point, remebering the right hand rule
            if ( idx == first_point ) {
                for ( auto sideIdx = nSides - 1; sideIdx >= 2; --sideIdx ) {
                    faceOF << "3"
                           << " " << num_vertices + sideIdx
                           << " " << num_vertices + sideIdx - 1
                           << " " << num_vertices << "\n";
                }
                num_faces += nSides - 2;
            }
            if ( idx == last_point ) {
                // std::cout << "Writing end cap, num_vertices = " << num_vertices << "\n";
                for ( auto sideIdx = 2; sideIdx <= nSides - 1; ++sideIdx ) {
                    faceOF << "3"
                           << " " << num_vertices
                           << " " << num_vertices + sideIdx
                           << " " << num_vertices + sideIdx - 1 << "\n";
                }
                num_faces += nSides - 2;
            }
            // We needed to maintain the number of vertices for the caps, now increment for the "circles"
            num_vertices += nSides;
        }
        return true;
    }

    ~PLYWriter() {
        // write out list of tracks:
        vertexOF.close();
        faceOF.close();
        
        out <<
            "ply\n"
            "format ascii 1.0\n"
            "comment written by tckconvert v" << mrtrix_version << "\n"
            "comment part of the mtrix3 suite of tools (http://www.mrtrix.org/)\n"
            "element vertex " << num_vertices << "\n"
            "property float32 x\n"
            "property float32 y\n"
            "property float32 z\n"
            "property uchar red\n"
            "property uchar green\n"
            "property uchar blue\n"
            "element face " << num_faces << "\n"
            "property list uint8 int32 vertex_indices\n"
            "end_header\n";

        std::ifstream vertexIF(vertexFilename);
        out << vertexIF.rdbuf();
        vertexIF.close();
        std::ifstream faceIF(faceFilename);
        out << faceIF.rdbuf();
        faceIF.close();
        std::remove ( vertexFilename.c_str() );
        std::remove ( faceFilename.c_str() );
        out.close();
    }

private:
    std::string vertexFilename;
    std::string faceFilename;
    File::OFStream out;
    File::OFStream vertexOF;
    File::OFStream faceOF;
    size_t face_offset;
    size_t vertex_offset;
    size_t num_vertices;
    size_t num_faces;
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
    else if (has_suffix(argument[1], ".ply")) {
        writer.reset( new PLYWriter(argument[1]) );
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
    opt = get_options("voxel2scanner");
    if (opt.size()) {
        auto header = Header::open(opt[0][0]);
        T = Transform(header).voxel2scanner;
        nopts++;
    }
    opt = get_options("image2scanner");
    if (opt.size()) {
        auto header = Header::open(opt[0][0]);
        T = Transform(header).image2scanner;
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

