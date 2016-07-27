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



#include "command.h"
#include "progressbar.h"

#include "connectome/connectome.h"
#include "connectome/graph_theory.h"


using namespace MR;
using namespace App;
using namespace MR::Connectome;


void usage ()
{

  AUTHOR = "Chun-Hung Jimmy Yeh (chun-hung.yeh@florey.edu.au)";

  DESCRIPTION
  + "compute connectomic metrics.";

  ARGUMENTS
  + Argument( "matrix_in", "the connectome matrix file" ).type_file_in();

  OPTIONS
  + OptionGroup( "Connectome pre-processing options" )

  + Option( "exclude", "exclude the specified node from connectome (multiple nodes can be specified)." )
    + Argument( "nodes" ).type_sequence_int()

  + Option( "zero_diagonal", "set diagonal coefficients to zero" )

  + Option( "symmetrise", "symmetrise connectivity matrix (lower triangle = upper triangle)" )

  + Option( "export", "output pre-processed connectivity matrix" )
    + Argument( "path" ).type_file_out();

}


void run ()
{

  GraphTheory graph_theory( Connectome::read_matrix( argument[0] ) );

  // pre-processing connectivity matrix if required
  auto opt = get_options( "exclude" );
  if ( opt.size() )
  {
    std::set< node_t > unwanted;
    auto nodes_in = parse_ints( opt[0][0] );
    for ( auto n = nodes_in.begin(); n != nodes_in.end(); n++ )
    {
      unwanted.insert( (node_t)(*n) );
    }
    graph_theory.exclude( unwanted );
  }

  opt = get_options( "zero_diagonal" );
  if ( opt.size() )
  {
    graph_theory.zero_diagonal();
  }

  opt = get_options( "symmetrise" );
  if ( opt.size() )
  {
    graph_theory.symmetrise();
  }

  opt = get_options( "export" );
  if ( opt.size() )
  {
    graph_theory.write_matrix( opt[0][0] );
  }

  // computing and display metrics
  graph_theory.weight_conversion();
  graph_theory.print_global();

}

