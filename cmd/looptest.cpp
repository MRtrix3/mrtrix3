#include "command.h"
#include "debug.h"
#include "image.h"
#include "interp/cubic.h"
#include "algo/loop.h"
#include "algo/threaded_loop.h"
#include "algo/random_loop.h"
// #include <array>      // std::array
// #include <random>     // std::default_random_engine
// #include <chrono>     // std::chrono::system_clock

namespace MR {
}


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "Joe Bloggs (joe.bloggs@acme.org)";

  DESCRIPTION
  + "test loop";

  ARGUMENTS
  + Argument ("in", "the input image.").type_image_in ()
  + Argument ("sparsity", "sparsity.").type_float ()
  + Argument ("dense", "dense.").type_bool ();
}


void run ()
{
  auto input = Image<float>::open (argument[0]); // .with_direct_io (Stride::contiguous_along_axis(2));
  float sparsity = argument[1];
  // std::vector<ssize_t> a0(input.size(0));
  bool dense = argument[2];
  typedef Image<float> ImageType;
    size_t cnt = 0;
  if (dense){
    CONSOLE("dense");
    size_t seed = std::chrono::system_clock::now().time_since_epoch().count();
    auto engine = std::default_random_engine{seed};

    for (auto z = 0; z < 1000; ++z){
    auto loop1 = Random_loop<ImageType,decltype(engine)>(input, engine, 0, std::ceil((float) input.size(0) * sparsity));
    auto loop2 = Random_loop<ImageType,decltype(engine)>(input, engine, 1, std::ceil((float) input.size(1) * sparsity));
    auto loop3 = Random_loop<ImageType,decltype(engine)>(input, engine, 2, std::ceil((float) input.size(2) * sparsity));
      for (auto i = loop1; i; ++i)
        for (auto j = loop2; j; ++j)
          for (auto k = loop3; k; ++k)
            INFO(str(cnt++) + " " + str(input));
    }

    // std::vector<size_t> idx(input.size(0));
    // std::vector<size_t> jdx(input.size(1));
    // std::vector<size_t> kdx(input.size(2));
    // std::iota (std::begin(idx), std::end(idx), 0);
    // std::iota (std::begin(jdx), std::end(jdx), 0);
    // std::iota (std::begin(kdx), std::end(kdx), 0);
    // std::shuffle(std::begin(idx), std::end(idx), engine);
    // std::shuffle(std::begin(jdx), std::end(jdx), engine);
    // std::shuffle(std::begin(kdx), std::end(kdx), engine);

    // cnt = 0;
    // auto loop1 = Iterator_loop<ImageType, decltype(std::begin(idx))>(input,std::begin(idx),std::end(idx), 0, std::ceil((float) input.size(0) * sparsity));
    // auto loop2 = Iterator_loop<ImageType, decltype(std::begin(jdx))>(input,std::begin(jdx),std::end(jdx), 1, std::ceil((float) input.size(1) * sparsity));
    // auto loop3 = Iterator_loop<ImageType, decltype(std::begin(jdx))>(input,std::begin(kdx),std::end(kdx), 2, std::ceil((float) input.size(2) * sparsity));

    // while (true){
    //   if(loop1) ++loop1;
    //   if(loop2) ++loop2;
    //   if(loop3) ++loop3;
    //   INFO(str(cnt++) + " " + str(input));
    //   if (!loop1 and !loop2 and !loop3) break;
    // }

    // for (auto i = Iterator_loop<ImageType, decltype(std::begin(idx))>(input,std::begin(idx),std::end(idx), 0, std::ceil((float) input.size(0) * sparsity)); i; ++i){
    //   for (auto j = Iterator_loop<ImageType, decltype(std::begin(jdx))>(input,std::begin(jdx),std::end(jdx), 1, std::ceil((float) input.size(1) * sparsity)); j; ++j){
    //     for (auto k = Iterator_loop<ImageType, decltype(std::begin(jdx))>(input,std::begin(kdx),std::end(kdx), 2, std::ceil((float) input.size(2) * sparsity)); k; ++k){
    //       INFO(str(cnt++) + " " + str(input));
    //     }
    //   }
    // }
  } else {
    CONSOLE("sparse");
    // auto inner_loop = Loop(1, 3);
    for (auto z = 0; z < 1000; ++z){
      auto loop1 = Random_sparse_loop<ImageType>(input, 0, std::ceil((float) input.size(0) * sparsity));
      auto loop2 = Random_sparse_loop<ImageType>(input, 1, std::ceil((float)  input.size(1) * sparsity));
      auto loop3 = Random_sparse_loop<ImageType>(input, 2, std::ceil((float) input.size(2) * sparsity));
      for (auto i = loop1; i; ++i)
        for (auto j = loop2; j; ++j)
          for (auto j = loop3; j; ++j)
            INFO(str(cnt++) + " " + str(input)); // << std::endl;
    }



  }



  // size_t ni = input.size(0);
  // size_t nj = input.size(1);
  // size_t nk = input.size(2);
  // size_t n  = 20;
  // size_t seed = std::chrono::system_clock::now().time_since_epoch().count();
  // auto engine = std::default_random_engine{};

  // std::vector<size_t> idx(input.size(0));
  // std::vector<size_t> jdx(nj);
  // std::array<size_t,nj> jdx;
  // std::iota (std::begin(idx), std::end(idx), 0);
  // std::iota (std::begin(jdx), std::end(jdx), 0);
  // std::shuffle(std::begin(idx), std::end(idx), engine);
  // std::shuffle(std::begin(jdx), std::end(jdx), engine);

  // size_t cnt = 0;
  //

  // for (auto i = Volume_loop (input); i; ++i) {

  // while (cnt < n){
  //   input.index(0) = idx[cnt];
  //   input.index(1) = jdx[cnt];
  //   input.index(1) = jdx[cnt];
  //   for (auto j = inner_loop (mask, data); j; ++j)
  //   ++cnt;
  // }


//   Interp::SplineInterp<Image<float>, Math::UniformBSpline<float>, Math::SplineProcessingType::ValueAndGradient> interp (input, 0.0, false  );


//   Eigen::Vector3 voxel(46,41,29);
//   interp.voxel(voxel);
//   float value = 0;
//   Eigen::Matrix<float,1,3> gradient;
// //  std::cout << interp.gradient() << std::endl;

//   interp.value_and_gradient(value, gradient);

//   std::cout << value << std::endl;

//   std::cout << gradient << std::endl;
}
