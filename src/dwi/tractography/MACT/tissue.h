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


#ifndef __dwi_tractography_mact_tissue_h__
#define __dwi_tractography_mact_tissue_h__


#include "dwi/tractography/MACT/polygonlut.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace MACT
{


class SceneModeller;


enum TissueType
{
  CBR_GM, CBR_WM, SGM, CBL_GM, CBL_WM, CSF, BST, OTHER
};


class Tissue
{

  public:

    Tissue( const TissueType& type,
            const std::string& name,
            const Surface::Mesh& mesh,
            const std::shared_ptr< SceneModeller >& sceneModeller,
            double radiusOfInfluence = 0.0 );
    virtual ~Tissue();

    TissueType type() const;
    std::string name() const;
    Surface::Mesh mesh() const;
    std::shared_ptr< SceneModeller > sceneModeller() const;
    double radiusOfInfluence() const;

    size_t polygonCount() const;
    const PolygonLut& polygonLut() const;

  private:

    TissueType _type;
    std::string _name;
    Surface::Mesh _mesh;
    std::shared_ptr< SceneModeller > _sceneModeller;
    double _radiusOfInfluence;
    PolygonLut _polygonLut;

};


}

}

}

}

#endif

