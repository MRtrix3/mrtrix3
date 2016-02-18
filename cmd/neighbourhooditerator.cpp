/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see www.mrtrix.org
 *
 */

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
  std::cerr << input.original_header() << std::endl;

  auto iter = Iterator(input);
  Eigen::Vector3 voxel(46,41,29);

  iter.index(0) = 0;
  iter.index(1) = 50;
  iter.index(2) = 59;
  
  std::cerr << iter.index(0) << " " << iter.index(1) << " " <<iter.index(2) << std::endl;
  std::vector<size_t> extent(3,5);
  VAR(extent);

  auto niter = NeighbourhoodIterator(iter, extent);
  // std::cerr << "niter: " <<  niter.index(0) << " " << niter.index(1) << " " << niter.index(2) << std::endl;

  // for (auto j = 0; j < niter.ndim(); ++j){ 
  //   for (auto i = 0; i<niter.extent(j); ++niter.index(j), ++i)
  //     std::cerr << niter << std::endl;
  //   niter.reset(j);
  // }

  while(niter.loop()){
      std::cerr << niter << std::endl;
  }

  // for (auto i = Loop ()(niter); i; ++i)
      // std::cerr << niter.index(0) << " " <<  niter.index(1) << " " << niter.index(2) << std::endl;
  std::cerr << iter << std::endl;
  std::cerr << niter << std::endl;
}
