/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "command.h"
#include "surface/meshfactory.h"


using namespace MR;
using namespace App;

using namespace MR::Surface;


void usage()
{

  AUTHOR = "Chun-Hung Yeh (chun-hung.yeh@florey.edu.au)";

  SYNOPSIS = "concatenate several meshes into one.";

  ARGUMENTS
  + Argument( "mesh1", "the first input mesh." ).type_file_in()

  + Argument( "mesh2", "additional input mesh(es)." ).type_file_in().allow_multiple()

  + Argument( "output", "the output mesh." ).type_file_out ();

};


void run()
{

  size_t meshCount = argument.size() - 1;
  std::vector< Mesh > meshes( meshCount );
  for ( size_t m = 0; m < meshCount; m++ )
  {
    meshes[ m ] = Mesh{ argument[ m ] };
  }
  Mesh out = MeshFactory::getInstance().concatenate( meshes );
  out.save( argument[ meshCount ] );

}

