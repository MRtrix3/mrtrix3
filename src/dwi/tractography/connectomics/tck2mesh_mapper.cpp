#include "dwi/tractography/connectomics/tck2mesh_mapper.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace Connectomics
{


Tck2MeshMapper::Tck2MeshMapper( Mesh::SceneModeller* sceneModeller,
                                float distanceLimit )
               : _sceneModeller( sceneModeller ),
                 _distanceLimit( distanceLimit )
{

  // preparing a polygon lut & allocating the output connectivity matrix
  int32_t polygonIndex = 0;
  int32_t polygonCount = 0;
  int32_t globalPolygonCount = 0;
  int32_t sceneMeshCount = _sceneModeller->getSceneMeshCount();
  for ( int32_t m = 0; m < sceneMeshCount; m++ )
  {

    Mesh::SceneMesh* currentSceneMesh = _sceneModeller->getSceneMesh( m );
    Mesh::PolygonList polygons = currentSceneMesh->getMesh()->polygons;
    polygonCount = currentSceneMesh->getPolygonCount();
    for ( int32_t p = 0; p < polygonCount; p++ )
    {

      _polygonLut[ Point< int32_t >( polygons[ p ].indices[ 0 ],
                                     polygons[ p ].indices[ 1 ],
                                     polygons[ p ].indices[ 2 ] )
                  ] = polygonIndex;
      ++ polygonIndex;

    }
    globalPolygonCount += polygonCount;

  }
  _matrix.allocate( globalPolygonCount, globalPolygonCount );

}


Tck2MeshMapper::~Tck2MeshMapper()
{
}


void Tck2MeshMapper::update( const Streamline< float >& tck )
{

  int32_t node_1 = getNodeIndex( tck.front() );
  if ( node_1 >= 0 )
  {

    int32_t node_2 = getNodeIndex( tck.back() );
    if ( node_2 >= 0 )
    {

      if ( node_1 < node_2 )
      {

        ++ _matrix( node_1, node_2 );

      }
      else
      {

        ++ _matrix( node_2, node_1 );

      }

    }

  }

}


void Tck2MeshMapper::write( const std::string& path )
{

  _matrix.save( path );

}


Point< int32_t > Tck2MeshMapper::getPolygonIndices(
                                       const Mesh::Polygon< 3 >& polygon ) const
{

  return Point< int32_t >( polygon.indices[ 0 ],
                           polygon.indices[ 1 ],
                           polygon.indices[ 2 ] );

}


int32_t Tck2MeshMapper::getNodeIndex( const Point< float >& point ) const
{

  float distance = std::numeric_limits< float >::infinity();
  Mesh::SceneMesh* sceneMesh;
  Mesh::Polygon< 3 > polygon;
  Point< float > projectionPoint;
  _sceneModeller->getClosestMeshPolygon( point,
                                         distance,
                                         sceneMesh,
                                         polygon,
                                         projectionPoint );

  if ( distance > _distanceLimit )
  {

    return -1;

  }
  else
  {

    return _polygonLut.find( getPolygonIndices( polygon ) )->second;

  }

}


}

}

}

}

