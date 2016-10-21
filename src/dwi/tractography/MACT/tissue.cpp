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


}

}

}

}

