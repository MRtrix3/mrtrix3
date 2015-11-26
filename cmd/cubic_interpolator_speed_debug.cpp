#define FLUSH_TO_ZERO
#include <cmath>
#include <cfloat>
#include "command.h"
#include "debug.h"
#include "image.h"
#include "interp/cubic.h"
#include "math/cubic_spline.h"
#include "transform.h"
// _mm_setcsr(_mm_getcsr() | 0x8040);
namespace MR {
}


using std::fpclassify;
using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "Joe Bloggs (joe.bloggs@acme.org)";

  DESCRIPTION
  + "test cubic interpolator speed";

  ARGUMENTS
  + Argument ("in", "the input image.").type_image_in ();
}


void run ()
{
  typedef float T;
  auto input = Image<T>::open (argument[0]);
  typedef Image<T> ImageType;
  typedef Math::HermiteSpline<T> SplineType;

  auto trafo = Transform(input.original_header());
  auto where = Eigen::Matrix<double,3,1>  (1.0e-14, 1.0e-14, 1.0e-14); // slow
  // auto where = Eigen::Matrix<double,3,1>  (1.0e-30, 1.0e-30, 1.0e-30); // fast
  // auto where = Eigen::Matrix<double,3,1>  (0.1, 0.1, 0.1); // fast

  Interp::SplineInterp<ImageType, Math::HermiteSpline<T>, Math::SplineProcessingType::ValueAndGradient> interp (input, 0.0);

  // SplineType H[3]{SplineType(Math::SplineProcessingType::ValueAndGradient), SplineType(Math::SplineProcessingType::ValueAndGradient), SplineType(Math::SplineProcessingType::ValueAndGradient)};

  if (fpclassify(DBL_MIN/2) == FP_SUBNORMAL){
    CONSOLE("double subnormals exist. here is one:");
    CONSOLE(str(DBL_MIN/2));
  }
  if (fpclassify(FLT_MIN/2) == FP_SUBNORMAL){
    CONSOLE("float subnormals exist. here is one:");
    CONSOLE(str(FLT_MIN/2));
  }

  ssize_t howoften = 10000000;
  Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic> bogus;
  ProgressBar progress ("...", howoften);
  for (ssize_t i=0; i<howoften; i++){
    // trafo.is_out_of_bounds(where);
    // trafo.set_to_nearest_test(where);
    // H[0].set(where[0]);
    // H[1].set(where[1]);
    // H[2].set(where[2]);

    // bogus = H[0].weights.cwiseProduct(H[1].weights);
    // bogus = H[2].weights.cwiseProduct(H[1].weights);

    // if(fpclassify(H[0].weights[0]) == FP_SUBNORMAL or fpclassify(H[0].weights[1]) == FP_SUBNORMAL or fpclassify(H[2].weights[0]) == FP_SUBNORMAL) std::cerr << H[0].weights << std::endl;
    // H[1].set(9.9475983006414026e-14);
    // H[2].set(9.9475983006414026e-14);
    interp.voxel(where);
    ++progress;
  }
  // VAR(H[0].weights);

}
