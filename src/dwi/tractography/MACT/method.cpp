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
                        _seed_in_sgm( false ),
                        _sgm_seed_to_wm( false ),
                        _point_in_sgm( false ),
                        _crop_at_gmwmi( shared.mact()._crop_at_gmwmi ),
                        _sceneModeller( shared.mact()._sceneModeller )
{
}


const std::shared_ptr< SceneModeller >&
MACT_Method_additions::sceneModeller() const
{
  return _sceneModeller;
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
      return _sgm_depth ? EXIT_SGM : ENTER_CSF;
    }
    else if ( tissue->type() == WM )
    {
      if ( _crop_at_gmwmi )
      {
        new_pos = firstIntersection._point.cast< float >();
      }
      return ENTER_CGM;
    }
    else if ( tissue->type() == SGM )
    {
      if ( _sgm_depth )
      {
        // the seed was already inside SGM
        if ( _seed_in_sgm && !_sgm_seed_to_wm )
        {
          // the seed moves from sgm to wm
          _sgm_seed_to_wm = true;
          _sgm_depth = 0;
          _point_in_sgm = false;
          return CONTINUE;
        }
        return EXIT_SGM;
      }
      else if ( !_sgm_depth && _seed_in_sgm && !_sgm_seed_to_wm )
      {
        // the seed moves from sgm to wm at the first tracking step
        _sgm_seed_to_wm = true;
        _sgm_depth = 0;
        _point_in_sgm = false;
        return CONTINUE;
      }
      else
      {
        _point_in_sgm = true;
      }
    }
    else if ( tissue->type() == CGM )
    {
      // outer surface can be used to prevent cross sulcus connections
      // For instance:
      // if the old position is above the CGM
      //   reject the track
      // else
      //   ENTER_CGM
      return ENTER_CGM;
    }
    /* possible method to deal with cerebellum
    else if ( tissue->type() == CBM )
    {
      if ( _sceneModeller->inTissue( from, CBM_WHITE ) )
      {
        // the track's coming from white matter of CBM -> keep the track
        // return ENTER_CGM;
      }
      else
      {
        // the track's coming outside the brain -> reject the track
      }
    }*/
  }
  if ( _point_in_sgm )
  {
    ++ _sgm_depth;
  }

  return CONTINUE;
}


bool MACT_Method_additions::check_seed( const Eigen::Vector3f& pos )
{
  Eigen::Vector3d point = pos.cast< double >();
  _sgm_depth = 0;
  if ( _sceneModeller->inTissue( point, CSF ) )
  {
    return false;
  }
  if ( _sceneModeller->inTissue( point, SGM ) )
  {
    _seed_in_sgm = true;
    _sgm_seed_to_wm = false;
    _point_in_sgm = true;
    return true;
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
  if ( _sceneModeller->onTissue( p, WM, intersection ) )
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


bool MACT_Method_additions::fetch_tissue_data( const Eigen::Vector3f& pos )
{
  return true;
}


bool MACT_Method_additions::in_pathology() const
{
  return false;
}


void MACT_Method_additions::reverse_track()
{
  _sgm_depth = 0;
}


}

}

}

}

