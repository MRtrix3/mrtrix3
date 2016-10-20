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

#ifndef __surface_algo_mesh2image_h__
#define __surface_algo_mesh2image_h__


#include "image.h"
#include "types.h"
#include "surface/mesh.h"



namespace MR
{
  namespace Surface
  {
    namespace Algo
    {


      void mesh2image (const Mesh&, Image<float>&);


    }
  }
}

#endif

