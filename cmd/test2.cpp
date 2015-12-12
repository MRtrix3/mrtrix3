#include "command.h"
#include "debug.h"
#include "image.h"
#include "registration/transform/affine.h"
#include "registration/transform/rigid.h"
#include "math/median.h"

namespace MR {
}


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "Joe Bloggs (joe.bloggs@acme.org)";

  DESCRIPTION
  + "test ";

  ARGUMENTS
  + Argument ("bogus", "float").type_float ();
}


void run ()
{
  // Registration::Transform::Affine A;

  Eigen::Matrix<default_type, 3, 5> points;
  points << 3.37788995,   3.45745724,   3.38547503,   3.3259503 ,   3.37706281,
          1.73255279,   1.61154894,   1.74094915,   1.6341395 ,   1.65484647,
        -27.13338627, -26.99696195, -27.12497958, -27.29667296, -27.20756166;
  VAR(points);


  Eigen::Matrix<default_type, 3, 1> median;
  Math::median_weiszfeld(points, median);
  VAR(median.transpose());


  Eigen::Matrix<default_type, 3, 4> corners;
  corners <<    1.0,    0, -Math::sqrt1_2,
               -1.0,    0, -Math::sqrt1_2,
                  0,  1.0,  Math::sqrt1_2,
                  0, -1.0,  Math::sqrt1_2;
  corners *= 10.0;
  VAR(corners);


  transform_type trafo;
  trafo.matrix() << 1.04059000e+00,  -6.30162000e-02,  -2.45339000e-02,   2.50705000e+00,
                    1.20565000e-01,   9.60553000e-01,  -2.97240000e-01,   2.58577000e+00,
                    5.45821000e-02,   2.64435000e-01,   8.83170000e-01,  -2.72320000e+01;

  Eigen::Matrix<default_type, 3, 4> corners_dash;
  for (size_t j = 0; j<4; ++j){
    corners_dash.col(j) = trafo * corners.col(j);
  }
  VAR(corners);
  VAR(corners_dash);
  // geometric_median3(corners_dash, median);
  // VAR(median.transpose());
  // Eigen::Matrix3f m = Eigen::Matrix3f::Random();
  // Eigen::Matrix3f y = Eigen::Matrix3f::Random();
  // std::cout << "Here is the matrix m:" << std::endl << m << std::endl;
  // std::cout << "Here is the matrix y:" << std::endl << y << std::endl;
  // Eigen::Matrix3f x;
  // x = m.colPivHouseholderQr().solve(y);
  // assert(y.isApprox(m*x));
  // std::cout << "Here is a solution x to the equation mx=y:" << std::endl << x << std::endl;

}
