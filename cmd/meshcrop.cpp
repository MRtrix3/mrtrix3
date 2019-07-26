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
  + Option( "annot", "crop FreeSurfer's pial or white surface; remove triangles with vertex label = 0" )
    + Argument( "file", "FreeSurfer's annotation file" ).type_file_in();

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
                       "(vertex count and label count are not equal)." );
    }

    // collect indices of vertices with label value = 0
    std::vector< uint32_t > crop_v;
    for ( size_t index = 0; index < labels.size(); index++ )
    {
      if ( !labels[ index ] )
      {
        crop_v.push_back( index );
      }
    }

    // collect indices of triangles having a vertex with label value = 0
    std::vector< uint32_t > crop_t;
    for ( size_t t = 0; t < triangles.size(); t++ )
    {
      for ( size_t v = 0; v < 3; v++ )
      {
        if ( find( crop_v.begin(), crop_v.end(), triangles[ t ][ v ] ) !=
                   crop_v.end() )
        {
          crop_t.push_back( t );
          break;
        }
      }
    }

    // crop vertices
    size_t crop_v_count = crop_v.size();
    for ( size_t c = 0; c < crop_v_count; c++ )
    {
      size_t i = crop_v[ crop_v_count - c - 1 ];
      vertices.erase( vertices.begin() + i );
    }

    // crop triangles and update vertex index for the remaining triangles
    size_t crop_t_count = crop_t.size();
    for ( size_t c = 0; c < crop_t_count; c++ )
    {
      size_t i = crop_t[ crop_t_count - c - 1 ];
      triangles.erase( triangles.begin() + i );
    }
    for ( size_t c = 0; c < crop_v_count; ++c )
    {
      for ( size_t f = 0; f < triangles.size(); ++f )
      {
        for ( size_t v = 0; v < 3; ++v )
        {
          if ( triangles[ f ][ v ] > crop_v[ crop_v_count - c - 1 ] )
          {
            triangles[ f ][ v ] -= 1;
          }
        }
      }
    }
  }

  // export the output mesh
  Mesh out;
  out.load( vertices, triangles );
  out.save( argument[ 1 ] );

}

