#include "mesh/bresenham_line_algorithm.h"


namespace MR
{

namespace Mesh
{


BresenhamLineAlgorithm::BresenhamLineAlgorithm(
                                        const BoundingBox< float >& boundingBox,
                                        const Point< int32_t >& cacheSize )
                       : _cacheSize( cacheSize ),
                         _cacheSizeMinusOne( cacheSize[ 0 ] - 1,
                                             cacheSize[ 1 ] - 1,
                                             cacheSize[ 2 ] - 1 )
{

  _lowerX = boundingBox.getLowerX();
  _upperX = boundingBox.getUpperX();
  _lowerY = boundingBox.getLowerY();
  _upperY = boundingBox.getUpperY();
  _lowerZ = boundingBox.getLowerZ();
  _upperZ = boundingBox.getUpperZ();

  _cacheVoxelFactorX = ( float )_cacheSize[ 0 ] / ( _upperX - _lowerX );
  _cacheVoxelFactorY = ( float )_cacheSize[ 1 ] / ( _upperY - _lowerY );
  _cacheVoxelFactorZ = ( float )_cacheSize[ 2 ] / ( _upperZ - _lowerZ );

  _minimumResolution = std::min( std::min(
                                      ( _upperX - _lowerX ) / _cacheSize[ 0 ],
                                      ( _upperY - _lowerY ) / _cacheSize[ 1 ] ),
                                      ( _upperZ - _lowerZ ) / _cacheSize[ 2 ] );

}


BresenhamLineAlgorithm::~BresenhamLineAlgorithm()
{
}


void BresenhamLineAlgorithm::getVoxelFromPoint( const Point< float >& point,
                                                Point< int32_t >& voxel ) const
{

  // x coordinate
  if ( point[ 0 ] < _lowerX )
  {

    voxel[ 0 ] = 0;

  }
  else if ( point[ 0 ] >= _upperX )
  {

    voxel[ 0 ] = _cacheSizeMinusOne[ 0 ];

  }
  else
  {

    voxel[ 0 ] = ( int32_t )( ( point[ 0 ] - _lowerX ) * _cacheVoxelFactorX );
    if ( voxel[ 0 ] < 0 )
    {

      voxel[ 0 ] = 0;

    }
    else if ( voxel[ 0 ] > _cacheSizeMinusOne[ 0 ] )
    {

      voxel[ 0 ] = _cacheSizeMinusOne[ 0 ];

    }

  }

  // y coordinate
  if ( point[ 1 ] < _lowerY )
  {

    voxel[ 1 ] = 0;

  }
  else if ( point[ 1 ] >= _upperY )
  {

    voxel[ 1 ] = _cacheSizeMinusOne[ 1 ];

  }
  else
  {

    voxel[ 1 ] = ( int32_t )( ( point[ 1 ] - _lowerY ) * _cacheVoxelFactorY );
    if ( voxel[ 1 ] < 0 )
    {

      voxel[ 1 ] = 0;

    }
    else if ( voxel[ 1 ] > _cacheSizeMinusOne[ 1 ] )
    {

      voxel[ 1 ] = _cacheSizeMinusOne[ 1 ];

    }

  }

  // z coordinate
  if ( point[ 2 ] < _lowerZ )
  {

    voxel[ 2 ] = 0;

  }
  else if ( point[ 2 ] >= _upperZ )
  {

    voxel[ 2 ] = _cacheSizeMinusOne[ 2 ];

  }
  else
  {

    voxel[ 2 ] = ( int32_t )( ( point[ 2 ] - _lowerZ ) * _cacheVoxelFactorZ );
    if ( voxel[ 2 ] < 0 )
    {

      voxel[ 2 ] = 0;

    }
    else if ( voxel[ 2 ] > _cacheSizeMinusOne[ 2 ] )
    {

      voxel[ 2 ] = _cacheSizeMinusOne[ 2 ];

    }

  }

}


void BresenhamLineAlgorithm::getNeighbouringVoxels(
                                     const Point< int32_t >& voxel,
                                     Point< int32_t > stride,
                                     std::vector< Point< int32_t > >& neighbors,
                                     std::vector< bool >& neighborMasks ) const
{

  // resizing the neighborhood offsets and masks according to the stride
  Point< int32_t > size( 2 * stride[ 0 ] + 1,
                         2 * stride[ 1 ] + 1,
                         2 * stride[ 2 ] + 1 );
  neighbors.resize( size[ 0 ] * size[ 1 ] * size[ 2 ] );
  neighborMasks.resize( size[ 0 ] * size[ 1 ] * size[ 2 ] );

  // processing the neighborhood offsets and masks
  Point< int32_t > offset;
  int32_t index = 0;
  int32_t x, y, z;
  for ( x = -stride[ 0 ]; x <= stride[ 0 ]; x++ )
  {

    offset[ 0 ] = x;
    for ( y = -stride[ 1 ]; y <= stride[ 1 ]; y++ )
    {

      offset[ 1 ] = y;
      for ( z = -stride[ 2 ]; z <= stride[ 2 ]; z++ )
      {

        offset[ 2 ] = z;

        Point< int32_t >& neighbor = neighbors[ index ];
        neighbor = voxel + offset;

        if ( ( neighbor[ 0 ] >= 0 ) &&
             ( neighbor[ 0 ] <= _cacheSizeMinusOne[ 0 ] ) &&
             ( neighbor[ 1 ] >= 0 ) &&
             ( neighbor[ 1 ] <= _cacheSizeMinusOne[ 1 ] ) &&
             ( neighbor[ 2 ] >= 0 ) &&
             ( neighbor[ 2 ] <= _cacheSizeMinusOne[ 2 ] ) )
        {

          neighborMasks[ index ] = true;

        }
        else
        {

          neighborMasks[ index ] = false;

        }
        ++ index;

      }

    }

  }  

}


void BresenhamLineAlgorithm::getRayVoxels(
                                    const Point< float >& from,
                                    const Point< float >& to,
                                    std::set< Point< int32_t >,
                                              PointCompare< int32_t > >& voxels,
                                    bool clearVoxelsAtBeginning ) const
{

  if ( clearVoxelsAtBeginning )
  {

    voxels.clear();

  }

  // processing initial and last voxel
  MR::Point< int32_t > fromVoxel, toVoxel;
  getVoxelFromPoint( from, fromVoxel );
  getVoxelFromPoint( to, toVoxel );

  voxels.insert( fromVoxel );

  if ( toVoxel != fromVoxel )
  {

    voxels.insert( toVoxel );

    // getting the length between the starting and ending points
    float length = ( to - from ).norm();

    // getting the segment count and the step
    int32_t segmentCount = ( int32_t )( length / _minimumResolution ) + 1;
    Point< float > step = ( to - from ) / ( float )segmentCount;

    Point< int32_t > currentVoxel = fromVoxel;
    Point< float > newPoint = from;
    Point< int32_t > newVoxel;
    Point< int32_t > offset;
    Point< int32_t > neighbor;
    int32_t s;

    for ( s = 1; s <= segmentCount; s++ )
    {

      // getting the current point and voxel
      newPoint += step;
      getVoxelFromPoint( newPoint, newVoxel );
      voxels.insert( newVoxel );

      // processing the offset and collecting the voxels
      offset = newVoxel - currentVoxel;

      if ( ( std::abs( offset[ 0 ] ) +
             std::abs( offset[ 1 ] ) +
             std::abs( offset[ 2 ] ) ) > 1 )
      {

        neighbor[ 0 ] = currentVoxel[ 0 ] + offset[ 0 ];
        neighbor[ 1 ] = currentVoxel[ 1 ];
        neighbor[ 2 ] = currentVoxel[ 2 ];
        voxels.insert( neighbor ); 

        neighbor[ 0 ] = currentVoxel[ 0 ];
        neighbor[ 1 ] = currentVoxel[ 1 ] + offset[ 1 ];
        neighbor[ 2 ] = currentVoxel[ 2 ];
        voxels.insert( neighbor ); 

        neighbor[ 0 ] = currentVoxel[ 0 ];
        neighbor[ 1 ] = currentVoxel[ 1 ];
        neighbor[ 2 ] = currentVoxel[ 2 ] + offset[ 2 ];
        voxels.insert( neighbor ); 

        neighbor[ 0 ] = currentVoxel[ 0 ] + offset[ 0 ];
        neighbor[ 1 ] = currentVoxel[ 1 ] + offset[ 1 ];
        neighbor[ 2 ] = currentVoxel[ 2 ];
        voxels.insert( neighbor ); 

        neighbor[ 0 ] = currentVoxel[ 0 ] + offset[ 0 ];
        neighbor[ 1 ] = currentVoxel[ 1 ];
        neighbor[ 2 ] = currentVoxel[ 2 ] + offset[ 2 ];
        voxels.insert( neighbor ); 

        neighbor[ 0 ] = currentVoxel[ 0 ];
        neighbor[ 1 ] = currentVoxel[ 1 ] + offset[ 1 ];
        neighbor[ 2 ] = currentVoxel[ 2 ] + offset[ 2 ];
        voxels.insert( neighbor ); 

      }
      // updating the current voxel
      currentVoxel = newVoxel;

    }

  }

}


void BresenhamLineAlgorithm::getTriangleVoxels(
                                    const Point< float >& vertex1,
                                    const Point< float >& vertex2,
                                    const Point< float >& vertex3,
                                    std::set< Point< int32_t >,
                                              PointCompare< int32_t > >& voxels,
                                    bool clearVoxelsAtBeginning ) const
{

  if ( clearVoxelsAtBeginning )
  {

    voxels.clear();

  }

  //////////////////////////////////////////////////////////////////////////////
  // I: collecting the voxels along the rays defined by the three vertices
  //////////////////////////////////////////////////////////////////////////////

  getRayVoxels( vertex1, vertex2, voxels, false );
  getRayVoxels( vertex2, vertex3, voxels, false );
  getRayVoxels( vertex3, vertex1, voxels, false );

  //////////////////////////////////////////////////////////////////////////////
  // II: collecting the voxels along the rays defined by sub-triangles
  //////////////////////////////////////////////////////////////////////////////

  // 1) getting the length of the three line segments defined by the triangle
  float segment12 = ( vertex1 - vertex2 ).norm();
  float segment23 = ( vertex2 - vertex3 ).norm();
  float segment31 = ( vertex3 - vertex1 ).norm();

  Point< float > point;
  Point< float > step;
  int32_t segmentCount = 0;
  int32_t s = 0;

  // 2) getting the voxels between vertex3 and the points located on the segment
  //    defined by vertex1 and vertex2
  point = vertex1;
  segmentCount = ( int32_t )( segment12 / _minimumResolution ) + 1;
  step = ( vertex2 - vertex1 ) / ( float )segmentCount;
  for ( s = 1; s < segmentCount; s++ )
  {

    point += step;
    getRayVoxels( vertex3, point, voxels, false );

  }

  // 3) getting the voxels between vertex1 and the points located on the segment
  //    defined by vertex2 and vertex3
  point = vertex2;
  segmentCount = ( int32_t )( segment23 / _minimumResolution ) + 1;
  step = ( vertex3 - vertex2 ) / ( float )segmentCount;

  for ( s = 1; s < segmentCount; s++ )
  {

    point += step;
    getRayVoxels( vertex1, point, voxels, false );

  }

  // 4) getting the voxels between vertex2 and the points located on the segment
  //    defined by vertex3 and vertex1
  point = vertex3;
  segmentCount = ( int32_t )( segment31 / _minimumResolution ) + 1;
  step = ( vertex1 - vertex3 ) / ( float )segmentCount;

  for ( s = 1; s < segmentCount; s++ )
  {

    point += step;
    getRayVoxels( vertex2, point, voxels, false );

  }

}


void BresenhamLineAlgorithm::getThickTriangleVoxels(
                                    const Point< float >& vertex1,
                                    const Point< float >& vertex2,
                                    const Point< float >& vertex3,
                                    float radiusOfInfluence,
                                    std::set< Point< int32_t >,
                                              PointCompare< int32_t > >& voxels,
                                    bool clearVoxelsAtBeginning ) const

{

  if ( clearVoxelsAtBeginning )
  {

    voxels.clear();

  }

  if ( radiusOfInfluence == 0 )
  {

    getTriangleVoxels( vertex1, vertex2, vertex3,
                       voxels, false );

  }
  else
  {

    // getting the midpoint on each segment of the triangle
    Point< float > midPoint1 = ( vertex2 + vertex3 ) / 2.0f;
    Point< float > midPoint2 = ( vertex3 + vertex1 ) / 2.0f;
    Point< float > midPoint3 = ( vertex1 + vertex2 ) / 2.0f;

    Point< float >
      shift1 = ( ( vertex1 - midPoint1 ).normalise() ) * radiusOfInfluence;
    Point< float >
      shift2 = ( ( vertex2 - midPoint2 ).normalise() ) * radiusOfInfluence;
    Point< float >
      shift3 = ( ( vertex3 - midPoint3 ).normalise() ) * radiusOfInfluence;

    // getting the normal vector of the plane defined by the vertices
    Point< float >
      normal = ( ( vertex2 - vertex1 ).cross( vertex3 - vertex1 ) ).normalise();

    // 1) getting new vertices;
    // 2) moving the new triangle along its normal and collecting the voxels
    //    contained in the triangle for each movement;
    // 3) adding the voxels into a set to keep the voxel unique

    if ( ( 2 * radiusOfInfluence ) < _minimumResolution )
    {

      Point< float > newVertex1, newVertex2, newVertex3;
      for ( int32_t s = 0; s < 3; s++ )
      {

        newVertex1 = vertex1 + shift1 + 
                     normal * ( radiusOfInfluence -
                                radiusOfInfluence * ( float )s );
        newVertex2 = vertex2 + shift2 + 
                     normal * ( radiusOfInfluence -
                                radiusOfInfluence * ( float )s );
        newVertex3 = vertex3 + shift3 + 
                     normal * ( radiusOfInfluence -
                                radiusOfInfluence * ( float )s );

        getTriangleVoxels( newVertex1, newVertex2, newVertex3,
                           voxels, false );

      }

    }
    else
    {

      int32_t segmentCount = ( int32_t )( radiusOfInfluence /
                                          _minimumResolution ) + 1;
      float step = radiusOfInfluence / ( float )segmentCount;

      // first adding the central triangle
      getTriangleVoxels( vertex1 + shift1,
                         vertex2 + shift2,
                         vertex3 + shift3,
                         voxels, false );

      for ( int32_t s = 1; s <= segmentCount; s++ )
      {

        // along +normal direction
        getTriangleVoxels( vertex1 + shift1 + normal * step * ( float )s,
                           vertex2 + shift2 + normal * step * ( float )s,
                           vertex3 + shift3 + normal * step * ( float )s,
                           voxels, false );

        // along -normal direction
        getTriangleVoxels( vertex1 + shift1 - normal * step * ( float )s,
                           vertex2 + shift2 - normal * step * ( float )s,
                           vertex3 + shift3 - normal * step * ( float )s,
                           voxels, false );

      }

    }

  }

}


}

}

