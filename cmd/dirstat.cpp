/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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
#include "header.h"
#include "dwi/directions/file.h"
#include "dwi/gradient.h"
#include "dwi/shells.h"



using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Report statistics on a direction set";

  ARGUMENTS
    + Argument ("dirs", "the text file or image containing the directions.").type_file_in();

  OPTIONS
    + OptionGroup ("Output options")
    + Option ("bipolar", "output statistics for bipolar electrostatic repulsion model")
    + Option ("unipolar", "output statistics for unipolar electrostatic repulsion model")
    + Option ("shfit", "output statistics for spherical harmonics fit")
    + Option ("symmetry", "output measure of symmetry of spherical coverage (as given by the norm of mean direction vector). This is important to ensure minimal bias due to eddy-currents.")
    + Option ("nearest_neighour", "output nearest-neighbour angle statistics")
    + Option ("energy", "output energy statistics")
    + Option ("total", "output total of statistic (affects -energy only)")
    + Option ("mean", "output mean of statistic (affects -nearest_neighbour or -energy only)")
    + Option ("range", "output range of statistic (affects -nearest_neighbour or -energy only)")
    + DWI::GradImportOptions()
    + DWI::ShellOption;
}



int precision = 6;

void report (const std::string& title, Eigen::MatrixXd& directions);

bool bipolar = false;
bool unipolar = false;
bool shfit = false;
bool nearest_neighour = false;
bool energy = false;
bool symmetry = false;
bool total = false;
bool mean = false;
bool range = false;



void run () 
{
  bipolar = get_options ("bipolar").size();
  unipolar = get_options ("unipolar").size();
  shfit = get_options ("shfit").size();
  symmetry = get_options ("symmetry").size();
  nearest_neighour = get_options ("nearest_neighour").size();
  energy = get_options ("energy").size();
  total = get_options ("total").size();
  mean = get_options ("mean").size();
  range = get_options ("range").size();


  if (!(bipolar | unipolar | shfit | symmetry))
    bipolar = unipolar = shfit = symmetry = true;

  if (!(nearest_neighour | energy))
    nearest_neighour = energy = true;

  if (!(total | mean | range))
    total = mean = range = true;

  Eigen::MatrixXd directions;

  try {
    directions = DWI::Directions::load_cartesian (argument[0]);
  }
  catch (Exception& E) {
    try {
      directions = load_matrix<double> (argument[0]);
    }
    catch (Exception& E) {
      auto header = Header::open (argument[0]);
      directions = DWI::get_valid_DW_scheme (header);
    }
  }

  if (directions.cols() >= 4) {
    auto shells = DWI::Shells (directions).select_shells (false, false, false);
    Eigen::MatrixXd dirs;

    for (size_t n = 0; n < shells.count(); ++n) {
      dirs.resize (shells[n].count(), 3);
      for (size_t idx = 0; idx < shells[n].count(); ++idx) 
        dirs.row (idx) = directions.row (shells[n].get_volumes()[idx]).head (3);

      report (std::string (argument[0]) + " (b=" + str(shells[n].get_mean()) + ")", dirs);
    }

  }
  else 
    report (argument[0], directions);
}













void report (const std::string& title, Eigen::MatrixXd& directions)
{
  if (directions.cols() < 3) 
    throw Exception ("unexpected matrix size for DW scheme \"" + str(argument[0]) + "\"");
  DWI::normalise_grad (directions);

  if (log_level)
    print (title + " [ " + str(directions.rows(), precision) + " directions ]\n");


  if ((bipolar | unipolar) && (energy | nearest_neighour)) {
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

        double E = 1.0 / (directions.row(i).head(3) - directions.row(j).head(3)).norm();

        E_unipolar[i] += E;
        E_unipolar[j] += E;

        E += 1.0 / (directions.row(i).head(3) + directions.row(j).head(3)).norm();

        E_bipolar[i] += E;
        E_bipolar[j] += E;

      }
    }




    auto report_NN = [](const vector<double>& NN) {
      double NN_min = std::numeric_limits<double>::max();
      double NN_mean = 0.0;
      double NN_max = 0.0;
      for (auto a : NN) {
        a = (180.0/Math::pi) * std::acos (a);
        NN_mean += a;
        NN_min = std::min (NN_min, a);
        NN_max = std::max (NN_max, a);
      }
      NN_mean /= NN.size();

      std::string output;

      if (log_level) 
        output = "    nearest-neighbour angles: ";

      if (mean) {
        if (log_level) 
          output += "mean = ";
        output += str(NN_mean, precision);
      }

      if (range) {
        if (mean)
          output += log_level ? ", " : " ";
        if (log_level) 
          output += "range [ " + str(NN_min, precision) + " - " + str(NN_max, precision) + " ]";
        else 
          output += str(NN_min, precision) + " " + str(NN_max, precision);
      }

      if (output.size())
        print (output + "\n");
    };



    auto report_E = [](const vector<double>& E) {
      double E_min = std::numeric_limits<double>::max();
      double E_total = 0.0;
      double E_max = 0.0;
      for (auto e : E) {
        E_total += e;
        E_min = std::min (E_min, e);
        E_max = std::max (E_max, e);
      }


      std::string output;

      if (log_level) 
        output = "    energy: ";

      if (total) {
        if (log_level) 
          output += "total = ";
        output += str(0.5*E_total, precision);
      }

      if (mean) {
        if (total)
          output += log_level ? ", " : " ";
        if (log_level) 
          output += "mean = ";
        output += str(E_total/E.size(), precision);
      }

      if (range) {
        if (mean | total)
          output += log_level ? ", " : " ";
        if (log_level) 
          output += "range [ " + str(E_min, precision) + " - " + str(E_max, precision) + " ]";
        else 
          output += str(E_min, precision) + " " + str(E_max, precision);
      }

      if (output.size())
        print (output + "\n");
    };






    if (bipolar) {
      if (log_level)
        print ("\n  Bipolar electrostatic repulsion model:\n");
      if (nearest_neighour)
        report_NN (NN_bipolar);
      if (energy) 
        report_E (E_bipolar);
    }

    if (unipolar) {
      if (log_level)
        print ("\n  Unipolar electrostatic repulsion model:\n");
      if (nearest_neighour)
        report_NN (NN_unipolar);
      if (energy)
        report_E (E_unipolar);
    }

  }


  if (shfit) {
    std::string lmax_results;
    for (size_t lmax = 2; lmax <= Math::SH::LforN (directions.rows()); lmax += 2) 
      lmax_results += str(DWI::condition_number_for_lmax (directions, lmax), precision) + " ";
    if (log_level) 
      print ("\n  Spherical Harmonic fit:\n    condition numbers for lmax = " + str(2) + " -> " 
        + str(Math::SH::LforN (directions.rows()), precision) + ": ");
    print (lmax_results + "\n");
  }

  if (symmetry) {
    if (log_level)
      print ("\n  Symmetry of sampling:\n    norm of mean direction vector = ");
    auto symmetry_index = directions.leftCols(3).colwise().mean().norm();
    print (str (symmetry_index, precision) + "\n");
    if (log_level && symmetry_index >= 0.1)
        print (std::string ("    WARNING: sampling is ") + ( symmetry_index >= 0.4 ? "strongly" : "moderately" ) 
            + " asymmetric - this may affect resiliance to eddy-current distortions");
  }

  if (log_level) 
    print ("\n");
}


