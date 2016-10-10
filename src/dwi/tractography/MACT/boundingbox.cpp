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


#include "dwi/tractography/MACT/boundingbox.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace MACT
{


template < class T >
BoundingBox< T >::BoundingBox( const T& lowerX, const T& upperX,
                               const T& lowerY, const T& upperY,
                               const T& lowerZ, const T& upperZ,
                               const T& lowerT, const T& upperT )
                 : _lowerX( lowerX ), _upperX( upperX ),
                   _lowerY( lowerY ), _upperY( upperY ),
                   _lowerZ( lowerZ ), _upperZ( upperZ ),
                   _lowerT( lowerT ), _upperT( upperT )
{
}


template < class T >
BoundingBox< T >::BoundingBox( const BoundingBox< T >& other )
                 : _lowerX( other._lowerX ), _upperX( other._upperX ),
                   _lowerY( other._lowerY ), _upperY( other._upperY ),
                   _lowerZ( other._lowerZ ), _upperZ( other._upperZ ),
                   _lowerT( other._lowerT ), _upperT( other._upperT )
{
}


template < class T >
BoundingBox< T >::~BoundingBox()
{
}


template < class T >
BoundingBox< T >&  BoundingBox< T >::operator=( const BoundingBox< T >& other )
{
  _lowerX = other._lowerX;
  _upperX = other._upperX;
  _lowerY = other._lowerY;
  _upperY = other._upperY;
  _lowerZ = other._lowerZ;
  _upperZ = other._upperZ;
  _lowerT = other._lowerT;
  _upperT = other._upperT;

  return *this;
}


template < class T >
void BoundingBox< T >::setLowerX( const T& lowerX )
{
  _lowerX = lowerX;
}


template < class T >
const T& BoundingBox< T >::getLowerX() const
{
  return _lowerX;
}


template < class T >
void BoundingBox< T >::setUpperX( const T& upperX )
{
  _upperX = upperX;
}


template < class T >
const T& BoundingBox< T >::getUpperX() const
{
  return _upperX;
}


template < class T >
void BoundingBox< T >::setLowerY( const T& lowerY )
{
  _lowerY = lowerY;
}


template < class T >
const T& BoundingBox< T >::getLowerY() const
{
  return _lowerY;
}


template < class T >
void BoundingBox< T >::setUpperY( const T& upperY )
{
  _upperY = upperY;
}


template < class T >
const T& BoundingBox< T >::getUpperY() const
{
  return _upperY;
}


template < class T >
void BoundingBox< T >::setLowerZ( const T& lowerZ )
{
  _lowerZ = lowerZ;
}


template < class T >
const T& BoundingBox< T >::getLowerZ() const
{
  return _lowerZ;
}


template < class T >
void BoundingBox< T >::setUpperZ( const T& upperZ )
{
  _upperZ = upperZ;
}


template < class T >
const T& BoundingBox< T >::getUpperZ() const
{
  return _upperZ;
}


template < class T >
void BoundingBox< T >::setLowerT( const T& lowerT )
{
  _lowerT = lowerT;
}


template < class T >
const T& BoundingBox< T >::getLowerT() const
{
  return _lowerT;
}


template < class T >
void BoundingBox< T >::setUpperT( const T& upperT )
{
  _upperT = upperT;
}


template < class T >
const T& BoundingBox< T >::getUpperT() const
{
  return _upperT;
}


template < class T >
bool BoundingBox< T >::contains( const T& x, const T& y,
                                 const T& z, const T& t ) const
{
  return ( x >= _lowerX ) && ( x <= _upperX ) &&
         ( y >= _lowerY ) && ( y <= _upperY ) &&
         ( z >= _lowerZ ) && ( z <= _upperZ ) &&
         ( t >= _lowerT ) && ( t <= _upperT );
}


template < class T >
bool BoundingBox< T >::contains( const Eigen::Matrix< T, 3, 1 >& site,
                                 const T& t ) const
{
  return ( site[ 0 ] >= _lowerX ) && ( site[ 0 ] <= _upperX ) &&
         ( site[ 1 ] >= _lowerY ) && ( site[ 1 ] <= _upperY ) &&
         ( site[ 2 ] >= _lowerZ ) && ( site[ 2 ] <= _upperZ ) &&
         ( t >= _lowerT ) && ( t <= _upperT );
}


template < class T >
bool BoundingBox< T >::contains( const BoundingBox< T >& other ) const
{
  return ( other.getLowerX() >= _lowerX ) && ( other.getUpperX() <= _upperX ) &&
         ( other.getLowerY() >= _lowerY ) && ( other.getUpperY() <= _upperY ) &&
         ( other.getLowerZ() >= _lowerZ ) && ( other.getUpperZ() <= _upperZ ) &&
         ( other.getLowerT() >= _lowerT ) && ( other.getUpperT() <= _upperT );
}


template < class T >
bool BoundingBox< T >::onBoundary( const T& x, const T& y,
                                   const T& z, const T& t ) const
{
  bool result = false;

  // the bounding box is 3D
  if ( _upperT == 0 )
  {
    // the bounding box is 2D
    if ( _upperZ == 0 )
    {
      result = ( ( x == _lowerX ) || ( x == _upperX ) ||
                 ( y == _lowerY ) || ( y == _upperY ) ) &&
               contains( x, y, z, t );
    }
    // the bounding box is really 3D
    else
    {
      result = ( ( x == _lowerX ) || ( x == _upperX ) ||
                 ( y == _lowerY ) || ( y == _upperY ) ||
                 ( z == _lowerZ ) || ( z == _upperZ ) ) &&
               contains( x, y, z, t );
    }
  }
  // else it is a 4D container
  else
  {
    result = ( ( x == _lowerX ) || ( x == _upperX ) ||
               ( y == _lowerY ) || ( y == _upperY ) ||
               ( z == _lowerZ ) || ( z == _upperZ ) ||
               ( t == _lowerT ) || ( t == _upperT ) ) &&
             contains( x, y, z, t );
  }

  return result;
}


template < class T >
bool BoundingBox< T >::onBoundary( const Eigen::Matrix< T, 3, 1 >& site,
                                   const T& t ) const
{
  return onBoundary( site[ 0 ], site[ 1 ], site[ 2 ], t );
}


//
// Instanciations
//
template class BoundingBox< int8_t >;
template class BoundingBox< int16_t >;
template class BoundingBox< int32_t >;
template class BoundingBox< int64_t >;
template class BoundingBox< float >;
template class BoundingBox< double >;


}

}

}

}

