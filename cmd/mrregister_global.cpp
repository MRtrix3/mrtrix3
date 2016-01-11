#include <vector>
#include <iostream>     // std::streambuf

#include "command.h"
#include "image.h"
#include "filter/reslice.h"
#include "interp/cubic.h"
#include "transform.h"
#include "registration/linear.h"
#include "registration/syn.h"
#include "registration/metric/syn_demons.h"
#include "registration/metric/mean_squared.h"
#include "registration/metric/difference_robust.h"
#include "registration/metric/difference_robust_4D.h"
#include "registration/metric/cross_correlation.h"
#include "registration/metric/mean_squared_4D.h"
#include "registration/transform/affine.h"
#include "registration/transform/rigid.h"
#include "dwi/directions/predefined.h"
#include "math/SH.h"
#include "image/average_space.h"

#include "filter/resize.h"
#include "algo/threaded_loop.h"
#include "interp/linear.h"
#include "interp/nearest.h"
#include "registration/metric/params.h"
#include "registration/metric/evaluate.h"
#include "registration/transform/initialiser.h"
#include "math/gradient_descent.h"
#include "math/check_gradient.h"
#include "math/rng.h"
#include "math/math.h"

#include "registration/transform/global_search.h"

using namespace MR;
using namespace App;

const char* transformation_choices[] = { "rigid", "affine", "syn", "rigid_affine", "rigid_syn", "affine_syn", "rigid_affine_syn", NULL };


void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au) & Max Pietsch (maximilian.pietsch@kcl.ac.uk)";

  ARGUMENTS
    + Argument ("image1", "input image 1 ('moving')").type_image_in ()
    + Argument ("image2", "input image 2 ('template')").type_image_in ();

}

typedef double value_type;

void load_image (std::string filename, size_t num_vols, Image<value_type>& image) {
  auto temp_image = Image<value_type>::open (filename);
  auto header = Header::open (filename);
  header.datatype() = DataType::from_command_line (DataType::Float32);
  if (num_vols > 1) {
    header.size(3) = num_vols;
    header.stride(0) = 2;
    header.stride(1) = 3;
    header.stride(2) = 4;
    header.stride(3) = 1;
  }
  image = Image<value_type>::scratch (header);
  if (num_vols > 1) {
    for (auto i = Loop ()(image); i; ++i) {
      assign_pos_of (image).to (temp_image);
        image.value() = temp_image.value();
    }
  } else {
    threaded_copy (temp_image, image);
  }
}

void run() {

    const auto im1_header = Header::open (argument[0]);
    const auto im2_header = Header::open (argument[1]);

    Image<value_type> im1_image;
    Image<value_type> im2_image;
    Image<value_type> im1_mask;
    Image<value_type> im2_mask;

    load_image (argument[0], 1, im1_image);
    load_image (argument[1], 1, im2_image);

    typedef Registration::Metric::MeanSquared MetricType;
    typedef Registration::Transform::Affine TrafoType;

    MetricType metric;
    TrafoType transform;

    Registration::GlobalSearch transformation_search;
    transformation_search.run_masked(metric, transform, im1_image, im2_image, im1_mask, im2_mask);
    transform.debug();
}
