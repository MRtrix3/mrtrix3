/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "dwi/tractography/MACT/mact.h"
#include "dwi/tractography/properties.h"


namespace MR
{

namespace DWI
{

namespace Tractography
{

namespace MACT
{

  using namespace App;

  const OptionGroup MACTOption = OptionGroup( "Mesh-based Anatomically-Constrained Tractography options" )

    + Option( "mact", "use the Mesh-based Anatomically-Constrained Tractography framework during tracking; "
                      "provide all relevent surface mesh files" )
      + Argument( "cgm", "surface of cortical grey matter (cerebrum & cerebellum merged)" ).type_file_in()
      + Argument( "sgm", "surface of subcortical grey matter" ).type_file_in()
      + Argument( "bst", "surface of brain stem" ).type_file_in()
      + Argument( "csf", "surface of ventricles" ).type_file_in()

    + Option( "lut", "edge length in mm for surface lookup table (default=0.2mm)" )
      + Argument( "value" ).type_float( 0.0, 25.0 )

    + Option( "backtrack", "allow tracks to be truncated and re-tracked if a poor structural termination is encountered" )

    + Option( "crop_at_gmwmi", "crop streamline endpoints precisely on the surface as they cross the GM-WM interface" );


  void load_mact_properties( Properties& properties )
  {
    auto opt = get_options( "mact" );
    if ( opt.size() )
    {
      properties[ "mact" ] = "1";
      properties[ "mact_cgm" ] = std::string( opt[ 0 ][ 0 ] );
      properties[ "mact_sgm" ] = std::string( opt[ 0 ][ 1 ] );
      properties[ "mact_bst" ] = std::string( opt[ 0 ][ 2 ] );
      properties[ "mact_csf" ] = std::string( opt[ 0 ][ 3 ] );

      opt = get_options( "lut" );
      properties[ "mact_lut" ] = opt.size() ? std::string( opt[ 0 ][ 0 ] ) :
                                              std::to_string( 0.2 );
      opt = get_options( "backtrack" );
      if ( opt.size() )
      {
        properties[ "backtrack" ] = "1";
      }
      opt = get_options( "crop_at_gmwmi" );
      if ( opt.size() )
      {
        properties[ "crop_at_gmwmi" ] = "1";
      }

    }
    else
    {
      if ( get_options( "backtrack" ).size() )
      {
        WARN( "ignoring -backtrack option - only valid if using ACT or MACT" );
      }
      if ( get_options ("crop_at_gmwmi").size() )
      {
        WARN( "ignoring -crop_at_gmwmi option - only valid if using ACT or MACT" );
      }
    }
  }

}

}

}

}

