#ifndef __mesh_bounding_box_h__
#define __mesh_bounding_box_h__

#include "point.h"


namespace MR
{


namespace Mesh
{


template < class T >
class BoundingBox
{

  public:

    BoundingBox( const T& lowerX = 0, const T& upperX = 0,
                 const T& lowerY = 0, const T& upperY = 0,
                 const T& lowerZ = 0, const T& upperZ = 0,
                 const T& lowerT = 0, const T& upperT = 0 );
    BoundingBox( const BoundingBox< T >& other );
    virtual ~BoundingBox();

    BoundingBox< T >& operator=( const BoundingBox< T >& other );

    void setLowerX( const T& lowerX );
    const T& getLowerX() const;

    void setUpperX( const T& upperX );
    const T& getUpperX() const;

    void setLowerY( const T& lowerY );
    const T& getLowerY() const;

    void setUpperY( const T& upperY );
    const T& getUpperY() const;

    void setLowerZ( const T& lowerZ );
    const T& getLowerZ() const;

    void setUpperZ( const T& upperZ );
    const T& getUpperZ() const;

    void setLowerT( const T& lowerT );
    const T& getLowerT() const;

    void setUpperT( const T& upperT );
    const T& getUpperT() const;

    bool contains( const T& x = 0,
                   const T& y = 0,
                   const T& z = 0,
                   const T& t = 0 ) const;
    bool contains( const Point< T >& site,
                   const T& t = 0 ) const;
    bool contains( const BoundingBox< T >& other ) const;
    bool isOnBoundary( const T& x = 0,
                       const T& y = 0,
                       const T& z = 0,
                       const T& t = 0 ) const;
    bool isOnBoundary( const Point< T >& site,
                       const T& t = 0 ) const;

  protected:

    T _lowerX, _upperX;
    T _lowerY, _upperY;
    T _lowerZ, _upperZ;
    T _lowerT, _upperT;


};


}


}


#endif

