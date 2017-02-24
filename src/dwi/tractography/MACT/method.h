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


#ifndef __dwi_tractography_mact_method_h__
#define __dwi_tractography_mact_method_h__


#include "dwi/tractography/tracking/shared.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace MACT
{


using namespace MR::DWI::Tractography::Tracking;


class MACT_Method_additions
{

  public:

    MACT_Method_additions( const SharedBase& shared );

    MACT_Method_additions( const MACT_Method_additions& ) = delete;
    MACT_Method_additions() = delete;

    // term_t check_structural( const Eigen::Vector3f& pos );
    term_t check_structural( const Eigen::Vector3f& old_pos,
                             Eigen::Vector3f& new_pos );

    bool check_seed( Eigen::Vector3f& pos );
    bool seed_is_unidirectional( Eigen::Vector3f& pos,
                                 Eigen::Vector3f& dir ) const;

    bool in_pathology() const;
    void reverse_track();

    size_t _sgm_depth;

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW  // avoid memory alignment errors in Eigen3;

  private:

    bool _seed_in_sgm;
    bool _sgm_seed_to_wm;
    bool _point_in_sgm;
    bool _crop_at_gmwmi;
    std::shared_ptr< SceneModeller > _sceneModeller;

};


}

}

}

}


#endif

