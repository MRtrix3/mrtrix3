#include "command.h"
#include "debug.h"
#include "math/rng.h"
#include "image.h"
#include "algo/neighbourhooditerator.h"
#include "algo/threaded_copy.h"

namespace MR {
}


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "Joe Bloggs (joe.bloggs@acme.org)";

  DESCRIPTION
  + "command to create image artifacts";

  ARGUMENTS
  + Argument ("in", "the input image.").type_image_in ()
  + Argument ("out", "the output image.").type_image_out ();

  OPTIONS
  + Option ("displaced", " TODO " "...")
    + Argument ("size").type_integer ()
    + Argument ("number").type_integer ()
    + Argument ("mode").type_integer ();
}

ssize_t get_central_index(Math::RNG::Normal<float>& rndn, Image<float> image, size_t j){
  ssize_t idx = -1;
  while (idx < 0 or idx > image.size(j)){
    idx = image.size(j) / 2 + std::round(float(image.size(j)) * rndn() / 4);
  } 
  return idx;
}

void run ()
{
  auto in = Image<float>::open (argument[0]);
  auto out = Image<float>::create (argument[1], in.original_header());
  threaded_copy (in, out);
  auto opt = get_options ("displaced");
  if (opt.size()){
    assert(in.ndim() == 3);
    const size_t ext = opt[0][0];
    const size_t artifacts = opt[0][1];
    size_t mode = opt[0][2];
    if (mode%2 == 0) INFO("uniform");
    if (mode%2 != 0) INFO("centered");
    std::vector<size_t> extent (3, ext);
    VAR(extent);
    Math::RNG::Uniform<float> rnd;
    Math::RNG::Normal<float> rndn;
    
    for (auto i = 0; i < artifacts; ++i){
      for (auto j = 0; j < 3; ++j){
        if (mode%2 == 0){
          in.index(j) =  std::round(float((in.size(j) - 2 * ext))  * rnd() + ext);
          out.index(j) = size_t (std::round((in.size(j) - 2 * ext)  * rnd() + ext));
        } else {
          in.index(j) =  get_central_index(rndn, in, j);
          out.index(j) = get_central_index(rndn, out, j);
        }
      }
      auto niter_in = NeighbourhoodIterator (in, extent);
      auto niter_out = NeighbourhoodIterator (out, extent);
      while(niter_in.loop() and niter_out.loop()){
        for (auto j = 0; j < 3; ++j){
          in.index(j) = niter_in.index(j);
          out.index(j) = niter_out.index(j);
        }
        out.value() = in.value();
      }
    }
  }
}
