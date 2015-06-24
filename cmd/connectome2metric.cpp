/*
    Copyright 2015 Brain Research Institute, Melbourne, Australia

    Written by Chun-Hung Jimmy Yeh, 2015.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/


#include "command.h"
#include "dwi/tractography/connectomics/graph_metrics.h"


using namespace MR;
using namespace App;
using namespace MR::DWI;
using namespace MR::DWI::Tractography;


void usage ()
{

  AUTHOR = "C.-H. Jimmy Yeh (j.yeh@brain.org.au)";

  DESCRIPTION
  + "perform graphy theoretical analysis";

  ARGUMENTS
  + Argument( "connectome_in",
              "the input connectome file (.csv)").type_file_in();

  OPTIONS
  + Option( "density", "diplay connectome density" )

  + Option( "degree", "output degree per node" )
    + Argument( "file" ).type_file_out()

  + Option( "strength", "output strength per node" )
    + Argument( "file" ).type_file_out();

};


void run ()
{

  // sanity checks
  bool metricSelected = false;
  Options opt = get_options( "density" );
  if ( opt.size() )
  {
    metricSelected = true;
  }
  opt = get_options( "degree" );
  if ( opt.size() )
  {
    metricSelected = true;
  }
  opt = get_options( "strength" );
  if ( opt.size() )
  {
    metricSelected = true;
  }
  if ( !metricSelected )
  {
    throw Exception( "No output connectomic metric is selected." );
  }

  // reading connectome file
  Connectomics::Connectome* connectome = new Connectomics::Connectome();
  connectome->read( argument[ 0 ] );
  Connectomics::GraphMetrics graphMetrics( connectome );

  // collecting metrics on request
  opt = get_options( "density" );
  if ( opt.size() )
  {

    std::cout << graphMetrics.density() << std::endl;

  }
  opt = get_options( "degree" );
  if ( opt.size() )
  {

    graphMetrics.write( opt[ 0 ][ 0 ], graphMetrics.degree() );

  }
  opt = get_options( "strength" );
  if ( opt.size() )
  {

    graphMetrics.write( opt[ 0 ][ 0 ], graphMetrics.strength() );

  }

}

