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


#ifndef __dwi_tractography_mact_shared_h__
#define __dwi_tractography_mact_shared_h__


#include "dwi/tractography/MACT/scenemodeller.h"
#include "dwi/tractography/properties.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace MACT
{


class MACT_Shared_additions
{

  public:

    MACT_Shared_additions( Properties& property_set );

    bool backtrack() const;
    // bool crop_at_gmwmi() const;
    // void crop_at_gmwmi( std::vector< Eigen::Vector3f >& tck ) const;

  private:

    Image< float > _source;
    bool _backtrack;
    bool _crop_at_gmwmi;
    std::shared_ptr< SceneModeller > _sceneModeller;

  protected:

    friend class MACT_Method_additions;

};


}

}

}

}


#endif

