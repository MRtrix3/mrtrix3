#ifndef __mesh_bresenham_line_algorithm_h__
#define __mesh_bresenham_line_algorithm_h__

#include "mesh/bounding_box.h"
#include <set>


namespace MR
{


namespace Mesh
{


class BresenhamLineAlgorithm
{

  public:

    BresenhamLineAlgorithm( const BoundingBox< float >& boundingBox,
                            const Point< int32_t >& cacheSize );
    virtual ~BresenhamLineAlgorithm();

    void getVoxelFromPoint( const Point< float >& point,
                            Point< int32_t >& voxel ) const;

    void getNeighbouringVoxels( const Point< int32_t >& voxel,
                                Point< int32_t > stride,
                                std::vector< Point< int32_t > >& neighbors,
                                std::vector< bool >& neighborMasks ) const;

    void getRayVoxels( const Point< float >& from,
                       const Point< float >& to,
                       std::set< Point< int32_t >,
                                 PointCompare< int32_t > >& voxels,
                       bool clearVoxelsAtBeginning = true ) const;

    void getTriangleVoxels( const Point< float >& vertex1,
                            const Point< float >& vertex2,
                            const Point< float >& vertex3,
                            std::set< Point< int32_t >,
                                      PointCompare< int32_t > >& voxels,
                            bool clearVoxelsAtBeginning = true ) const;

    void getThickTriangleVoxels( const Point< float >& vertex1,
                                 const Point< float >& vertex2,
                                 const Point< float >& vertex3,
                                 float radiusOfInfluence,
                                 std::set< Point< int32_t >,
                                           PointCompare< int32_t > >& voxels,
                                 bool clearVoxelsAtBeginning = true ) const;

  protected:

    Point< int32_t > _cacheSize;
    Point< int32_t > _cacheSizeMinusOne;
    float _lowerX;
    float _upperX;
    float _lowerY;
    float _upperY;
    float _lowerZ;
    float _upperZ;
    float _cacheVoxelFactorX;
    float _cacheVoxelFactorY;
    float _cacheVoxelFactorZ;
    float _minimumResolution;

};


}


}


#endif

