#include "mesh/scene_modeller.h"
#include "mesh/intersection_set.h"


namespace MR
{


namespace Mesh
{


SceneModeller::SceneModeller( const BoundingBox< float >& boundingBox,
                              const Point< int32_t >& cacheSize )
              : _boundingBox( boundingBox ),
                _integerBoundingBox( 0, cacheSize[ 0 ] - 1,
                                     0, cacheSize[ 1 ] - 1,
                                     0, cacheSize[ 2 ] - 1 ),
                _cacheSize( cacheSize ),
                _bresenhamLineAlgorithm( boundingBox, cacheSize ),
                _sceneMeshCache( this )
{
}


SceneModeller::~SceneModeller()
{
}


const BoundingBox< float >& SceneModeller::getBoundingBox() const
{

  return _boundingBox;

}


const Point< int32_t >& SceneModeller::getCacheSize() const
{

  return _cacheSize;

}


const BresenhamLineAlgorithm& SceneModeller::getBresenhamLineAlgorithm() const
{

  return _bresenhamLineAlgorithm;

}


void SceneModeller::getCacheVoxel( const Point< float >& point,
                                   Point< int32_t >& voxel ) const
{

  _bresenhamLineAlgorithm.getVoxelFromPoint( point, voxel );

}


void SceneModeller::addSceneMesh( SceneMesh* sceneMesh )
{

  _meshes.push_back( sceneMesh );
  _sceneMeshCache.update( sceneMesh );

}


int32_t SceneModeller::getSceneMeshCount() const
{

  return ( int32_t )_meshes.size();

}


SceneMesh* SceneModeller::getSceneMesh( int32_t index )
{

  return _meshes[ index ];

}


const SceneMeshCache& SceneModeller::getSceneMeshCache() const
{

  return _sceneMeshCache;

}


void SceneModeller::getClosestMeshPolygon(
                                         const Point< float >& point,
                                         float& distance,
                                         SceneMesh*& mesh,
                                         Polygon< 3 >& polygon,
                                         Point< float >& projectionPoint ) const
{

  // initialising empty scene mesh
  SceneMesh* closestMesh = NULL;

  if ( !_meshes.empty() )
  {

    // getting the local voxel from the given point
    Point< int32_t > voxel;
    _bresenhamLineAlgorithm.getVoxelFromPoint( point, voxel );

    // increasing the stride until the closest mesh is found
    Point< int32_t > currentVoxel;
    float newDistance;
    Polygon< 3 > newPolygon;
    Point< float > newPoint;

    int32_t stride = 1;
    while ( closestMesh == 0 )
    {

      for ( int32_t x = -stride; x <= stride; x++ )
      {

        currentVoxel[ 0 ] = voxel[ 0 ] + x;
        for ( int32_t y = -stride; y <= stride; y++ )
        {

          currentVoxel[ 1 ] = voxel[ 1 ] + y;
          for ( int32_t z = -stride; z <= stride; z++ )
          {

            currentVoxel[ 2 ] = voxel[ 2 ] + z;
            if ( _integerBoundingBox.contains( currentVoxel ) )
            {

              // collecting meshes crossing this voxel
              std::vector< SceneMesh* >
                meshes = _sceneMeshCache.getSceneMeshes( currentVoxel );

              // if some meshes are found
              if ( !meshes.empty() )
              {

                // looping over the meshes
                std::vector< SceneMesh* >::const_iterator
                  m = meshes.begin(),
                  me = meshes.end();
                while ( m != me )
                {

                  // getting potential intersections
                  ( *m )->getClosestPolygonAtVoxel( point,
                                                    currentVoxel,
                                                    newDistance,
                                                    newPolygon,
                                                    newPoint );

                  if ( newDistance < distance )
                  {

                    // updating distance / mesh / polygon / projection point
                    distance = newDistance;

                    closestMesh = *m;
                    polygon = newPolygon;
                    projectionPoint = newPoint;

                  }
                  ++ m;

                }

              }

            }

          }

        }

      }
      ++ stride;

    }

  }

}


bool SceneModeller::isInsideSceneMesh( const MR::Point< float >& point ) const
{

  // **** Note : the method only functions for a closed mesh ****

  // initialising a projection point
  Point< float > projectionPoint( point );

  ////// casting a ray in +x or -x direction

  // getting the projection point on the bounding box
  float upperX = _boundingBox.getUpperX();
  float lowerX = _boundingBox.getLowerX();
  if ( ( upperX - point[ 0 ] ) < ( point[ 0 ] - lowerX ) )
  {

    projectionPoint[ 0 ] = upperX;

  }
  else
  {

    projectionPoint[ 0 ] = lowerX;

  }

  // collecting intersections
  IntersectionSet intersectionSetAlongX( *this, point, projectionPoint );
  int32_t intersectionCountAlongX = intersectionSetAlongX.getCount();

  ////// casting a ray in +y or -y direction

  // getting the projection point on the bounding box
  projectionPoint[ 0 ] = point[ 0 ];
  float upperY = _boundingBox.getUpperY();
  float lowerY = _boundingBox.getLowerY();
  if ( ( upperY - point[ 1 ] ) < ( point[ 1 ] - lowerY ) )
  {

    projectionPoint[ 1 ] = upperY;

  }
  else
  {

    projectionPoint[ 1 ] = lowerY;

  }
  IntersectionSet intersectionSetAlongY( *this, point, projectionPoint );
  int32_t intersectionCountAlongY = intersectionSetAlongY.getCount();

  // checking the intersection count
  if ( ( intersectionCountAlongX % 2 ) &&
       ( intersectionCountAlongY % 2 ) )
  {

    return true;

  }
  else
  {

    return false;

  }

}


}

}

