#include "command.h"
#include "point.h"
#include "math/SH.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "math/vector.h"
#include "image/loop.h"


using namespace MR; 
using namespace App; 

void usage () {
  DESCRIPTION
    + "generate a planar ROI.";

  ARGUMENTS
    + Argument ("point1",
        "a point on the plane, supplied as comma-separated "
        "3-vector of floating-point values, corresponding to the real/scanner "
        "coordinates of the point in mm.")
    .type_sequence_float()

    + Argument ("point2",
        "a point on the plane, supplied as comma-separated "
        "3-vector of floating-point values, corresponding to the real/scanner "
        "coordinates of the point in mm.")
    .type_sequence_float()

    + Argument ("point3",
        "a point on the plane, supplied as comma-separated "
        "3-vector of floating-point values, corresponding to the real/scanner "
        "coordinates of the point in mm.")
    .type_sequence_float()

    + Argument ("template",
        "a template image used to compute the required extent of the ROI.")
    .type_file()

    + Argument ("ROI", "the output ROI image.").type_image_out();

  OPTIONS
    + Option ("vox", 
        "the desired (isotropic) voxel size (default is same as template).")
    + Argument ("size").type_float (0.0, 00.0, std::numeric_limits<float>::max());
}


inline Point<> get_bounds (const Point<>& corner, const Point<>& axis, const Point<>& ref, const Point<>& d1, const Point<>& d2) {
  Math::Matrix<float> M(3,3);
  Math::Vector<float> X(3);

  M(0,0) = d1[0]; M(0,1) = d2[0]; M(0,2) = axis[0]; 
  M(1,0) = d1[1]; M(1,1) = d2[1]; M(1,2) = axis[1]; 
  M(2,0) = d1[2]; M(2,1) = d2[2]; M(2,2) = axis[2]; 

  X[0] = corner[0] - ref[0];
  X[1] = corner[1] - ref[1];
  X[2] = corner[2] - ref[2];

  Math::Permutation p (3);
  int signum;
  Math::LU::decomp (M, p, signum); 
  Math::LU::solve (X, M, p);

  return Point<> (X[0], X[1], X[2]);
}



inline Point<> get_bounds (const Image::Header& header, Point<int> corner, int axis, const Point<>& ref, const Point<>& d1, const Point<>& d2) {
  const Math::Matrix<float>& T (header.transform());
  Point<> pos (T(0,3), T(1,3), T(2,3));
  for (size_t n = 0; n < 3; ++n) 
    if (corner[n]) 
      pos += header.dim(n) * header.vox(n) * Point<> (T(0,n), T(1,n), T(2,n));
  Point<> dir (T(0,axis), T(1,axis), T(2,axis));
  Point<> ret = get_bounds (pos, dir, ref, d1, d2);
  ret[2] /= -header.vox (axis) * header.dim (axis);
  return ret;
}


inline void update_bounds (float& min1, float& min2, float& max1, float& max2, const Image::Header& header, Point<int> corner, int axis, const Point<>& ref, const Point<>& d1, const Point<>& d2) {
  Point<> bound = get_bounds (header, corner, axis, ref, d1, d2);
  if (bound[2] >= 0.0 && bound[2] <= 1.0) {
    if (min1 > bound[0]) min1 = bound[0];
    if (min2 > bound[1]) min2 = bound[1];
    if (max1 < bound[0]) max1 = bound[0];
    if (max2 < bound[1]) max2 = bound[1];
  }
}





void run () {
  Point<> p[3];
  for (size_t arg = 0; arg < 3; ++arg) {
    std::vector<float> V = argument[arg]; 
    if (V.size() != 3) 
      throw Exception ("coordinates must contain 3 elements");
    p[arg][0] = V[0];
    p[arg][1] = V[1];
    p[arg][2] = V[2];
  }

  Image::Header header (argument[3]);
  float vox = Math::pow (header.vox(0)*header.vox(1)*header.vox(2), 1.0f/3.0f);
  Options opt = get_options ("vox");
  if (opt.size()) 
    vox = opt[0][0];


  Point<> d1 = p[1] - p[0];
  Point<> d2 = p[2] - p[0];
  d1.normalise();
  d2.normalise();

  d2 = d2 - d1 * d1.dot(d2);
  d2.normalise();

  Point<> d3 = d1.cross(d2);


  float min1 = std::numeric_limits<float>::infinity();
  float min2 = std::numeric_limits<float>::infinity();
  float max1 = -std::numeric_limits<float>::infinity();
  float max2 = -std::numeric_limits<float>::infinity();

  update_bounds (min1, min2, max1, max2, header, Point<int>(0,0,0), 0, p[0], d1, d2);
  update_bounds (min1, min2, max1, max2, header, Point<int>(0,0,0), 1, p[0], d1, d2);
  update_bounds (min1, min2, max1, max2, header, Point<int>(0,0,0), 2, p[0], d1, d2);
  update_bounds (min1, min2, max1, max2, header, Point<int>(1,0,0), 1, p[0], d1, d2);
  update_bounds (min1, min2, max1, max2, header, Point<int>(1,0,0), 2, p[0], d1, d2);
  update_bounds (min1, min2, max1, max2, header, Point<int>(0,1,0), 0, p[0], d1, d2);
  update_bounds (min1, min2, max1, max2, header, Point<int>(0,1,0), 2, p[0], d1, d2);
  update_bounds (min1, min2, max1, max2, header, Point<int>(0,0,1), 0, p[0], d1, d2);
  update_bounds (min1, min2, max1, max2, header, Point<int>(0,0,1), 1, p[0], d1, d2);
  update_bounds (min1, min2, max1, max2, header, Point<int>(1,1,0), 2, p[0], d1, d2);
  update_bounds (min1, min2, max1, max2, header, Point<int>(1,0,1), 1, p[0], d1, d2);
  update_bounds (min1, min2, max1, max2, header, Point<int>(0,1,1), 0, p[0], d1, d2);


  Point<> translation (p[0] + min1*d1 + min2*d2);

  header.transform()(0,0) = d1[0];
  header.transform()(1,0) = d1[1];
  header.transform()(2,0) = d1[2];

  header.transform()(0,1) = d2[0];
  header.transform()(1,1) = d2[1];
  header.transform()(2,1) = d2[2];

  header.transform()(0,2) = d3[0];
  header.transform()(1,2) = d3[1];
  header.transform()(2,2) = d3[2];

  header.transform()(0,3) = translation[0];
  header.transform()(1,3) = translation[1];
  header.transform()(2,3) = translation[2];

  header.set_ndim (3);
  header.dim (0) = (max1-min1) / vox;
  header.dim (1) = (max2-min2) / vox;
  header.dim (2) = 1;

  header.vox(0) = header.vox(1) = header.vox(2) = vox;

  header.datatype() = DataType::Bit;
  header.DW_scheme().clear();

  Image::Buffer<bool> roi_buffer (argument[4], header);
  Image::Buffer<bool>::voxel_type roi (roi_buffer);
  Image::LoopInOrder loop (roi);
  for (loop.start (roi); loop.ok(); loop.next (roi))
    roi.value() = true;

}


