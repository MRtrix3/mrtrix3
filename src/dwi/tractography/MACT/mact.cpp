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

    + Option( "mact", "Mesh-based ACT framework. All relevent surface meshes of brain tissues need to be provides " )

    /*
     * Hard codes: options for adding tissue meshes
     */
    + Option( "cbr_gm", "outer surface mesh of cerebral cortex" )
      + Argument( "mesh" ).type_file_in()

    + Option( "cbr_wm", "inner surface mesh of cerebral cortex (GM-WM interface)" )
      + Argument( "mesh" ).type_file_in()

    + Option( "sgm", "surface mesh of sub-cortical gray matter" )
      + Argument( "mesh" ).type_file_in()

    + Option( "cbl_gm", "surface mesh of cerebellar gray matter" )
      + Argument( "mesh" ).type_file_in()

    + Option( "cbl_wm", "surface mesh of cerebellar white matter" )
      + Argument( "mesh" ).type_file_in()

    + Option( "bst", "surface mesh of brain stem" )
      + Argument( "mesh" ).type_file_in()

    + Option( "csf", "surface mesh of brain ventricles" )
      + Argument( "mesh" ).type_file_in()

    // other properties
    + Option( "lut", "edge length in mm for spatial lookup table (default=0.2mm)" )
      + Argument( "value" ).type_float( 0.0, 25.0 )

    + Option( "backtrack", "allow tracks to be truncated and re-tracked if a poor structural termination is encountered" )

    + Option( "crop_at_gmwmi", "crop streamline endpoints precisely on the surface as they cross the GM-WM interface" );


  void load_mact_properties( Properties& properties )
  {
    auto opt = get_options( "mact" );
    if ( opt.size() )
    {
      properties[ "mact" ] = "1";

      opt = get_options( "cbr_gm" );
      if ( opt.size() )
      {
        properties[ "mact_cbr_gm" ] = std::string( opt[ 0 ][ 0 ] );
      }
      else
      {
        throw Exception( "mact failed: no input outer brain mesh provided" );
      }
      opt = get_options( "cbr_wm" );
      if ( opt.size() )
      {
        properties[ "mact_cbr_wm" ] = std::string( opt[ 0 ][ 0 ] );
      }
      else
      {
        throw Exception( "mact failed: no input inner brain mesh provided" );
      }
      opt = get_options( "sgm" );
      if ( opt.size() )
      {
        properties[ "mact_sgm" ] = std::string( opt[ 0 ][ 0 ] );
      }
      else
      {
        throw Exception( "mact failed: no input sgm mesh provided" );
      }
      opt = get_options( "cbl_gm" );
      if ( opt.size() )
      {
        properties[ "mact_cbl_gm" ] = std::string( opt[ 0 ][ 0 ] );
      }
      else
      {
        throw Exception( "mact failed: no input cerebellar gm mesh provided" );
      }
      opt = get_options( "cbl_wm" );
      if ( opt.size() )
      {
        properties[ "mact_cbl_wm" ] = std::string( opt[ 0 ][ 0 ] );
      }
      else
      {
        throw Exception( "mact failed: no input cerebellar wm mesh provided" );
      }
      opt = get_options( "bst" );
      if ( opt.size() )
      {
        properties[ "mact_bst" ] = std::string( opt[ 0 ][ 0 ] );
      }
      else
      {
        throw Exception( "mact failed: no input bst mesh provided" );
      }
      opt = get_options( "csf" );
      if ( opt.size() )
      {
        properties[ "mact_csf" ] = std::string( opt[ 0 ][ 0 ] );
      }
      else
      {
        throw Exception( "mact failed: no input csf mesh provided" );
      }

      // other properties
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

