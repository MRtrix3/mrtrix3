#include "dwi/tractography/connectomics/tckmesh_mapper.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace Connectomics
{


TckMeshMapper::TckMeshMapper( Mesh::SceneModeller* sceneModeller,
                              float distanceLimit )
              : _sceneModeller( sceneModeller ),
                _distanceLimit( distanceLimit )
{

  // preparing a polygon lut
  int32_t polygonIndex = 0;
  int32_t polygonCount = 0;
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

  }

}


TckMeshMapper::~TckMeshMapper()
{
}


void TckMeshMapper::findNodePair( const Streamline< float >& tck,
                                  NodePair& nodePair )
{

  int32_t node1 = getNodeIndex( tck.front() );
  if ( node1 >= 0 )
  {

    int32_t node2 = getNodeIndex( tck.back() );
    if ( node2 >= 0 )
    {

      if ( node1 <= node2 )
      {

        nodePair.setNodePair( node1, node2 );

      }
      else
      {

        nodePair.setNodePair( node2, node1 );

      }

    }
    else
    {

      // node not found
      nodePair.setNodePair( -1, -1 );

    }

  }
  else
  {

    // node not found
    nodePair.setNodePair( -1, -1 );

  }

}


// functor for multithreading application
bool TckMeshMapper::operator() ( const Streamline< float >& tck,
                                 NodePair& nodePair )
{

  int32_t node1 = getNodeIndex( tck.front() );
  if ( node1 >= 0 )
  {

    int32_t node2 = getNodeIndex( tck.back() );
    if ( node2 >= 0 )
    {

      if ( node1 <= node2 )
      {

        nodePair.setNodePair( node1, node2 );

      }
      else
      {

        nodePair.setNodePair( node2, node1 );

      }

    }
    else
    {

      // node not found
      nodePair.setNodePair( -1, -1 );

    }

  }
  else
  {

    // node not found
    nodePair.setNodePair( -1, -1 );

  }
  return true;

}


int32_t TckMeshMapper::getNodeCount() const
{

  return _polygonLut.size();

}


Point< int32_t > TckMeshMapper::getPolygonIndices(
                                       const Mesh::Polygon< 3 >& polygon ) const
{

  return Point< int32_t >( polygon.indices[ 0 ],
                           polygon.indices[ 1 ],
                           polygon.indices[ 2 ] );

}


int32_t TckMeshMapper::getNodeIndex( const Point< float >& point ) const
{

  float distance = std::numeric_limits< float >::infinity();
  Mesh::SceneMesh* sceneMesh;
  Mesh::Polygon< 3 > polygon;
  if ( findNode( point, distance, sceneMesh, polygon ) )
  {

    return _polygonLut.find( getPolygonIndices( polygon ) )->second;

  }
  else
  {

    return -1;

  }

}


bool TckMeshMapper::findNode( const Point< float >& point,
                              float& distance,
                              Mesh::SceneMesh*& sceneMesh,
                              Mesh::Polygon< 3 >& polygon ) const
{

  // initialising empty scene mesh
  Mesh::SceneMesh* closestMesh = NULL;

  if ( !_sceneModeller->getSceneMeshes().empty() )
  {

    // getting the local voxel from the given point
    Point< int32_t > voxel;
    _sceneModeller->getBresenhamLineAlgorithm().getVoxelFromPoint( point,
                                                                   voxel );

    // getting the minimum resolution of the cache voxel
    Point< float > resolution = _sceneModeller->getResolution();
    float minimumResolution = std::min( std::min( resolution[ 0 ],
                                                  resolution[ 1 ] ),
                                                  resolution[ 2 ] );

    // increasing searching distance until reaching the limit (threshold)
    Point< int32_t > v;
    float newDistance;
    Mesh::Polygon< 3 > newPolygon;
    Point< float > newPoint;

    float searchingDistance = minimumResolution;
    int32_t stride = 1;
    do
    {

      for ( int32_t x = -stride; x <= stride; x++ )
      {

        v[ 0 ] = voxel[ 0 ] + x;
        for ( int32_t y = -stride; y <= stride; y++ )
        {

          v[ 1 ] = voxel[ 1 ] + y;
          for ( int32_t z = -stride; z <= stride; z++ )
          {

            v[ 2 ] = voxel[ 2 ] + z;
            if ( _sceneModeller->getIntegerBoundingBox().contains( v ) )
            {

              // collecting meshes crossing this voxel
              std::vector< Mesh::SceneMesh* > meshes =
                _sceneModeller->getSceneMeshCache().getSceneMeshes( v );

              // if some meshes are found in the current voxel
              if ( !meshes.empty() )
              {

                // looping over the meshes
                std::vector< Mesh::SceneMesh* >::const_iterator
                  m = meshes.begin(),
                  me = meshes.end();
                while ( m != me )
                {

                  // getting the distance to the closest polygon
                  ( *m )->getClosestPolygonAtVoxel( point,
                                                    v,
                                                    newDistance,
                                                    newPolygon,
                                                    newPoint );

                  if ( newDistance < distance && newDistance <= _distanceLimit )
                  {

                    // updating distance / mesh / polygon
                    distance = newDistance;
                    closestMesh = *m;
                    polygon = newPolygon;

                  }
                  ++ m;

                }

              }

            }

          }

        }

      }
      searchingDistance += minimumResolution;
      ++ stride;

    }
    while ( closestMesh == 0 && searchingDistance <= _distanceLimit );

  }

  // checking whether if the closest mesh/polygon exists
  if ( closestMesh != 0 )
  {

    return true;

  }
  else
  {

    distance = std::numeric_limits< float >::infinity(); 
    sceneMesh = closestMesh;
    polygon[ 0 ] = polygon[ 1 ] = polygon[ 2 ] =
                                     std::numeric_limits< int32_t >::infinity();
    return false;

  }

}


}

}

}

}

