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



#include "connectome/connectome.h"
#include "file/ofstream.h"

#include "exception.h"
#include "mrtrix.h"


namespace MR {
namespace Connectome {


matrix_type read_matrix( const std::string& path )
{
  std::ifstream cm_in( path, std::ios_base::in );
  if ( !cm_in )
  {
    throw Exception( "Unable to open lookup table file" );
  }
  std::string line;
  std::vector< float > values;
  node_t num_rows = 0;
  while ( std::getline( cm_in, line ) )
  {
    std::stringstream line_stream( line );
    float value;
    while ( !line_stream.eof() )
    {
      line_stream >> value;
      if ( !line_stream )
      {
        break;
      }
      values.push_back( value );
    }
    ++ num_rows;
  }
  cm_in.close();

  node_t num_cols = ( size_t )( values.size() / num_rows + 0.5 );
  if ( num_rows != num_cols )
  {
    throw Exception( "Input is not a square matrix" );
  }

  node_t num_nodes = num_rows;
  matrix_type cm( num_nodes, num_nodes );
  for ( node_t r = 0; r != num_nodes; r++ )
  {
    for ( node_t c = 0; c != num_nodes; c++ )
    {
      cm( r, c ) = values[ r * num_nodes + c ];
    }
  }
  // Connectome::verify_matrix( cm, num_nodes );
  return cm;
}


void verify_matrix (matrix_type& in, const node_t num_nodes)
{
  if (in.rows() != in.cols())
    throw Exception ("Connectome matrix is not square (" + str(in.rows()) + " x " + str(in.cols()) + ")");
  if (in.rows() != num_nodes)
    throw Exception ("Connectome matrix contains " + str(in.rows()) + " nodes; expected " + str(num_nodes));

  for (node_t row = 0; row != num_nodes; ++row) {
    for (node_t column = row+1; column != num_nodes; ++column) {

      const float lower_value = in (column, row);
      const float upper_value = in (row, column);

      // if (upper_value && lower_value && (upper_value != lower_value))
      if (upper_value != lower_value)
        throw Exception ("Connectome matrix is not symmetrical");

      if (!upper_value && lower_value)
        in (row, column) = lower_value;

      in (column, row) = 0.0f;

  } }
}


}
}


