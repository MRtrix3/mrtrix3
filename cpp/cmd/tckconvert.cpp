/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "command.h"
#include "datatype.h"
#include "dwi/tractography/field_registry.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/formats/list.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/sidecar.h"
#include "dwi/tractography/sidecar_value.h"
#include "dwi/tractography/tractogram.h"
#include "dwi/tractography/tractogram_item.h"
#include "enum.h"
#include "file/matrix.h"
#include "file/name_parser.h"
#include "file/ofstream.h"

using namespace MR;
using namespace App;
using namespace MR::DWI::Tractography;

//! \brief CLI selector disambiguating per-streamline (dps) from per-vertex (dpv) sidecar fields.
/*! Every sidecar-manipulation option takes this as its first argument: a given
 * name may identify both a dps and a dpv field (legal in every sidecar-capable
 * format), so the role must be stated explicitly rather than inferred. */
enum class SidecarType { dps, dpv };

namespace {
FieldRole to_field_role(const SidecarType type) { return type == SidecarType::dpv ? FieldRole::DPV : FieldRole::DPS; }
std::string role_word(const FieldRole role) {
  return role == FieldRole::DPV ? "per-vertex (dpv)" : "per-streamline (dps)";
}
} // namespace

constexpr int default_ply_increment = 1;
constexpr float default_ply_radius = 0.1F;
constexpr int default_ply_sides = 5;

// clang-format off
void usage() {

  AUTHOR = "Daan Christiaens (daan.christiaens@kcl.ac.uk)"
           " and J-Donald Tournier (jdtournier@gmail.com)"
           " and Philip Broser (philip.broser@me.com)"
           " and Daniel Blezek (daniel.blezek@gmail.com)";

  SYNOPSIS = "Convert between different track file formats";

  DESCRIPTION
    + "The program currently supports"
      " MRtrix .tck files (input/output),"
      " ascii text files (input/output),"
      " VTK polydata files (input/output),"
      " QFib lossy compressed .qfib files (input/output),"
      " and RenderMan RIB (export only)."

    + "The QFib format (Mercier et al.) stores each streamline as its first two"
      " vertices plus a sequence of quantized unit tangents. It is lossy, requires"
      " the input to be of constant step size (resample beforehand with"
      " \"tckresample -step_size\" otherwise), and stores geometry only:"
      " per-streamline weights and dps/dpv sidecar data are discarded."

    + "Some tractography file formats (the TrackVis \".trk\" format and the TRX"
      " format) can embed per-streamline (dps) and per-vertex (dpv) sidecar data"
      " within the tractogram dataset itself. The -extract, -insert, -rename,"
      " -remove and -convert options manipulate this embedded data during"
      " conversion. Each takes a leading \"dps\" or \"dpv\" argument to disambiguate"
      " the two, since a per-streamline and a per-vertex field may legitimately"
      " share the same name. Per-streamline data is exchanged with standalone"
      " numerical files (text, \".csv\" or \".npy\"); per-vertex data with track"
      " scalar (\".tsf\") files. When a \".tsf\" is produced from extracted"
      " per-vertex data, a matching \"timestamp\" key-value is recorded on both it"
      " and the output tractogram so the pair passes the track-scalar validation"
      " checks. Fields are always referenced by string name, never by index.";

  EXAMPLES
    + Example("Writing multiple ASCII files, one per streamline",
              "tckconvert input.tck output-[].txt",
              "By using the multi-file numbering syntax,"
              " where square brackets denote the position of the numbering for the files,"
              " this example will produce files named"
              " output-0000.txt, output-0001.txt, output-0002.txt, ...");

  ARGUMENTS
    + Argument ("input", "the input track file.").type_tracks_in().type_file_in().type_text()
    + Argument ("output", "the output track file.").type_tracks_out().type_file_out();

  OPTIONS
    + Option ("scanner2voxel",
        "if specified,"
        " the properties of this image will be used to convert track point positions"
        " from real (scanner) coordinates into voxel coordinates.")
      + Argument ("reference").type_image_in()

    + Option ("scanner2image",
        "if specified,"
        " the properties of this image will be used to convert track point positions"
        " from real (scanner) coordinates into image coordinates (in mm).")
      + Argument ("reference").type_image_in()

    + Option ("voxel2scanner",
        "if specified,"
        " the properties of this image will be used to convert track point positions"
        " from voxel coordinates into real (scanner) coordinates.")
      + Argument ("reference").type_image_in()

    + Option ("image2scanner",
        "if specified,"
        " the properties of this image will be used to convert track point positions"
        " from image coordinates (in mm) into real (scanner) coordinates.")
      + Argument ("reference").type_image_in()

    + OptionGroup ("Options specific to PLY writer")

    + Option ("sides", "number of sides for streamlines")
      + Argument("sides").type_integer(3, 15)

    + Option ("increment", "generate streamline points at every (increment) points")
      + Argument("increment").type_integer(1)

    + OptionGroup ("Options specific to RIB writer")

    + Option ("dec", "add DEC as a primvar")

    + OptionGroup ("Options for both PLY and RIB writer")

    + Option ("radius", "radius of the streamlines")
      + Argument("radius").type_float(0.0)

    + OptionGroup ("Options specific to VTK writer")

    + Option ("ascii", "write an ASCII VTK file"
                       " (binary by default)")
  // The "-ascii" option is consumed by the framework's .vtk format handler
  //   (dwi/tractography/formats/vtk.cpp), which selects the ASCII encoding when
  //   it is present; tckconvert itself no longer branches on the VTK encoding.

    + OptionGroup ("Options specific to ZFIB writer")

    + Option ("zfib_max_error",
              "the worst-case compression error in mm for lossy .zfib output (default: 0.5)")
      + Argument("value").type_float(0.0)
  // The "-zfib_max_error" option is consumed by the framework's .zfib format
  //   handler backend (dwi/tractography/formats/zfib.cpp), which derives the
  //   linearization tolerance and the quantization precision from it.

    + OptionGroup ("Options specific to the QFib writer")

    + Option ("qfib_bits",
              "the per-direction quantization bit depth for lossy .qfib output,"
              " either 8 or 16 (default: 16)")
      + Argument("depth").type_integer(8, 16)

    + Option ("qfib_max_angle",
              "the maximum streamline deviation angle in degrees for lossy .qfib output;"
              " defaults to the max_angle property of the input, else 90")
      + Argument("angle").type_float(0.0, 90.0)
  // The "-qfib_bits" and "-qfib_max_angle" options are consumed by the framework's
  //   .qfib format handler backend (dwi/tractography/formats/qfib.cpp), which
  //   derives the octahedral bit depth and the cap-to-sphere ratio from them.

    + OptionGroup ("Options for manipulating embedded sidecar data")

    + Option ("extract",
              "extract an embedded sidecar field, referenced by name, to a standalone file")
      .allow_multiple()
      + Argument ("type").type_choice<SidecarType>()
      + Argument ("name").type_text()
      + Argument ("file").type_file_out()

    + Option ("insert",
              "embed a new sidecar field, read from a standalone file, into the output")
      .allow_multiple()
      + Argument ("type").type_choice<SidecarType>()
      + Argument ("name").type_text()
      + Argument ("file").type_file_in()

    + Option ("rename", "rename an embedded sidecar field")
      .allow_multiple()
      + Argument ("type").type_choice<SidecarType>()
      + Argument ("old").type_text()
      + Argument ("new").type_text()

    + Option ("remove", "remove an embedded sidecar field")
      .allow_multiple()
      + Argument ("type").type_choice<SidecarType>()
      + Argument ("name").type_text()

    + Option ("convert", "change the on-disk datatype of an embedded sidecar field")
      .allow_multiple()
      + Argument ("type").type_choice<SidecarType>()
      + Argument ("name").type_text()
      + Argument ("datatype").type_text();

}
// clang-format on

class ASCIIReader : public ReaderInterface<float> {
public:
  ASCIIReader(std::string_view file) { auto num = list.parse_scan_check(file); }

  bool operator()(Streamline<float> &tck) {
    tck.clear();
    if (item < list.size()) {
      auto t = File::Matrix::load_matrix<float>(list[item].name());
      for (decltype(t)::Index i = 0; i < t.rows(); i++)
        tck.push_back(Eigen::Vector3f(t.row(i)));
      item++;
      return true;
    }
    return false;
  }

  ~ASCIIReader() {}

private:
  File::ParsedName::List list;
  size_t item = 0;
};

class ASCIIWriter : public WriterInterface<float> {
public:
  ASCIIWriter(std::string_view file) {
    count.push_back(0);
    parser.parse(file);
    if (parser.ndim() != 1)
      throw Exception("output file specifier should contain one placeholder for numbering (e.g. output-[].txt)");
    parser.calculate_padding({1000000});
  }

  bool operator()(const Streamline<float> &tck) {
    File::OFStream out(parser.name(count));
    for (auto i = tck.begin(); i != tck.end(); ++i)
      out << (*i)[0] << " " << (*i)[1] << " " << (*i)[2] << "\n";
    out.close();
    count[0]++;
    return true;
  }

  ~ASCIIWriter() {}

private:
  File::NameParser parser;
  std::vector<uint32_t> count;
};

class PLYWriter : public WriterInterface<float> {
public:
  PLYWriter(const std::filesystem::path &path,
            int increment = default_ply_increment,
            float radius = default_ply_radius,
            int sides = default_ply_sides)
      : out(path), increment(increment), radius(radius), sides(sides) {
    vertexFilepath = File::create_tempfile(0, ".vertex");
    faceFilepath = File::create_tempfile(0, ".face");

    vertexOF.open(vertexFilepath);
    faceOF.open(faceFilepath);
    num_faces = 0;
    num_vertices = 0;
  }

  Eigen::Vector3f computeNormal(const Streamline<float> &tck) {
    // copy coordinates to  matrix in Eigen format
    size_t num_atoms = tck.size();
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic> coord(3, num_atoms);
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

  void computeNormals(const Streamline<float> &tck, Streamline<float> &normals) {
    Eigen::Vector3f sPrev = (tck[1] - tck[0]).normalized();
    Eigen::Vector3f normal = Eigen::Vector3f::Zero();

    // Find a good starting normal
    for (size_t idx = 1; idx < tck.size() - 1; idx++) {
      const auto &pt1 = tck[idx];
      const auto &pt2 = tck[idx + 1];
      const auto sNext = (pt2 - pt1).normalized();
      const auto n = sPrev.cross(sNext);
      if (n.norm() > 1.0E-3) {
        normal = n;
        sPrev = sNext;
        break;
      }
    }

    normal.normalize(); // vtkPolyLine.cxx:170
    for (size_t idx = 0; idx < tck.size() - 1; idx++) {
      const auto &pt1 = tck[idx];
      const auto &pt2 = tck[idx + 1];
      const auto sNext = (pt2 - pt1).normalized();

      // compute rotation vector vtkPolyLine.cxx:187
      auto w = sPrev.cross(normal);
      if (w.norm() == 0.0) {
        // copy the normal and continue
        normals.push_back(normal);
        continue;
      }
      // compute rotation of line segment
      auto q = sNext.cross(sPrev);
      if (q.norm() == 0.0) {
        // copy the normal and continue
        normals.push_back(normal);
        continue;
      }
      auto f1 = q.dot(normal);
      auto f2 = 1.0 - (f1 * f1);
      if (f2 > 0.0) {
        f2 = sqrt(1.0 - (f1 * f1));
      } else {
        f2 = 0.0;
      }

      auto c = (sNext + sPrev).normalized();
      w = c.cross(q);
      c = sPrev.cross(q);
      if ((normal.dot(c) * w.dot(c)) < 0) {
        f2 = -1.0 * f2;
      }
      normals.push_back(normal);
      sPrev = sNext;
      normal = (f1 * q) + (f2 * w);
    }
  }

  bool operator()(const Streamline<float> &intck) {
    // Need at least 5 points, silently ignore...
    if (intck.size() < static_cast<size_t>(increment) * 3) {
      return true;
    }

    auto nSides = sides;
    Eigen::MatrixXf coords(nSides, 2);
    Eigen::MatrixXi faces(nSides, 6);
    auto theta = 2.0 * Math::pi / static_cast<default_type>(nSides);
    for (auto i = 0; i < nSides; i++) {
      coords(i, 0) = cos(static_cast<default_type>(i) * theta);
      coords(i, 1) = sin(static_cast<default_type>(i) * theta);
      // Face offsets
      faces(i, 0) = i;
      faces(i, 1) = (i + 1) % nSides;
      faces(i, 2) = i + nSides;
      faces(i, 3) = (i + 1) % nSides;
      faces(i, 4) = (i + 1) % nSides + nSides;
      faces(i, 5) = i + nSides;
    }

    // to handle the increment, we want to keep the first 2 and last 2 points, but we can skip inside
    Streamline<float> tck;

    // Push on the first 2 points
    tck.push_back(intck[0]);
    tck.push_back(intck[1]);
    for (size_t idx = 3; idx < intck.size() - 2; idx += increment) {
      tck.push_back(intck[idx]);
    }
    tck.push_back(intck[intck.size() - 2]);
    tck.push_back(intck[intck.size() - 1]);

    Streamline<float> normals;
    this->computeNormals(tck, normals);
    auto globalNormal = computeNormal(tck);
    Eigen::Vector3f sNext = tck[1] - tck[0];
    auto isFirst = true;
    for (size_t idx = 1; idx < tck.size() - 1; ++idx) {
      auto isLast = idx == tck.size() - 2;

      // vtkTubeFilter.cxx:386
      Eigen::Vector3f p = tck[idx];
      Eigen::Vector3f pNext = tck[idx + 1];
      Eigen::Vector3f sPrev = sNext;
      sNext = pNext - p;
      Eigen::Vector3f n = normals[idx];

      sNext.normalize();
      if (sNext.norm() == 0.0) {
        continue;
      }

      // Average vectors
      Eigen::Vector3f s = (sPrev + sNext) / 2.0;
      s.normalize();
      if (s.norm() == 0.0) {
        s = sPrev.cross(n).normalized();
      }

      auto T = s;
      auto N = T.cross(globalNormal).normalized();
      auto B = T.cross(N).normalized();
      N = B.cross(T).normalized();

      // have our coordinate frame, now add circles
      for (auto sideIdx = 0; sideIdx < nSides; sideIdx++) {
        auto sidePoint = p + radius * (N * coords(sideIdx, 0) + B * coords(sideIdx, 1));
        vertexOF << sidePoint[0] << " " << sidePoint[1] << " " << sidePoint[2] << " ";
        vertexOF << (int)(255 * fabs(T[0])) << " " << (int)(255 * fabs(T[1])) << " " << (int)(255 * fabs(T[2])) << "\n";
        if (!isLast) {
          faceOF << "3"
                 << " " << num_vertices + faces(sideIdx, 0) << " " << num_vertices + faces(sideIdx, 1) << " "
                 << num_vertices + faces(sideIdx, 2) << "\n";
          faceOF << "3"
                 << " " << num_vertices + faces(sideIdx, 3) << " " << num_vertices + faces(sideIdx, 4) << " "
                 << num_vertices + faces(sideIdx, 5) << "\n";
          num_faces += 2;
        }
      }
      // Cap the first point, remebering the right hand rule
      if (isFirst) {
        for (auto sideIdx = nSides - 1; sideIdx >= 2; --sideIdx) {
          faceOF << "3"
                 << " " << num_vertices + sideIdx << " " << num_vertices + sideIdx - 1 << " " << num_vertices << "\n";
        }
        num_faces += nSides - 2;
        isFirst = false;
      }
      if (isLast) {
        for (auto sideIdx = 2; sideIdx <= nSides - 1; ++sideIdx) {
          faceOF << "3"
                 << " " << num_vertices + sideIdx - 1 << " " << num_vertices + sideIdx << " " << num_vertices << "\n";
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

      out << "ply\n"
             "format ascii 1.0\n"
             "comment written by tckconvert v"
          << App::mrtrix_version
          << "\n"
             "comment part of the mtrix3 suite of tools (http://www.mrtrix.org/)\n"
             "comment the coordinate system and scale is taken from directly from the input and is not adjusted\n"
             "element vertex "
          << num_vertices
          << "\n"
             "property float32 x\n"
             "property float32 y\n"
             "property float32 z\n"
             "property uchar red\n"
             "property uchar green\n"
             "property uchar blue\n"
             "element face "
          << num_faces
          << "\n"
             "property list uint8 int32 vertex_indices\n"
             "end_header\n";

      std::ifstream vertexIF(vertexFilepath);
      out << vertexIF.rdbuf();
      vertexIF.close();
      std::filesystem::remove(vertexFilepath);

      std::ifstream faceIF(faceFilepath);
      out << faceIF.rdbuf();
      faceIF.close();
      std::filesystem::remove(faceFilepath);

      out.close();
    } catch (Exception &e) {
      e.display();
      App::exit_error_code = 1;
    }
  }

private:
  std::filesystem::path vertexFilepath;
  std::filesystem::path faceFilepath;
  File::OFStream out;
  File::OFStream vertexOF;
  File::OFStream faceOF;
  size_t num_vertices;
  size_t num_faces;
  int increment;
  float radius;
  int sides;
};

class RibWriter : public WriterInterface<float> {
public:
  RibWriter(const std::filesystem::path &path, float radius = 0.1, bool dec = false)
      : out(path), writeDEC(dec), radius(radius), hasPoints(false), wroteHeader(false) {
    pointsFilepath = File::create_tempfile(0, ".points");
    pointsOF.open(pointsFilepath);
    pointsOF << "\"P\" [";
    decFilepath = File::create_tempfile(0, ".dec");
    decOF.open(decFilepath);
    decOF << "\"varying color dec\" [";
    // Header
    out << "##RenderMan RIB\n"
        << "# Written by tckconvert\n"
        << "# Part of the MRtrix package (http://mrtrix.org)\n"
        << "# version: " << App::mrtrix_version << "\n";
  }

  bool operator()(const Streamline<float> &tck) {
    if (tck.size() < 3) {
      return true;
    }

    hasPoints = true;
    if (!wroteHeader) {
      wroteHeader = true;
      // Start writing the header
      out << "Basis \"catmull-rom\" 1 \"catmull-rom\" 1\n"
          << "Attribute \"dice\" \"int roundcurve\" [1] \"int hair\" [1]\n"
          << "Curves \"linear\" [";
    }
    out << tck.size() << " ";
    Eigen::Vector3f prev = tck[1];
    for (auto pt : tck) {
      pointsOF << pt[0] << " " << pt[1] << " " << pt[2] << " ";
      // Should we write the dec?
      if (writeDEC) {
        Eigen::Vector3f T = (prev - pt).normalized();
        decOF << fabs(T[0]) << " " << fabs(T[1]) << " " << fabs(T[2]) << " ";
        prev = pt;
      }
    }
    return true;
  }

  ~RibWriter() {
    try {

      if (hasPoints) {
        pointsOF << "]\n";
        decOF << "]\n";
      }

      pointsOF.close();
      decOF.close();

      if (hasPoints) {
        out << "] \"nonperiodic\" ";

        std::ifstream pointsIF(pointsFilepath);
        out << pointsIF.rdbuf();

        if (writeDEC) {
          std::ifstream decIF(decFilepath);
          out << decIF.rdbuf();
          decIF.close();
        }

        out << " \"constantwidth\" " << radius << "\n";
      }

      out.close();

      std::filesystem::remove(pointsFilepath);
      std::filesystem::remove(decFilepath);

    } catch (Exception &e) {
      e.display();
      App::exit_error_code = 1;
    }
  }

private:
  std::filesystem::path pointsFilepath;
  std::filesystem::path decFilepath;
  File::OFStream out;
  File::OFStream pointsOF;
  File::OFStream decOF;
  bool writeDEC;
  float radius;
  bool hasPoints;
  bool wroteHeader;
};

//! \brief resolve the (optional) point-position transform from the CLI options.
/*! The four scanner/voxel/image transform options are mutually exclusive; the
 * identity transform is returned when none is given. Shared verbatim by both
 * conversion branches so the transform behaviour is independent of the I/O
 * path selected. */
transform_type get_transform() {
  transform_type T;
  T.setIdentity();
  size_t nopts = 0;
  auto opt = get_options("scanner2voxel");
  if (!opt.empty()) {
    auto header = Header::open(opt[0][0]);
    T = Transform(header).scanner2voxel;
    nopts++;
  }
  opt = get_options("scanner2image");
  if (!opt.empty()) {
    auto header = Header::open(opt[0][0]);
    T = Transform(header).scanner2image;
    nopts++;
  }
  opt = get_options("voxel2scanner");
  if (!opt.empty()) {
    auto header = Header::open(opt[0][0]);
    T = Transform(header).voxel2scanner;
    nopts++;
  }
  opt = get_options("image2scanner");
  if (!opt.empty()) {
    auto header = Header::open(opt[0][0]);
    T = Transform(header).image2scanner;
    nopts++;
  }
  if (nopts > 1) {
    throw Exception("Transform options are mutually exclusive.");
  }
  return T;
}

namespace {

//! \brief one parsed sidecar-manipulation operation (each role-qualified, by name).
struct ExtractOp {
  FieldRole role;
  std::string name;
  std::filesystem::path path;
};
struct InsertOp {
  FieldRole role;
  std::string name;
  std::filesystem::path path;
};
struct RenameOp {
  FieldRole role;
  std::string old_name;
  std::string new_name;
};
struct RemoveOp {
  FieldRole role;
  std::string name;
};
struct ConvertOp {
  FieldRole role;
  std::string name;
  DataType dtype;
};

//! \brief the full set of sidecar-manipulation operations requested on the command line.
struct SidecarPlan {
  std::vector<ExtractOp> extracts;
  std::vector<InsertOp> inserts;
  std::vector<RenameOp> renames;
  std::vector<RemoveOp> removes;
  std::vector<ConvertOp> converts;
  bool empty() const {
    return extracts.empty() && inserts.empty() && renames.empty() && removes.empty() && converts.empty();
  }
};

//! \brief functor that asserts a DataType is a representable sidecar element type.
/*! dispatch_sidecar_datatype() throws for an unsupported datatype, so invoking it
 * with this no-op probe validates a "-convert" target up front. */
struct SidecarDataTypeProbe {
  template <typename T> bool operator()() const { return true; }
};

//! \brief parse the role-qualified sidecar options into a SidecarPlan (§2.4).
SidecarPlan parse_sidecar_plan() {
  SidecarPlan plan;
  // The "file" argument is a filesystem-path type, so it converts directly to
  //   std::filesystem::path (never via std::string, which trips the pure-filesystem
  //   argument assertion); "type"/"name" are non-filesystem and read as text.
  for (const auto &opt : get_options("extract"))
    plan.extracts.push_back(
        {to_field_role(Enum::from_name<SidecarType>(opt[0])), std::string(opt[1]), std::filesystem::path(opt[2])});
  for (const auto &opt : get_options("insert"))
    plan.inserts.push_back(
        {to_field_role(Enum::from_name<SidecarType>(opt[0])), std::string(opt[1]), std::filesystem::path(opt[2])});
  for (const auto &opt : get_options("rename"))
    plan.renames.push_back(
        {to_field_role(Enum::from_name<SidecarType>(opt[0])), std::string(opt[1]), std::string(opt[2])});
  for (const auto &opt : get_options("remove"))
    plan.removes.push_back({to_field_role(Enum::from_name<SidecarType>(opt[0])), std::string(opt[1])});
  for (const auto &opt : get_options("convert")) {
    DataType dtype = DataType::parse(std::string(opt[2]));
    dtype.set_byte_order_native();
    dispatch_sidecar_datatype(dtype, SidecarDataTypeProbe{});
    plan.converts.push_back({to_field_role(Enum::from_name<SidecarType>(opt[0])), std::string(opt[1]), dtype});
  }
  return plan;
}

//! \brief carry of one input field to its output slot, with an optional dtype recast.
struct FieldCarry {
  size_t in_ordinal;
  size_t out_ordinal;
  std::optional<DataType> convert;
};

//! \brief the output field registry plus per-role carry maps derived from a plan.
struct SidecarTransform {
  FieldRegistry output_registry;
  std::vector<FieldCarry> dps_carry;
  std::vector<FieldCarry> dpv_carry;
};

//! \brief derive the output field set and the per-item carry maps from \a input + \a plan.
/*! Applies "-remove" (drops the field), "-rename" (changes its name) and
 * "-convert" (changes its on-disk datatype, recast per item) to the input field
 * registry, producing the output registry and, per role, the (input ordinal →
 * output ordinal [, recast]) carry list. Inserted fields are already part of
 * \a input (registered as input loaders), so they are carried like any other.
 * Every referenced field is validated to exist (and renames not to collide) up
 * front, so a bad option fails before any output is written. */
SidecarTransform build_transform(const FieldRegistry &input, const SidecarPlan &plan) {
  const auto exists = [&](const std::string_view name, const FieldRole role) {
    return input.find(name, role) != nullptr;
  };
  for (const RemoveOp &op : plan.removes)
    if (!exists(op.name, op.role))
      throw Exception(std::string("cannot remove ") + role_word(op.role) + " sidecar field \"" + op.name +
                      "\": no such field in the input tractogram");
  for (const ConvertOp &op : plan.converts)
    if (!exists(op.name, op.role))
      throw Exception(std::string("cannot convert ") + role_word(op.role) + " sidecar field \"" + op.name +
                      "\": no such field in the input tractogram");
  for (const RenameOp &op : plan.renames) {
    if (!exists(op.old_name, op.role))
      throw Exception(std::string("cannot rename ") + role_word(op.role) + " sidecar field \"" + op.old_name +
                      "\": no such field in the input tractogram");
    if (op.new_name != op.old_name && exists(op.new_name, op.role))
      throw Exception(std::string("cannot rename ") + role_word(op.role) + " sidecar field \"" + op.old_name +
                      "\" to \"" + op.new_name + "\": a field of that name already exists");
  }

  SidecarTransform transform;
  for (const FieldDescriptor &field : input) {
    const bool removed = std::any_of(plan.removes.begin(), plan.removes.end(), [&](const RemoveOp &op) {
      return op.role == field.role && op.name == field.name;
    });
    if (removed)
      continue;
    FieldDescriptor out = field;
    out.source = FieldSource::Internal;
    for (const RenameOp &op : plan.renames)
      if (op.role == field.role && op.old_name == field.name)
        out.name = op.new_name;
    std::optional<DataType> convert;
    for (const ConvertOp &op : plan.converts)
      if (op.role == field.role && op.name == field.name) {
        convert = op.dtype;
        out.dtype = op.dtype;
      }
    const size_t out_ordinal = transform.output_registry.add(out);
    const FieldCarry carry{field.ordinal, out_ordinal, convert};
    if (field.role == FieldRole::DPV)
      transform.dpv_carry.push_back(carry);
    else if (field.role == FieldRole::DPS)
      transform.dps_carry.push_back(carry);
  }
  return transform;
}

} // namespace

//! \brief generic conversion through the Tractogram format-handler framework.
/*! Used when both the input and output extensions are recognised by the
 * framework's handler list (Stage 1). Constructs an input and an output
 * Tractogram and copies the full composite TractogramItem single-threaded (pure
 * I/O; no thread queue), applying the point-position transform per vertex.
 *
 * The conversion is sidecar-aware (Stage 10): every dps/dpv field the input
 * carries is declared on the output and copied across in its native dtype, except
 * as redirected by \a plan — "-insert" adds a new field from a standalone file,
 * "-remove"/"-rename"/"-convert" drop/rename/recast an existing field, and
 * "-extract" taps an input field out to a standalone file (independent of whether
 * it is also carried). With an empty \a plan this is a verbatim sidecar copy. */
void run_generic(const std::filesystem::path &input_path,
                 const std::filesystem::path &output_path,
                 const SidecarPlan &plan) {
  Properties properties;
  auto input = Tractogram<float>::open(input_path, properties);

  // "-insert": register each new field as a named input loader, so its values flow
  //   into the streaming items and the field joins the input registry (carried to
  //   the output like any internal field).
  for (const InsertOp &op : plan.inserts)
    input.register_named_input_sidecar(op.role, op.name, op.path, properties);

  const SidecarTransform transform = build_transform(input.fields(), plan);

  // Resolve "-extract" targets to input payload ordinals before any output is
  //   created, so an unknown field name fails before writing anything.
  struct ExtractTarget {
    FieldRole role;
    size_t ordinal;
    std::filesystem::path path;
  };
  std::vector<ExtractTarget> extract_targets;
  for (const ExtractOp &op : plan.extracts) {
    const std::optional<size_t> ordinal = input.fields().ordinal(op.name, op.role);
    if (!ordinal.has_value())
      throw Exception(std::string("cannot extract ") + role_word(op.role) + " sidecar field \"" + op.name +
                      "\": no such field in the input tractogram");
    extract_targets.push_back({op.role, *ordinal, op.path});
  }

  // A ".tsf" extracted from per-vertex data must share a "timestamp" with the
  //   output tractogram (track-scalar validation). Stamp a fresh shared value
  //   before creating the output; formats that re-stamp on write (".tck", the
  //   pipe) overwrite it, and the exporters created afterwards inherit whichever
  //   value the output settled on, so the pair always matches.
  const bool any_dpv_extract = std::any_of(
      plan.extracts.begin(), plan.extracts.end(), [](const ExtractOp &op) { return op.role == FieldRole::DPV; });
  if (any_dpv_extract)
    properties.set_timestamp();

  auto output = Tractogram<float>::create(output_path, properties, transform.output_registry);

  // "-extract" exporters tap the INPUT item, independent of whether the field is
  //   also carried to (or dropped from) the output.
  std::vector<std::unique_ptr<SidecarExporter<float>>> extractors;
  for (const ExtractTarget &target : extract_targets)
    extractors.push_back(make_named_sidecar_exporter<float>(target.role, target.ordinal, target.path, properties));

  const transform_type T = get_transform();

  TractogramItem<float> in_item;
  TractogramItem<float> out_item;
  while (input.read(in_item)) {
    for (auto &extractor : extractors)
      (*extractor)(in_item);
    out_item.clear();
    out_item.streamline = in_item.streamline;
    for (auto &pos : out_item.streamline)
      pos = T.cast<float>() * pos;
    out_item.dps.resize(transform.output_registry.dps_count());
    out_item.dpv.resize(transform.output_registry.dpv_count());
    for (const FieldCarry &carry : transform.dps_carry)
      out_item.dps[carry.out_ordinal] = carry.convert.has_value()
                                            ? convert_dps_value(in_item.dps[carry.in_ordinal], *carry.convert)
                                            : in_item.dps[carry.in_ordinal];
    for (const FieldCarry &carry : transform.dpv_carry)
      out_item.dpv[carry.out_ordinal] = carry.convert.has_value()
                                            ? convert_dpv_value(in_item.dpv[carry.in_ordinal], *carry.convert)
                                            : in_item.dpv[carry.in_ordinal];
    output.write(out_item);
  }
  for (auto &extractor : extractors)
    extractor->finalise();
}

//! \brief bespoke conversion for esoteric / export-only formats.
/*! The original tckconvert reader/writer selection, retained verbatim for the
 * formats intentionally not exposed to other commands as framework handlers:
 * the ".txt" ASCII reader/writer and the write-only ".ply" and ".rib"
 * exporters. Selected as a fallback whenever the framework does not recognise
 * either of the two extensions. */
void run_bespoke(const std::filesystem::path &input_path, const std::filesystem::path &output_path) {
  // Reader
  Properties properties;
  std::unique_ptr<ReaderInterface<float>> reader;
  if (input_path.extension() == ".tck") {
    reader.reset(new TCKReader<float>(input_path, properties));
  } else if (input_path.extension() == ".txt") {
    reader.reset(new ASCIIReader(input_path.string()));
  } else {
    throw Exception("Unsupported input file type.");
  }

  // Writer
  std::unique_ptr<WriterInterface<float>> writer;
  if (output_path.extension() == ".tck") {
    writer.reset(new TCKWriter<float>(output_path, properties));
  } else if (output_path.extension() == ".ply") {
    const int increment = get_option_value("increment", default_ply_increment);
    const float radius = get_option_value("radius", default_ply_radius);
    const int sides = get_option_value("sides", default_ply_sides);
    writer.reset(new PLYWriter(output_path, increment, radius, sides));
  } else if (output_path.extension() == ".rib") {
    writer.reset(new RibWriter(output_path));
  } else if (output_path.extension() == ".txt") {
    writer.reset(new ASCIIWriter(output_path.string()));
  } else {
    throw Exception("Unsupported output file type.");
  }

  const transform_type T = get_transform();

  // Copy
  Streamline<float> tck;
  while ((*reader)(tck)) {
    for (auto &pos : tck) {
      pos = T.cast<float>() * pos;
    }
    (*writer)(tck);
  }
}

void run() {
  std::filesystem::path input_path{argument[0]};
  std::filesystem::path output_path{argument[1]};

  const SidecarPlan plan = parse_sidecar_plan();

  // First attempt the generic framework branch: it serves the conversion only
  // when both extensions are recognised by the format-handler framework
  // (".tck"/".trk"/TRX/".vtk"/...). Otherwise fall back to the bespoke handlers,
  // retained for the esoteric / export-only formats (".txt"/".ply"/".rib").
  const bool input_is_framework = MR::DWI::Tractography::Formats::get_handler(input_path) != nullptr;
  const bool output_is_framework = MR::DWI::Tractography::Formats::get_handler(output_path) != nullptr;
  if (input_is_framework && output_is_framework) {
    run_generic(input_path, output_path, plan);
  } else {
    if (!plan.empty())
      throw Exception("embedded sidecar manipulation (-extract / -insert / -rename / -remove / -convert)"
                      " is only available when converting between framework tractography formats"
                      " (\".tck\", \".trk\", TRX, \".vtk\", \".vtx\", \".qfib\", \".zfib\");"
                      " the \".txt\" / \".ply\" / \".rib\" paths do not carry sidecar data");
    run_bespoke(input_path, output_path);
  }
}
