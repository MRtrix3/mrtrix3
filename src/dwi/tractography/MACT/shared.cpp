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


#include "dwi/tractography/MACT/shared.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace MACT
{


MACT_Shared_additions::MACT_Shared_additions( Properties& property_set )
                      : _backtrack( false ),
                        _crop_at_gmwmi( false )
{
  /* need a progess bar */

  // import all meshes and determine the bounding box based on meshes
  std::map< std::string, Surface::Mesh > meshes;
  meshes[ "octx" ] = Surface::Mesh{ property_set[ "mact_ctx_outer" ] };
  meshes[ "ictx" ] = Surface::Mesh{ property_set[ "mact_ctx_inner" ] };
  meshes[ "sgm"  ] = Surface::Mesh{ property_set[ "mact_sgm"       ] };
  meshes[ "csf"  ] = Surface::Mesh{ property_set[ "mact_csf"       ] };

  Eigen::Vector3d lower_p = meshes.begin()->second.vert( 0 );
  Eigen::Vector3d upper_p = meshes.begin()->second.vert( 0 );
  for ( auto m = meshes.begin(); m != meshes.end(); ++ m )
  {
    for ( size_t v = 0; v < m->second.num_vertices(); v++ )
    {
      Surface::Vertex thisVertex = m->second.vert( v );
      for ( size_t axis = 0; axis < 3; axis++ )
      {
        lower_p[ axis ] = std::min( lower_p[ axis ], thisVertex[ axis ] );
        upper_p[ axis ] = std::max( upper_p[ axis ], thisVertex[ axis ] );
      }
    }
  }

  // build bounding box and scene modeller
  BoundingBox< double > boundingBox( lower_p[ 0 ], upper_p[ 0 ],
                                     lower_p[ 1 ], upper_p[ 1 ],
                                     lower_p[ 2 ], upper_p[ 2 ] );
  double edge_length = to< double >( property_set[ "mact_lut"  ] );
  Eigen::Vector3i lutSize( ( upper_p[ 0 ] - lower_p[ 0 ] ) / edge_length,
                           ( upper_p[ 1 ] - lower_p[ 1 ] ) / edge_length,
                           ( upper_p[ 2 ] - lower_p[ 2 ] ) / edge_length );
  _sceneModeller.reset( new SceneModeller( boundingBox, lutSize ) );

  // build tissues in scene modeller
  std::set< Tissue_ptr > tissues;
  Tissue_ptr tissue;
  tissue.reset( new Tissue( CGM, "octx", meshes[ "octx" ], _sceneModeller ) );
  tissues.insert( tissue );
  tissue.reset( new Tissue( WM , "ictx", meshes[ "ictx" ], _sceneModeller ) );
  tissues.insert( tissue );
  tissue.reset( new Tissue( SGM, "sgm" , meshes[ "sgm"  ], _sceneModeller ) );
  tissues.insert( tissue );
  tissue.reset( new Tissue( CSF, "csf" , meshes[ "csf"  ], _sceneModeller ) );
  tissues.insert( tissue );
  _sceneModeller->addTissues( tissues );

  if ( property_set.find( "backtrack" ) != property_set.end() )
  {
    property_set.set( _backtrack, "backtrack" );
  }
  if ( property_set.find( "crop_at_gmwmi" ) != property_set.end() )
  {
    property_set.set( _crop_at_gmwmi, "crop_at_gmwmi" );
  }
}


bool MACT_Shared_additions::backtrack() const
{
  return _backtrack;
}

/*
bool MACT_Shared_additions::crop_at_gmwmi() const
{
  return _crop_at_gmwmi;
}


void MACT_Shared_additions::crop_at_gmwmi( std::vector< Eigen::Vector3f >& tck ) const
{
  Eigen::Vector3d last = ( *tck.rbegin() ).cast< double >();
  Eigen::Vector3d second_to_last = ( *( ++tck.rbegin() ) ).cast< double >();
  IntersectionSet intersections( *_sceneModeller, last, second_to_last );
  if ( intersections.count() )
  {
    tck.back() = ( intersections.intersection( 0 )._point ).cast< float >();
  }
  else
  {
    throw Exception( "crop at gmwmi: missing intersection point" );
  }
}
*/

}

}

}

}

