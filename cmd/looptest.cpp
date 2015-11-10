#include "command.h"
#include "debug.h"
#include "image.h"
#include "interp/cubic.h"
#include "algo/loop.h"
#include "algo/threaded_loop.h"
#include "algo/random_threaded_loop2.h"
#include <numeric>
#include "algo/random_loop.h"
// #include <array>      // std::array
#include <random>     // std::default_random_engine
#include <chrono>     // std::chrono::system_clock
#include "timer.h"
#include "thread.h"

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
  + Argument ("density", "density.").type_float ()
  + Argument ("type", "dense, sparse, dense2.").type_integer ();
}

template <class ImageType>
  struct ThreadFunctor {
    public:
     ThreadFunctor (
      ImageType& image,
       const std::vector<size_t>& inner_axes,
       const default_type density,
       size_t& overall_count,
       Math::RNG& rng_engine) :
        image(image),
        inner_axes (inner_axes),
        density (density),
        overall_count (overall_count),
        cnt (0),
        rng (rng_engine) { assert(inner_axes.size() >= 2); }

      ~ThreadFunctor () {
        overall_count += cnt;
      }

      void operator() (const Iterator& iter) {
        auto engine = std::default_random_engine{static_cast<std::default_random_engine::result_type>(rng.get_seed())};
        DEBUG(str(iter));
        assign_pos_of(iter).to(image);
        // auto inner_loop2 = Loop(image, inner_axes[0]);
        auto inner_loop1 = Random_loop<ImageType,std::default_random_engine>(image, engine, inner_axes[1], std::ceil((float) image.size(inner_axes[1]) * density));
        for (auto j = inner_loop1; j; ++j)
          for (auto k = Loop (inner_axes[0]) (image); k; ++k){
            cnt += 1;
            INFO(str(image));
          }
      }
    private:
      ImageType image;
      std::vector<size_t> inner_axes;
      default_type density;
      size_t& overall_count;
      size_t cnt;
      Math::RNG rng;
  };


void run ()
{
  auto input = Image<float>::open (argument[0]); // .with_direct_io (Stride::contiguous_along_axis(2));
  float density = argument[1];
  // std::vector<ssize_t> a0(input.size(0));
  int type = argument[2];
  typedef Image<float> ImageType;
    size_t cnt = 0;
  if (type == 0){
    CONSOLE("dense");
    size_t seed = std::chrono::system_clock::now().time_since_epoch().count();
    auto engine = std::default_random_engine{static_cast<std::default_random_engine::result_type>(seed)};

    auto timer = Timer();
    auto loop1 = Random_loop<ImageType,decltype(engine)>(input, engine, 0, std::ceil((float) input.size(0) * 1.0));
    auto loop2 = Random_loop<ImageType,decltype(engine)>(input, engine, 1, std::ceil((float) input.size(1) * 1.0));
    auto loop3 = Random_loop<ImageType,decltype(engine)>(input, engine, 2, std::ceil((float) input.size(2) * density));
    for (auto i = loop1; i; ++i)
      for (auto j = loop2; j; ++j)
        for (auto k = loop3; k; ++k){
          cnt++;
          INFO(str(cnt) + " " + str(input));
        }
    CONSOLE(str(timer.elapsed()));
    VAR(cnt);

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
    // auto loop1 = Iterator_loop<ImageType, decltype(std::begin(idx))>(input,std::begin(idx),std::end(idx), 0, std::ceil((float) input.size(0) * density));
    // auto loop2 = Iterator_loop<ImageType, decltype(std::begin(jdx))>(input,std::begin(jdx),std::end(jdx), 1, std::ceil((float) input.size(1) * density));
    // auto loop3 = Iterator_loop<ImageType, decltype(std::begin(jdx))>(input,std::begin(kdx),std::end(kdx), 2, std::ceil((float) input.size(2) * density));

    // while (true){
    //   if(loop1) ++loop1;
    //   if(loop2) ++loop2;
    //   if(loop3) ++loop3;
    //   INFO(str(cnt++) + " " + str(input));
    //   if (!loop1 and !loop2 and !loop3) break;
    // }

    // for (auto i = Iterator_loop<ImageType, decltype(std::begin(idx))>(input,std::begin(idx),std::end(idx), 0, std::ceil((float) input.size(0) * density)); i; ++i){
    //   for (auto j = Iterator_loop<ImageType, decltype(std::begin(jdx))>(input,std::begin(jdx),std::end(jdx), 1, std::ceil((float) input.size(1) * density)); j; ++j){
    //     for (auto k = Iterator_loop<ImageType, decltype(std::begin(jdx))>(input,std::begin(kdx),std::end(kdx), 2, std::ceil((float) input.size(2) * density)); k; ++k){
    //       INFO(str(cnt++) + " " + str(input));
    //     }
    //   }
    // }
  } else if (type == 1) {
    CONSOLE("sparse");
    // auto inner_loop = Loop(1, 3);
    auto timer = Timer();
    size_t seed = std::chrono::system_clock::now().time_since_epoch().count();
    auto engine = std::default_random_engine{static_cast<std::default_random_engine::result_type>(seed)};
    auto loop1 = Random_loop<ImageType,decltype(engine)>(input, engine, 0, std::ceil((float) input.size(0) * 1.0));
    auto loop2 = Random_loop<ImageType,decltype(engine)>(input, engine, 1, std::ceil((float) input.size(1) * 1.0));
    auto loop3 = Random_sparse_loop<ImageType>(input, 2, std::ceil((float) input.size(2) * density));
    for (auto i = loop1; i; ++i)
      for (auto j = loop2; j; ++j)
        for (auto j = loop3; j; ++j){
          ++cnt;
          INFO(str(cnt) + " " + str(input)); // << std::endl;
        }
    CONSOLE(str(timer.elapsed()));
    VAR(cnt);
  } else if (type == 2) {
    CONSOLE ("random threaded counting");

    class MyThread {
      public:
        MyThread(size_t& count):
        overall_count (count),
        cnt(0) {}

        ~MyThread () {
          overall_count += cnt;
        }

        void operator() (const Iterator& iter) {
          // std::cerr << iter.index(0) << " " << iter.index(1) << " " << iter.index(2) << std::endl;
          // throw Exception ("stop");
          INFO(str(iter));
          ++cnt;
        }

      protected:
        size_t& overall_count;
        size_t cnt;
    };

    size_t cnt = 0;
    MyThread thread(cnt);
    std::vector<size_t> dimensions(3);
    dimensions[0] = input.size(0);
    dimensions[1] = input.size(1);
    dimensions[2] = input.size(2);
    VAR(density);
    auto timer = Timer();
    size_t n_repeats = 10;
    for (auto i = 1; i < n_repeats; ++i){
      RandomThreadedLoop (input, 0, 3).run (thread, density, dimensions);
    }
    CONSOLE(str(timer.elapsed()/n_repeats));
    cnt = (float) cnt / n_repeats;
    CONSOLE("actual density: " + str((float) cnt / (dimensions[0] * dimensions[1] * dimensions[2]) ));
    VAR(cnt);
  } else if (type == 3) {
    CONSOLE ("test");
    std::vector<size_t> dimensions(3);
    dimensions[0] = input.size(0); 
    dimensions[1] = input.size(1);
    dimensions[2] = input.size(2);
    VAR(density);
    auto timer = Timer();
    const size_t num_iter = dimensions[0] * dimensions[1];
    for (auto i = 0; i < num_iter; ++i){
      Math::RNG rng;
      typename std::default_random_engine::result_type seed = rng.get_seed();
      auto random_engine = std::default_random_engine{static_cast<std::default_random_engine::result_type>(seed)};
      auto idx = std::vector<size_t>(dimensions[2]);
      std::iota (std::begin(idx), std::end(idx), 0);
      std::shuffle (std::begin(idx), std::end(idx), random_engine);
    }

    CONSOLE(str(timer.elapsed()));
  } else if (type == 4) {
    // class MyThread {
    //   public:
    //     void execute() {
    //       std::cerr << "thread" << std::endl;
    //     }
    // };

    // MyThread thread;
    // Thread::multi thread_array (thread);
    // Thread::Exec exec (thread_array, "my threads");
    // Loop(input, i)
    // thread:
    // 
  //   auto loop1 = Random_loop<ImageType,decltype(engine)>(input, engine, 0, std::ceil((float) input.size(0) * 1.0));
  //     for j in Random_loop(input, 1)
  //       if cnt == total_cnt / nthreads / dim(2)
  //         break
  //       for k in Loop(im,2)
  // }
    // size_t seed = std::chrono::system_clock::now().time_since_epoch().count();
    // auto engine = std::default_random_engine{static_cast<std::default_random_engine::result_type>(seed)};
    

  // for (auto i = Loop (0) (input); i; ++i) {
  //   run_thread
  // }

  // nst std::string& progress_message,
  // 447         const HeaderType& source,
  // 448         size_t from_axis = 0,
  // 449         size_t to_axis = std::numeric_limits<size_t>::max(), 
  // 450         size_t num_inner_axes = 1)
    auto timer = Timer();
    auto loop = ThreadedLoop ("...", input, 0, 3, 2);
    Math::RNG rng;
    size_t cnt;
    ThreadFunctor<decltype(input)> functor (input, loop.inner_axes, density, cnt, rng);
    loop.run_outer (functor);
    CONSOLE(str(timer.elapsed()));
    VAR(cnt);


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
}
