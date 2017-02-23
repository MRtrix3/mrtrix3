/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "command.h"
#include "progressbar.h"
#include "dwi/directions/file.h"
#include "dwi/gradient.h"



using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Report statistics on a direction set";

  ARGUMENTS
    + Argument ("dirs", "the text file containing the directions.").type_file_in();
}



int precision = 6;




void report (const std::string& title, const Eigen::MatrixXd& directions) 
{
  vector<double> NN_bipolar (directions.rows(), -1.0);
  vector<double> NN_unipolar (directions.rows(), -1.0);

  vector<double> E_bipolar (directions.rows(), 0.0);
  vector<double> E_unipolar (directions.rows(), 0.0);

  for (ssize_t i = 0; i < directions.rows()-1; ++i) {
    for (ssize_t j = i+1; j < directions.rows(); ++j) {
      double cos_angle = directions.row(i).head(3).normalized().dot (directions.row(j).head(3).normalized());
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




  auto report_NN = [](const vector<double>& NN) {
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



  auto report_E = [](const vector<double>& E) {
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

