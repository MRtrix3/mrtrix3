#include "command.h"
#include "debug.h"
#include "image.h"
#include "algo/loop.h"
#include "algo/iterator.h"
#include "algo/neighbourhooditerator.h"

namespace MR {
}


using namespace MR;
using namespace App;


void usage ()
{
  AUTHOR = "Joe Bloggs (joe.bloggs@acme.org)";

  DESCRIPTION
  + "test iterator";

  ARGUMENTS
  + Argument ("in", "the input image.").type_image_in ();
}


void run ()
{
  auto input = Image<float>::open (argument[0]);
  auto iter = Iterator(input);
  Eigen::Vector3 voxel(46,41,29);

  iter.index(0) = 0;
  iter.index(1) = 50;
  iter.index(2) = 59;

  std::cerr << iter.index(0) << " " << iter.index(1) << " " <<iter.index(2) << std::endl;
  std::vector<size_t> extent(3,5);
  VAR(extent);

  auto niter = NeighbourhoodIterator(iter, extent);
  std::cerr << "niter: " <<  niter.index(0) << " " << niter.index(1) << " " << niter.index(2) << std::endl;
  for (auto i = Loop ()(niter); i; ++i){
      std::cerr << niter.index(0) << " " << niter.index(1) << " " << niter.index(2) << std::endl;
  }
  std::cerr << iter.index(0) << " " << iter.index(1) << " " <<iter.index(2) << std::endl;
}
