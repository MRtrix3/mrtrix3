#include "command.h"
#include "image.h"
#include "algo/loop.h"
#include "math/math.h"

#include "transform.h"
#include <Eigen/Geometry> 
#include <Eigen/SVD> 

// #include "registration/transform/compose.h"

using namespace MR;
using namespace App;

const char* space_choices[] = { "voxel", "image1", "image2", "average", NULL };

void usage ()
{
  AUTHOR = "Maximilian Pietsch (maximilian.pietsch@kcl.ac.uk)";

  DESCRIPTION
  + "computes the dissimiliarty metric between two transformations. Currently only affine transformations and the mean absolute displacement metric are implemented";

  ARGUMENTS
  + Argument ("image", 
        "the image that defines the space over which the "
        "dissimilarity is measured.").type_image_in ();

  OPTIONS
  + Option ("linear", 
        "specify a 4x4 linear transform to apply, in the form "
        "of a 4x4 ascii file. Note the standard 'reverse' convention "
        "is used, where the transform maps points in the template image "
        "to the moving image.")
    +   Argument ("transform").type_file_in ()

  + Option ("linear_inverse",
         "invert linear transformation")

  + Option ("linear2", 
        "TODO: compare transformations")
    +   Argument ("transform").type_file_in ()

  + Option ("template",
        "use the voxel to scanner transformation of the template image instead "
        "of the linear transformation. ")
    +   Argument ("transformation").type_image_in() 

  + Option ("voxel",
         "normalise dissimiliarity to voxel size of image")

  + Option ("norm",
         "display the norm of the displacement.");

}

typedef double value_type;

void run ()
{
  auto image = Image<value_type>::open (argument[0]);
  auto trafo = Transform(image.original_header()); // #.transform();
  // auto voxel2scanner = trafo.voxel2scanner();
  auto cost = Eigen::Matrix<value_type, 3, 1>(0,0,0) ;
  size_t n_voxels = image.size(0) * image.size(1) * image.size(2);

  bool do_linear = false;
  transform_type affine_trafo;
  auto opt = get_options ("linear");
  if (opt.size()){
    do_linear = true;
    affine_trafo = load_transform<value_type> (opt[0][0]);
  }
  opt = get_options ("template");
  if (opt.size()){
    if (do_linear) throw Exception ("linear transformation and template image provided ");
    do_linear = true;
    auto H = Header::open(opt[0][0]);
    // check_dimensions (image, H);
    // affine_trafo = Transform(H).voxel2scanner;
    throw Exception ("template image not yet implemented");
  }

  transform_type reference_trafo;
  reference_trafo.setIdentity();
  opt = get_options ("linear2");
  if (opt.size())
    reference_trafo = load_transform<value_type> (opt[0][0]);

  bool normalise = get_options ("normalise").size();
  bool norm = get_options ("norm").size();



  if (!do_linear)
    throw Exception ("TODO: only linear transformation implemented but no linear transformation provided.");
  
  Eigen::Matrix<value_type, 3, 1> pos;
  Eigen::Matrix<value_type, 3, 1> displacement;
  
  reference_trafo = reference_trafo * trafo.voxel2scanner;
  if (get_options ("linear_inverse").size())
    affine_trafo = affine_trafo.inverse();
  auto trafo2 = affine_trafo * trafo.voxel2scanner;
  DEBUG("reference voxel2scanner transformation:\n" + str (reference_trafo.matrix()));
  DEBUG("voxel2scanner transformation:\n" + str (trafo2.matrix()));

  for (auto i = Loop() (image); i ;++i){
    pos << image.index(0), image.index(1), image.index(2);
    displacement = trafo2 * pos - reference_trafo * pos;
    cost += displacement.cwiseAbs();
  }
  
  cost /= static_cast<value_type>(n_voxels);
  if (normalise){
    cost[0] /= static_cast<value_type>(image.spacing(0));
    cost[1] /= static_cast<value_type>(image.spacing(1));
    cost[2] /= static_cast<value_type>(image.spacing(2));
  }
  if (norm)
    std::cout << cost.norm() << std::endl;
  else
    std::cout << cost[0] << " " << cost[1] << " " << cost[2] << std::endl;
}

