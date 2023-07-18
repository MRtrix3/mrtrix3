/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include <cstdio>
#include <sstream>
#include "command.h"
#include "file/ofstream.h"
#include "file/name_parser.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "raw.h"

using namespace MR;
using namespace App;
using namespace MR::DWI::Tractography;
using namespace MR::Raw;
using namespace MR::ByteOrder;

void usage ()
{
  AUTHOR = "Daan Christiaens (daan.christiaens@kcl.ac.uk), "
    "J-Donald Tournier (jdtournier@gmail.com), "
    "Philip Broser (philip.broser@me.com), "
    "Daniel Blezek (daniel.blezek@gmail.com).";

  SYNOPSIS = "Convert between different track file formats";

  DESCRIPTION
    + "The program currently supports MRtrix .tck files (input/output), "
    "ascii text files (input/output), VTK polydata files (input/output), "
    "and RenderMan RIB (export only)."

    + "Note that ascii files will be stored with one streamline per numbered file. "
    "To support this, the command will use the multi-file numbering syntax, "
    "where square brackets denote the position of the numbering for the files, "
    "for example:"

    + "$ tckconvert input.tck output-'[]'.txt"

    + "will produce files named output-0000.txt, output-0001.txt, output-0002.txt, ...";

  ARGUMENTS
    + Argument ("input", "the input track file.").type_various ()
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
    +    Argument ("reference").type_image_in ()

    + OptionGroup ("Options specific to PLY writer")

    + Option ("sides", "number of sides for streamlines")
    +   Argument("sides").type_integer(3,15)

    + Option ("increment", "generate streamline points at every (increment) points")
    +   Argument("increment").type_integer(1)

    + OptionGroup ("Options specific to RIB writer")

    + Option ("dec", "add DEC as a primvar")

    + OptionGroup ("Options for both PLY and RIB writer")

    + Option ("radius", "radius of the streamlines")
    +   Argument("radius").type_float(0.0f)

    + OptionGroup ("Options specific to VTK writer")

    + Option ("ascii", "write an ASCII VTK file (binary by default)");

}







class VTKWriter: public WriterInterface<float> { 
  public:
    VTKWriter(const std::string& file, bool write_ascii = true) :
      VTKout (file, std::ios::binary), write_ascii(write_ascii) {
        // create and write header of VTK output file:
        VTKout <<
          "# vtk DataFile Version 3.0\n"
          "Data values for Tracks\n";
        if (write_ascii) {
          VTKout << "ASCII\n";
        } else {
          VTKout << "BINARY\n";
        }
        VTKout << "DATASET POLYDATA\n"
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
      if (write_ascii) {
        for (const auto &pos : tck) {
          VTKout << pos[0] << " " << pos[1] << " " << pos[2] << "\n";
        }
      } else {
        float p[3];
        for (const auto& pos : tck) {
          for (auto i = 0; i < 3; ++i) Raw::store_BE(pos[i], p, i);
          VTKout.write((char*)p, 3 * sizeof(float));
        }
      }
      return true;
    }

    ~VTKWriter() {
      try {
        // write out list of tracks:
        if (write_ascii == false) {
          // Need to include an extra new line when writing binary
          VTKout << "\n";
        }
        VTKout << "LINES " << track_list.size() << " " << track_list.size() + current_index << "\n";
        for (const auto& track : track_list) {
          if (write_ascii) {
            VTKout << track.second - track.first << " " << track.first;
            for (size_t i = track.first + 1; i < track.second; ++i)
              VTKout << " " << i;
            VTKout << "\n";
          }
          else {
            int32_t buffer;
            buffer = ByteOrder::BE<int32_t> (track.second - track.first);
            VTKout.write ((char*) &buffer, 1 * sizeof(int32_t));

            buffer = ByteOrder::BE<int32_t> (track.first);
            VTKout.write ((char*) &buffer, 1 * sizeof(int32_t));

            for (size_t i = track.first + 1; i < track.second; ++i) {
              buffer = ByteOrder::BE<int32_t> (i);
              VTKout.write ((char*)&buffer, 1* sizeof(int32_t));
            }
          }
        }
        if (write_ascii == false) {
          // Need to include an extra new line when writing binary
          VTKout << "\n";
        }

        // write back total number of points:
        VTKout.seekp (offset_num_points);
        std::string num_points (str (current_index));
        num_points.resize (10, ' ');
        VTKout.write (num_points.c_str(), 10);

        VTKout.close();
      }
      catch (Exception& e) {
        e.display();
        App::exit_error_code = 1;
      }
    }

  private:
    File::OFStream VTKout;
    const bool write_ascii;
    size_t offset_num_points;
    vector<std::pair<size_t,size_t>> track_list;
    size_t current_index = 0;

};





template <class T> void loadLines(vector<int64_t>& lines, std::ifstream& input, int number_of_line_indices)
{
  vector<T> buffer (number_of_line_indices);
  input.read((char*) &buffer[0], number_of_line_indices * sizeof(T));
  lines.resize (number_of_line_indices);
  // swap from big endian
  for (int i = 0; i < number_of_line_indices; i++)
    lines[i] = int64_t (ByteOrder::BE (buffer[i]));
}



class VTKReader: public ReaderInterface<float> { 
  public:
    VTKReader (const std::string& file) {
      std::ifstream input (file, std::ios::binary);
      std::string line;
      int number_of_points = 0;
      number_of_lines = 0;
      number_of_line_indices = 0;

      while (std::getline(input,line)) {
        if (line.find ("ASCII") == 0)
          throw Exception("VTK Reader only supports BINARY input");

        if (sscanf (line.c_str(), "POINTS %d float", &number_of_points) == 1) {
          points.resize (3*number_of_points);
          input.read ((char*) points.data(), 3*number_of_points * sizeof(float));

          // swap
          for (int i = 0; i < 3*number_of_points; i++)
            points[i] = ByteOrder::BE (points[i]);

          continue;
        }
        else {
          if (sscanf (line.c_str(), "LINES %d %d", &number_of_lines, &number_of_line_indices) == 2) {
            if (line.find ("vtktypeint64") != std::string::npos) {
              loadLines<int64_t> (lines, input, number_of_line_indices);
            } else {
              loadLines<int32_t> (lines, input, number_of_line_indices);
            }
            // We can safely break
            break;
          }
        }
      }
      input.close();
      lineIdx = 0;
    }

    bool operator() (Streamline<float>& tck) {
      tck.clear();
      if (lineIdx < number_of_line_indices) {
        int count = lines[lineIdx];
        lineIdx++;
        for ( int i = 0; i < count; i++ ) {
          int idx = lines[lineIdx];
          Eigen::Vector3f f (points[idx*3], points[idx*3+1], points[idx*3+2]);
          tck.push_back(f);
          lineIdx++;
        }
        return true;
      }
      return false;
    }

  private:
    vector<float> points;
    vector<int64_t> lines;
    int lineIdx;
    int number_of_lines;
    int number_of_line_indices;

};






class ASCIIReader: public ReaderInterface<float> { 
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







class ASCIIWriter: public WriterInterface<float> { 
  public:
    ASCIIWriter(const std::string& file) {
      count.push_back(0);
      parser.parse(file);
      if (parser.ndim() != 1)
        throw Exception ("output file specifier should contain one placeholder for numbering (e.g. output-[].txt)");
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
    vector<uint32_t> count;

};







class PLYWriter: public WriterInterface<float> { 
  public:
    PLYWriter(const std::string& file, int increment = 1, float radius = 0.1, int sides = 5) :
      out(file), increment(increment),
      radius(radius), sides(sides) {
        vertexFilename = File::create_tempfile (0,"vertex");
        faceFilename = File::create_tempfile (0,"face");

        vertexOF.open(vertexFilename);
        faceOF.open(faceFilename);
        num_faces = 0;
        num_vertices = 0;

      }

    Eigen::Vector3f computeNormal ( const Streamline<float>& tck ) {
      // copy coordinates to  matrix in Eigen format
      size_t num_atoms = tck.size();
      Eigen::Matrix< float, Eigen::Dynamic, Eigen::Dynamic > coord(3, num_atoms);
      for (size_t i = 0; i < num_atoms; ++i) {
        coord.col(i) = tck[i];
      }

      // calculate centroid
      Eigen::Vector3d centroid(coord.row(0).mean(), coord.row(1).mean(), coord.row(2).mean());

      // subtract centroid
      coord.row(0).array() -= centroid(0);
      coord.row(1).array() -= centroid(1);
      coord.row(2).array() -= centroid(2);

      // we only need the left-singular matrix here
      //  http://math.stackexchange.com/questions/99299/best-fitting-plane-given-a-set-of-points
      auto svd = coord.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV);
      Eigen::Vector3f plane_normal = svd.matrixU().rightCols<1>();
      return plane_normal;
    }

    void computeNormals ( const Streamline<float>& tck, Streamline<float>& normals) {
      Eigen::Vector3f sPrev = (tck[1] - tck[0]).normalized();
      Eigen::Vector3f normal = Eigen::Vector3f::Zero();
      
      // Find a good starting normal
      for (size_t idx = 1; idx < tck.size() - 1; idx++) {
        const auto& pt1 = tck[idx];
        const auto& pt2 = tck[idx+1];
        const auto sNext = (pt2 - pt1).normalized();
        const auto n = sPrev.cross(sNext);
        if ( n.norm() > 1.0E-3 ) {
          normal = n;
          sPrev = sNext;
          break;
        }
      }

      normal.normalize();  // vtkPolyLine.cxx:170
      for (size_t idx = 0; idx < tck.size() - 1; idx++) {
        const auto& pt1 = tck[idx];
        const auto& pt2 = tck[idx+1];
        const auto sNext = (pt2 - pt1).normalized();

        // compute rotation vector vtkPolyLine.cxx:187
        auto w = sPrev.cross(normal);
        if ( w.norm() == 0.0 ) {
          // copy the normal and continue
          normals.push_back ( normal );
          continue;
        }
        // compute rotation of line segment
        auto q = sNext.cross(sPrev);
        if ( q.norm() == 0.0 ) {
          // copy the normal and continue
          normals.push_back ( normal );
          continue;
        }
        auto f1 = q.dot(normal);
        auto f2 = 1.0 - ( f1 * f1 );
        if ( f2 > 0.0 ) {
          f2 = sqrt(1.0 - (f1*f1));
        } else {
          f2 = 0.0;
        }

        auto c = (sNext + sPrev).normalized();
        w = c.cross(q);
        c = sPrev.cross(q);
        if ( ( normal.dot(c) * w.dot(c)) < 0 ) {
          f2 = -1.0 * f2;
        }
        normals.push_back(normal);
        sPrev = sNext;
        normal = ( f1 * q ) + (f2 * w);
      }
    }


    bool operator() (const Streamline<float>& intck) {
      // Need at least 5 points, silently ignore...
      if (intck.size() < size_t(increment * 3)) { return true; }

      auto nSides = sides;
      Eigen::MatrixXf coords(nSides,2);
      Eigen::MatrixXi faces(nSides,6);
      auto theta = 2.0 * Math::pi / float(nSides);
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

      // to handle the increment, we want to keep the first 2 and last 2 points, but we can skip inside
      Streamline<float> tck;

      // Push on the first 2 points
      tck.push_back(intck[0]);
      tck.push_back(intck[1]);
      for (size_t idx = 3; idx < intck.size() - 2; idx += increment) {
        tck.push_back(intck[idx]);
      }
      tck.push_back(intck[intck.size()-2]);
      tck.push_back(intck[intck.size()-1]);

      Streamline<float> normals;
      this->computeNormals(tck,normals);
      auto globalNormal = computeNormal(tck);
      Eigen::Vector3f sNext = tck[1] - tck[0];
      auto isFirst = true;
      for (size_t idx = 1; idx < tck.size() - 1; ++idx) {
        auto isLast = idx == tck.size() - 2;

        // vtkTubeFilter.cxx:386
        Eigen::Vector3f p = tck[idx];
        Eigen::Vector3f pNext = tck[idx+1];
        Eigen::Vector3f sPrev = sNext;
        sNext = pNext - p;
        Eigen::Vector3f n = normals[idx];

        sNext.normalize();
        if ( sNext.norm() == 0.0 ) {
          continue;
        }

        // Average vectors
        Eigen::Vector3f s = ( sPrev + sNext ) / 2.0;
        s.normalize();
        if ( s.norm() == 0.0 ) {
          s = sPrev.cross(n).normalized();
        }

        auto T = s;
        auto N = T.cross(globalNormal).normalized();
        auto B = T.cross(N).normalized();
        N = B.cross(T).normalized();


        // have our coordinate frame, now add circles
        for ( auto sideIdx = 0; sideIdx < nSides; sideIdx++ ) {
          auto sidePoint = p + radius * ( N * coords(sideIdx,0) + B * coords(sideIdx,1));
          vertexOF << sidePoint[0] << " "<< sidePoint[1] << " " << sidePoint[2] << " ";
          vertexOF << (int)( 255 * fabs(T[0])) << " " << (int)( 255 * fabs(T[1])) << " " << (int)( 255 * fabs(T[2])) << "\n";
          if ( !isLast ) {
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
        if ( isFirst ) {
          for ( auto sideIdx = nSides - 1; sideIdx >= 2; --sideIdx ) {
            faceOF << "3"
              << " " << num_vertices + sideIdx
              << " " << num_vertices + sideIdx - 1
              << " " << num_vertices << "\n";
          }
          num_faces += nSides - 2;
          isFirst = false;
        }
        if ( isLast ) {
          for ( auto sideIdx = 2; sideIdx <= nSides - 1; ++sideIdx ) {
            faceOF << "3"
              << " " << num_vertices + sideIdx - 1
              << " " << num_vertices + sideIdx
              << " " << num_vertices
              << "\n";
          }
          num_faces += nSides - 2;
        }
        // We needed to maintain the number of vertices for the caps, now increment for the "circles"
        num_vertices += nSides;
      }
      return true;
    }

    ~PLYWriter() {
      try {
        // write out list of tracks:
        vertexOF.close();
        faceOF.close();

        out <<
          "ply\n"
          "format ascii 1.0\n"
          "comment written by tckconvert v" << App::mrtrix_version << "\n"
          "comment part of the mtrix3 suite of tools (http://www.mrtrix.org/)\n"
          "comment the coordinate system and scale is taken from directly from the input and is not adjusted\n"
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

        std::ifstream vertexIF (vertexFilename);
        out << vertexIF.rdbuf();
        vertexIF.close();
        File::remove (vertexFilename);

        std::ifstream faceIF (faceFilename);
        out << faceIF.rdbuf();
        faceIF.close();
        File::remove (faceFilename);

        out.close();
      } catch (Exception& e) {
        e.display();
        App::exit_error_code = 1;
      }
    }

  private:
    std::string vertexFilename;
    std::string faceFilename;
    File::OFStream out;
    File::OFStream vertexOF;
    File::OFStream faceOF;
    size_t num_vertices;
    size_t num_faces;
    int increment;
    float radius;
    int sides;
};










class RibWriter: public WriterInterface<float> { 
  public:
    RibWriter(const std::string& file, float radius = 0.1, bool dec = false) :
      out(file), writeDEC(dec), radius(radius),
      hasPoints(false), wroteHeader(false) {
        pointsFilename = File::create_tempfile (0,"points");
        pointsOF.open (pointsFilename);
        pointsOF << "\"P\" [";
        decFilename = File::create_tempfile (0,"dec");
        decOF.open (decFilename);
        decOF << "\"varying color dec\" [";
        // Header
        out << "##RenderMan RIB\n"
          << "# Written by tckconvert\n"
          << "# Part of the MRtrix package (http://mrtrix.org)\n"
          << "# version: " << App::mrtrix_version << "\n";

      }

    bool operator() (const Streamline<float>& tck) {
      if ( tck.size() < 3 ) {
        return true;
      }

      hasPoints = true;
      if ( !wroteHeader ) {
        wroteHeader = true;
        // Start writing the header
        out << "Basis \"catmull-rom\" 1 \"catmull-rom\" 1\n"
          << "Attribute \"dice\" \"int roundcurve\" [1] \"int hair\" [1]\n"
          << "Curves \"linear\" [";
      }
      out << tck.size() << " ";
      Eigen::Vector3f prev = tck[1];
      for ( auto pt : tck ) {
        pointsOF << pt[0] << " " << pt[1] << " " << pt[2] << " ";
        // Should we write the dec?
        if ( writeDEC ) {
          Eigen::Vector3f T = ( prev - pt ).normalized();
          decOF << fabs(T[0]) << " " << fabs(T[1]) << " " << fabs(T[2]) << " ";
          prev = pt;
        }
      }
      return true;
    }

    ~RibWriter() {
      try {

        if (hasPoints) {
          pointsOF << "]\n" ;
          decOF << "]\n" ;
        }

        pointsOF.close();
        decOF.close();

        if (hasPoints) {
          out << "] \"nonperiodic\" ";

          std::ifstream pointsIF ( pointsFilename );
          out << pointsIF.rdbuf();

          if ( writeDEC ) {
            std::ifstream decIF ( decFilename );
            out << decIF.rdbuf();
            decIF.close();
          }

          out << " \"constantwidth\" " << radius << "\n";
        }

        out.close();

        File::remove (pointsFilename);
        File::remove (decFilename);

      } catch (Exception& e) {
        e.display();
        App::exit_error_code = 1;
      }
    }


  private:
    std::string pointsFilename;
    std::string decFilename;
    File::OFStream out;
    File::OFStream pointsOF;
    File::OFStream decOF;
    bool writeDEC;
    float radius;
    bool hasPoints;
    bool wroteHeader;
};












void run ()
{
  // Reader
  Properties properties;
  std::unique_ptr<ReaderInterface<float> > reader;
  if (Path::has_suffix(argument[0], ".tck")) {
    reader.reset (new Reader<float>(argument[0], properties));
  }
  else if (Path::has_suffix(argument[0], ".txt")) {
    reader.reset (new ASCIIReader(argument[0]));
  }
  else if (Path::has_suffix(argument[0], ".vtk")) {
    reader.reset (new VTKReader(argument[0]));
  }
  else {
    throw Exception ("Unsupported input file type.");
  }


  // Writer
  std::unique_ptr<WriterInterface<float> > writer;
  if (Path::has_suffix(argument[1], ".tck")) {
    writer.reset (new Writer<float>(argument[1], properties));
  }
  else if (Path::has_suffix(argument[1], ".vtk")) {
    auto write_ascii = get_options("ascii").size();
    writer.reset (new VTKWriter(argument[1], write_ascii));
  }
  else if (Path::has_suffix(argument[1], ".ply")) {
    auto increment = get_options("increment").size() ? get_options("increment")[0][0].as_int() : 1;
    auto radius = get_options("radius").size() ? get_options("radius")[0][0].as_float() : 0.1f;
    auto sides = get_options("sides").size() ? get_options("sides")[0][0].as_int() : 5;
    writer.reset (new PLYWriter(argument[1], increment, radius, sides));
  }
  else if (Path::has_suffix(argument[1], ".rib")) {
    writer.reset (new RibWriter(argument[1]));
  }
  else if (Path::has_suffix(argument[1], ".txt")) {
    writer.reset (new ASCIIWriter(argument[1]));
  }
  else {
    throw Exception ("Unsupported output file type.");
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
  while ((*reader)(tck)) {
    for (auto& pos : tck) {
      pos = T.cast<float>() * pos;
    }
    (*writer)(tck);
  }

}

