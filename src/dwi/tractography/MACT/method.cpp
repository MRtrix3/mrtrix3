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


#include "dwi/tractography/MACT/method.h"


#define CUSTOM_PRECISION 1e-5


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace MACT
{


using namespace MR::DWI::Tractography::Tracking;


MACT_Method_additions::MACT_Method_additions( const SharedBase& shared )
                      : _sgm_depth( 0 ),
                        _sceneModeller( shared.mact()._sceneModeller ),
                        _seed_in_sgm( false ),
                        _sgm_seed_to_wm( false ),
                        _point_in_sgm( false ),
                        _crop_at_gmwmi( shared.mact()._crop_at_gmwmi )
{
}


/* Method using 'tissue encapsulation': it can be compuationally intensive as it
 * requires checking every tissue type for each position
 *
term_t MACT_Method_additions::check_structural( const Eigen::Vector3f& pos )
{
  Eigen::Vector3d point = pos.cast< double >();
  if ( _sceneModeller->inTissue( point, CSF ) )
  {
    return ENTER_CSF;
  }
  else if ( _sceneModeller->inTissue( point, SGM ) )
  {
    return TERM_IN_SGM;
  }
  else if ( ( !_sceneModeller->inTissue( point, WM ) ) &&
            ( _sceneModeller->inTissue( point, CGM ) ) )
  {
    return ENTER_CGM;
  }
  else
  {
    return CONTINUE;
  }
}*/


term_t MACT_Method_additions::check_structural( const Eigen::Vector3f& old_pos,
                                                Eigen::Vector3f& new_pos )
{
  /*
   * Method using 'tck-mesh intersections': if using FS surfaces, need to crop
   * the meshes of the unknown regions and the corpus callosum for ?h.white and
   * ?h.pial surfaces
   */
  Eigen::Vector3d from = old_pos.cast< double >();
  Eigen::Vector3d to = new_pos.cast< double >();

  IntersectionSet intersections( *_sceneModeller, from, to );
  if ( intersections.count() )
  {
    auto firstIntersection = intersections.intersection( 0 );
    auto tissue = firstIntersection._tissue;

    if ( tissue->type() == CSF )
    {
      // return _sgm_depth ? EXIT_SGM : ENTER_CSF;
      return ENTER_CSF;
    }
    else if ( tissue->type() == SGM )
    {
      /* disable tracking into deep sgm for the moment:
         need to solve the issue of mesh overlapping */
      if ( _crop_at_gmwmi )
      {
        new_pos = firstIntersection._point.cast< float >();
      }
      return TERM_IN_SGM;
    }
    else if ( tissue->type() == CGM )
    {
      /* can pre-calculate polygon normal */
      auto v1 = tissue->mesh().vert( firstIntersection._triangle[ 0 ] );
      auto v2 = tissue->mesh().vert( firstIntersection._triangle[ 1 ] );
      auto v3 = tissue->mesh().vert( firstIntersection._triangle[ 2 ] );
      auto n = ( v2 - v1 ).cross( v3 - v1 );
      if ( n.dot( v1 - from ) < 0 )
      {
        // exclude this track since the point locates outside cgm
        return ENTER_EXCLUDE;
      }
      if ( _crop_at_gmwmi )
      {
        new_pos = firstIntersection._point.cast< float >();
      }
      return ENTER_CGM;
    }
  }
  if ( !_sceneModeller->boundingBox().contains( to ) )
  {
    // the point leaves the mesh bounding box, might need have a safety margin
    return EXIT_IMAGE;
  }
  return CONTINUE;
}


bool MACT_Method_additions::check_seed( Eigen::Vector3f& pos )
{
  Eigen::Vector3d p = pos.cast< double >();
  _sgm_depth = 0;

  if ( _sceneModeller->inTissue( p, CSF ) || 
       _sceneModeller->inTissue( p, SGM ) )
  {
    return false;
  }

  _seed_in_sgm = false;
  _sgm_seed_to_wm = false;
  _point_in_sgm = false;
  return true;
}


bool MACT_Method_additions::seed_is_unidirectional( Eigen::Vector3f& pos,
                                                    Eigen::Vector3f& dir ) const
{
  Eigen::Vector3d p = pos.cast< double >();
  Eigen::Vector3d d = dir.cast< double >();
  Intersection intersection;
  if ( _sceneModeller->onTissue( p, CGM, intersection ) )
  {
    // seed is considered to be on WM surface but can actually locate inside or
    // outside the surface due to numerical precision error
    auto mesh = intersection._tissue->mesh();
    auto v1 = mesh.vert( intersection._triangle[ 0 ] );
    auto v2 = mesh.vert( intersection._triangle[ 1 ] );
    auto v3 = mesh.vert( intersection._triangle[ 2 ] );
    auto n = ( v2 - v1 ).cross( v3 - v1 );
    n.normalize();
    if ( n.dot( d ) > 0 )
    {
      // normal and seed direction are pointing outward from the surface
      // --> change the polarity of dir
      d = -d;
      dir = d.cast< float >();
    }
    while ( n.dot( v1 - p ) < 0 )
    {
      // seed locates outside the surface
      // --> shift the seed until it crosses over the surface
      p -= n * CUSTOM_PRECISION;
    }
    pos = p.cast< float >();
    return true;
  }
  return false;
}


bool MACT_Method_additions::in_pathology() const
{
  return false;
}


void MACT_Method_additions::reverse_track()
{
  _sgm_depth = 0;
  _point_in_sgm = _seed_in_sgm;
}


}

}

}

}

