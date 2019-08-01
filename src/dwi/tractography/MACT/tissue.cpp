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


#include "dwi/tractography/MACT/tissue.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace MACT
{


Tissue::Tissue( const TissueType& type,
                const std::string& name,
                const Surface::Mesh& mesh,
                const std::shared_ptr< SceneModeller >& sceneModeller,
                double radiusOfInfluence )
       : _type( type ),
         _name( name ),
         _mesh( mesh ),
         _sceneModeller( sceneModeller ),
         _radiusOfInfluence( radiusOfInfluence ),
         _polygonLut( Tissue_ptr( this ) )
{
  // compute polygon normals (can move to Mesh Class)
  auto triangles = mesh.get_triangles();
  _normals.resize( triangles.size() );
  for ( size_t t = 0; t < triangles.size(); t++ )
  {
    auto v0 = mesh.vert( triangles[ t ][ 0 ] );
    auto v1 = mesh.vert( triangles[ t ][ 1 ] );
    auto v2 = mesh.vert( triangles[ t ][ 2 ] );
    _normals[ t ] = ( v1 - v0 ).cross( v2 - v0 ).normalized();
  }
}


Tissue::~Tissue()
{
}


TissueType Tissue::type() const
{
  return _type;
}


std::string Tissue::name() const
{
  return _name;
}


Surface::Mesh Tissue::mesh() const
{
  return _mesh;
}


std::shared_ptr< SceneModeller > Tissue::sceneModeller() const
{
  return _sceneModeller;
}


double Tissue::radiusOfInfluence() const
{
  return _radiusOfInfluence;
}


size_t Tissue::polygonCount() const
{
  return ( size_t )_mesh.num_triangles();
}


const PolygonLut& Tissue::polygonLut() const
{
  return _polygonLut;
}


const Eigen::Vector3d& Tissue::normal( size_t triangleId ) const
{
  return _normals[ triangleId ];
}


}

}

}

}

