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



int precision = 6;




void report (const std::string& title, const Eigen::MatrixXd& directions) 
{
  std::vector<double> NN_bipolar (directions.rows(), 0.0);
  std::vector<double> NN_unipolar (directions.rows(), 0.0);

  std::vector<double> E_bipolar (directions.rows(), 0.0);
  std::vector<double> E_unipolar (directions.rows(), 0.0);

  for (ssize_t i = 0; i < directions.rows()-1; ++i) {
    for (ssize_t j = i+1; j < directions.rows(); ++j) {
      double cos_angle = directions.row(i).head(3).dot (directions.row(j).head(3));
      NN_unipolar[i] = std::max (NN_unipolar[i], cos_angle);
      NN_unipolar[j] = std::max (NN_unipolar[j], cos_angle);
      cos_angle = std::abs (cos_angle);
      NN_bipolar[i] = std::max (NN_bipolar[i], cos_angle);
      NN_bipolar[j] = std::max (NN_bipolar[j], cos_angle);

      double E = 1.0 / (directions.row(i).head(3) - directions.row(j).head(3)).squaredNorm();

      E_unipolar[i] += E;
      E_unipolar[j] += E;

      E += 1.0 / (directions.row(i).head(3) + directions.row(j).head(3)).squaredNorm();

      E_bipolar[i] += E;
      E_bipolar[j] += E;

    }
  }




  auto report_NN = [](const std::vector<double>& NN) {
    double min = std::numeric_limits<double>::max();
    double mean = 0.0;
    double max = 0.0;
    for (auto a : NN) {
      a = (180.0/Math::pi) * std::acos (a);
      mean += a;
      min = std::min (min, a);
      max = std::max (max, a);
    }
    mean /= NN.size();

    print ("    nearest-neighbour angles: mean = " + str(mean, precision) + ", range [ " + str(min, precision) + " - " + str(max, precision) + " ]\n");
  };



  auto report_E = [](const std::vector<double>& E) {
    double min = std::numeric_limits<double>::max();
    double total = 0.0;
    double max = 0.0;
    for (auto e : E) {
      total += e;
      min = std::min (min, e);
      max = std::max (max, e);
    }
    print ("    energy: total = " + str(total, precision) + ", mean = " + str(total/E.size(), precision) + ", range [ " + str(min, precision) + " - " + str(max, precision) + " ]\n");
  };




  print (title + " [ " + str(directions.rows(), precision) + " directions ]\n\n");

  print ("  Bipolar electrostatic repulsion model:\n");
  report_NN (NN_bipolar);
  report_E (E_bipolar);

  print ("\n  Unipolar electrostatic repulsion model:\n");
  report_NN (NN_unipolar);
  report_E (E_unipolar);

  std::string lmax_results;
  for (size_t lmax = 2; lmax <= Math::SH::LforN (directions.rows()); lmax += 2) 
    lmax_results += " " + str(DWI::condition_number_for_lmax (directions, lmax), precision);
  print ("\n  Spherical Harmonic fit:\n    condition numbers for lmax = " + str(2) + " -> " 
      + str(Math::SH::LforN (directions.rows()), precision) + ":" + lmax_results + "\n\n");
}



void run () 
{
  try {
    auto directions = DWI::Directions::load_cartesian (argument[0]);
    report (argument[0], directions);
  }
  catch (Exception& E) {
    auto directions = load_matrix<double> (argument[0]);
    DWI::normalise_grad (directions);
    if (directions.cols() < 3) 
      throw Exception ("unexpected matrix size for DW scheme \"" + str(argument[0]) + "\"");

    print (str(argument[0]) + " [ " + str(directions.rows()) + " volumes ]\n");
    DWI::Shells shells (directions);

    for (size_t n = 0; n < shells.count(); ++n) {
      Eigen::MatrixXd subset (shells[n].count(), 3);
      for (ssize_t i = 0; i < subset.rows(); ++i)
        subset.row(i) = directions.row(shells[n].get_volumes()[i]).head(3);
      report ("\nb = " + str(shells[n].get_mean(), precision), subset);
    }
  }
}

