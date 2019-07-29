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
#include "connectome/lut.h"
#include "surface/freesurfer.h"
#include "surface/mesh.h"
#include "surface/meshfactory.h"
#include <set>


using namespace MR;
using namespace App;

using namespace MR::Surface;


void usage ()
{

  AUTHOR = "Chun-Hung Yeh (chun-hung.yeh@florey.edu.au)";

  SYNOPSIS = "Crop a mesh";

  ARGUMENTS
  + Argument( "mesh_i", "the surface mesh to be cropped." ).type_file_in()

  + Argument( "mesh_o", "the output cropped mesh file." ).type_file_out ();

  OPTIONS
  + Option( "annot", "crop FreeSurfer's pial or white surface; "
                     "remove triangles according to the label/structure index in the annotation file" )
    + Argument( "annot", "FreeSurfer's annotation file" ).type_file_in()
    + Argument( "label", "structure to be removed" ).type_integer( 0 )

  + Option( "setdiff", "crop the intersection with a mesh; "
                       "intersection defined as the distance (mm) between vertices" )
    + Argument( "mesh" ).type_file_in()
    + Argument( "radius", "radius of influence" ).type_float( 0.0 );

};


void run ()
{

  Mesh mesh{ argument[ 0 ] };
  auto vertices = mesh.get_vertices();
  auto triangles = mesh.get_triangles();

  auto opt = get_options( "annot" );
  if ( opt.size() )
  {
    // read freesurfer's annotation file
    MR::Connectome::LUT ctable;
    label_vector_type labels;
    FreeSurfer::read_annot( opt[ 0 ][ 0 ], labels, ctable );

    if ( vertices.size() != labels.size() )
    {
      throw Exception( "Incompatible between surface mesh and annotation file " 
                       "(vertex count and label count are not equal)" );
    }

    // collect indices of vertices to be removed based on label value
    const size_t label = opt[ 0 ][ 1 ];
    std::vector< uint32_t > crop_v;
    for ( size_t index = 0; index < labels.size(); index++ )
    {
      if ( labels[ index ] == label )
      {
        crop_v.push_back( index );
      }
    }

    // crop the mesh if required
    if ( crop_v.size() )
    {
      MeshFactory::getInstance().crop( mesh, crop_v );
    }
    else
    {
      WARN( "Label value not found; "
            "the output surface mesh will be the same as the input" );
    }
  }

  opt = get_options( "setdiff" );
  if ( opt.size() )
  {
    Mesh m{ opt[ 0 ][ 0 ] };
    auto mv = m.get_vertices();
    auto mt = m.get_triangles();

    const float r = opt[ 0 ][ 1 ];

    // collect intersection for the given radius of influence
    std::set< uint32_t > intersect;
    for ( size_t u = 0; u < vertices.size(); u++ )
    {
      for ( size_t v = 0; v < mv.size(); v++ )
      {
        if ( ( vertices[ u ] - mv[ v ] ).norm() < r )
        {
          intersect.insert( u );
        }
      }
    }

    // collect indices of vertices to be removed based on intersection
    std::vector< uint32_t > crop_v;
    for ( size_t u = 0; u < vertices.size(); u++ )
    {
      if ( intersect.find( u ) == intersect.end() )
      {
        crop_v.push_back( u );
      }
    }

    // crop the mesh if required
    if ( crop_v.size() )
    {
      MeshFactory::getInstance().crop( mesh, crop_v );
    }
    else
    {
      WARN( "Intersection not found; "
            "the output surface mesh will be the same as the input" );
    }
  }

  mesh.save( argument[ 1 ] );

}

