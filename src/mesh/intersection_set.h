#ifndef __mesh_intersection_set_h__
#define __mesh_intersection_set_h__

#include "mesh/scene_modeller.h"
#include <map>


namespace MR
{


namespace Mesh
{


//
// struct Intersection
//

struct Intersection
{

  public:

    Intersection();
    Intersection( float theArcLength,
                  const Point< float >& thePoint,
                  SceneMesh* theSceneMesh,
                  const Polygon< 3 >& thePolygon );
    Intersection( const Intersection& other );
    
    Intersection& operator=( const Intersection& other );
    
    float _arcLength;
    Point< float > _point;
    SceneMesh* _sceneMesh;
    Polygon< 3 > _polygon;
    
};


//
// class IntersectionSet
//

class IntersectionSet
{

  public:

    IntersectionSet( const SceneModeller& sceneModeller,
                     const Point< float >& from,
                     const Point< float >& to );
    virtual ~IntersectionSet();

    int32_t getCount() const;
    const Intersection& getIntersection( int32_t index ) const;
    void eraseIntersection( int32_t index );

  protected:

    bool getRayTriangleIntersection( const Point< float >& from,
                                     const Point< float >& to,
                                     const Point< float >& vertex1,
                                     const Point< float >& vertex2,
                                     const Point< float >& vertex3, 
                                     Point< float >& intersectionPoint ) const;

    std::vector< float > _arcLengths;
    std::map< float, Intersection > _intersections;

};


}


}


#endif

