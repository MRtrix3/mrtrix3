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


#include "command.h"
#include "header.h"
#include "surface/mesh.h"
#include "surface/algo/vertex_align.h"



using namespace MR;
using namespace App;
using namespace MR::Surface;


void usage ()
{

  AUTHOR = "Daan Christiaens (daan.christiaens@kcl.ac.uk) and "
           "Max Pietsch (maximilian.pietsch@kcl.ac.uk)";

  SYNOPSIS = "Rigid registration of source and target meshes.";

  ARGUMENTS
  + Argument ("source", "the source mesh file").type_file_in()
  + Argument ("target", "the target mesh file").type_file_in()
  + Argument ("transform", "the output transform").type_file_out();

}

void run ()
{

  // Read in the mesh data
  Mesh source (argument[0]);
  Mesh target (argument[1]);

  // Scaling
  bool scale = false;

  // Register
  Eigen::MatrixXd vsource = Surface::Algo::vert2mat(source.get_vertices());
  Eigen::MatrixXd vtarget = Surface::Algo::vert2mat(target.get_vertices());

  double cost, costopt;
  transform_type T, Topt;

  Topt = Surface::Algo::iterative_closest_point(vtarget, vsource, scale, costopt);
  for (int k = 0; k < 500; k++) {    // random restarts
    Eigen::Matrix3d R = Eigen::Quaterniond::UnitRandom().toRotationMatrix();
    T = Surface::Algo::iterative_closest_point(vtarget, vsource * R.transpose(), scale, cost);
    T.prerotate(R);
    if (cost < costopt) {
      Topt = T;
      costopt = cost;
    }
  }

  // Save transform
  save_transform (Topt, argument[2]);

}
