/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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
#include "progressbar.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "point.h"
#include "dwi/directions/file.h"
#include "dwi/gradient.h"



using namespace MR;
using namespace App;

void usage () {

  DESCRIPTION
    + "report statistics on a direction set";

  ARGUMENTS
    + Argument ("dirs", "the text file containing the directions.").type_file_in();
}


typedef double value_type;





void report (const std::string& title, const Math::Matrix<value_type>& directions) 
{
  std::vector<value_type> NN_bipolar (directions.rows(), 0.0);
  std::vector<value_type> NN_unipolar (directions.rows(), 0.0);

  std::vector<value_type> E_bipolar (directions.rows(), 0.0);
  std::vector<value_type> E_unipolar (directions.rows(), 0.0);

  for (size_t i = 0; i < directions.rows()-1; ++i) {
    for (size_t j = i+1; j < directions.rows(); ++j) {
      value_type cos_angle = Math::dot (directions.row(i).sub(0,3), directions.row(j).sub(0,3));
      NN_unipolar[i] = std::max (NN_unipolar[i], cos_angle);
      NN_unipolar[j] = std::max (NN_unipolar[j], cos_angle);
      cos_angle = std::abs(cos_angle);
      NN_bipolar[i] = std::max (NN_bipolar[i], cos_angle);
      NN_bipolar[j] = std::max (NN_bipolar[j], cos_angle);

      value_type E = 
        Math::pow2 (directions(i,0) - directions(j,0)) + 
        Math::pow2 (directions(i,1) - directions(j,1)) + 
        Math::pow2 (directions(i,2) - directions(j,2));
      E = value_type (1.0) / E;

      E_unipolar[i] += E;
      E_unipolar[j] += E;

      value_type E2 = 
        Math::pow2 (directions(i,0) + directions(j,0)) + 
        Math::pow2 (directions(i,1) + directions(j,1)) + 
        Math::pow2 (directions(i,2) + directions(j,2));
      E += value_type (1.0) / E2;

      E_bipolar[i] += E;
      E_bipolar[j] += E;

    }
  }




  auto report_NN = [](const std::vector<value_type>& NN) {
    value_type min = std::numeric_limits<value_type>::max();
    value_type mean = 0.0;
    value_type max = 0.0;
    for (auto a : NN) {
      a = (180.0/Math::pi) * std::acos (a);
      mean += a;
      min = std::min (min, a);
      max = std::max (max, a);
    }
    mean /= NN.size();

    print ("    nearest-neighbour angles: mean = " + str(mean) + ", range [ " + str(min) + " - " + str(max) + " ]\n");
  };



  auto report_E = [](const std::vector<value_type>& E) {
    value_type min = std::numeric_limits<value_type>::max();
    value_type total = 0.0;
    value_type max = 0.0;
    for (auto e : E) {
      total += e;
      min = std::min (min, e);
      max = std::max (max, e);
    }
    print ("    energy: total = " + str(total) + ", mean = " + str(total/E.size()) + ", range [ " + str(min) + " - " + str(max) + " ]\n");
  };




  print (title + " [ " + str(directions.rows()) + " directions ]\n\n");

  print ("  Bipolar electrostatic repulsion model:\n");
  report_NN (NN_bipolar);
  report_E (E_bipolar);

  print ("\n  Unipolar electrostatic repulsion model:\n");
  report_NN (NN_unipolar);
  report_E (E_unipolar);

  std::string lmax_results;
  for (size_t lmax = 2; lmax <= Math::SH::LforN (directions.rows()); lmax += 2) 
    lmax_results += " " + str(DWI::condition_number_for_lmax (directions, lmax));
  print ("    condition numbers for lmax = " + str(2) + " -> " 
      + str(Math::SH::LforN (directions.rows())) + ":" + lmax_results + "\n\n");
}



void run () 
{
  try {
    Math::Matrix<value_type> directions = DWI::Directions::load_cartesian<value_type> (argument[0]);
    report (str(argument[0]), directions);
  }
  catch (Exception& E) {
    Math::Matrix<value_type> directions (str(argument[0]));
    DWI::normalise_grad (directions);
    if (directions.columns() < 3) 
      throw Exception ("unexpected matrix size for DW scheme \"" + str(argument[0]) + "\"");

    print (str(argument[0]) + " [ " + str(directions.rows()) + " volumes ]\n");
    DWI::Shells shells (directions);

    for (size_t n = 0; n < shells.count(); ++n) {
      Math::Matrix<value_type> subset (shells[n].count(), 3);
      for (size_t i = 0; i < subset.rows(); ++i)
        subset.row(i) = directions.row(shells[n].get_volumes()[i]).sub(0,3);
      report ("\nb = " + str(shells[n].get_mean()), subset);
    }
  }
}

