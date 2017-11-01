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

  DESCRIPTION
    + "This command will accept as inputs:"
    + "- directions file in spherical coordinates (ASCII text, [ az el ] space-separated values, one per line);"
    + "- directions file in Cartesian coordinates (ASCII text, [ x y z ] space-separated values, one per line);"
    + "- DW gradient files (MRtrix format: ASCII text, [ x y z b ] space-separated values, one per line);"
    + "- image files, using the DW gradient scheme found in the header (or provided using the appropriate command line options below)."

    + "By default, this produces all relevant metrics for the direction set "
    "provided. If the direction set contains multiple shells, metrics are "
    "provided for each shell separately."

    + "Metrics are produced assuming a unipolar or bipolar electrostatic "
    "repulsion model, producing the potential energy (total, mean, min & max), "
    "and the nearest-neighbour angles (mean, min & max). The condition "
    "number is also produced for the spherical harmonic fits up to the highest "
    "harmonic order supported by the number of volumes. Finally, the norm of the "
    "mean direction vector is provided as a measure of the overall symmetry of "
    "the direction set (important with respect to eddy-current resilience)."

    + "Specific metrics can also be queried independently via the \"-output\" "
    "option, using these shorthands: U/B for unipolar/bipolar model, E/N "
    "for energy and nearest-neighbour respectively, t/-/+ for total/min/max "
    "respectively (mean implied otherwise); SHn for condition number of SH fit "
    "at order n (with n an even integer); ASYM for asymmetry index (norm of "
    "mean direction vector); and N for the number of directions. For example:"
    + "-output BN,BN-,BN+   requests the mean, min and max nearest-neighour "
    "angles assuming a bipolar model."
    + "-output UE,SH8,SYM   requests the mean unipolar electrostatic energy, "
    "condition number of SH fit at order 8, and the asymmetry index.";

  ARGUMENTS
    + Argument ("dirs", "the text file or image containing the directions.").type_file_in();

  OPTIONS
    + Option ("output", "output selected metrics as a space-delimited list, "
        "suitable for use in scripts. This will produce one line of values per "
        "selected shell. Valid metrics are as specified in the description "
        "above.")
    +   Argument ("list")
    + DWI::ShellsOption
    + DWI::GradImportOptions();
}



int precision = 6;

void report (const std::string& title, Eigen::MatrixXd& directions);



void run ()
{
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
    int n_start = 0;
    auto shells = DWI::Shells (directions).select_shells (false, false, false);
    if (get_options ("shells").empty() && shells.has_bzero() && shells.count() > 1) {
      n_start = 1;
      if (get_options("output").empty())
        print (std::string (argument[0]) + " (b=0) [ " + str(shells.smallest().count(), precision) + " volumes ]\n\n");
    }


    Eigen::MatrixXd dirs;

    for (size_t n = n_start; n < shells.count(); ++n) {
      dirs.resize (shells[n].count(), 3);
      for (size_t idx = 0; idx < shells[n].count(); ++idx)
        dirs.row (idx) = directions.row (shells[n].get_volumes()[idx]).head (3);

      report (std::string (argument[0]) + " (b=" + str(shells[n].get_mean()) + ")", dirs);
    }

  }
  else
    report (argument[0], directions);
}





vector<default_type> summarise_NN (const vector<double>& NN)
{
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

  return { NN_mean, NN_min, NN_max };
}







vector<default_type> summarise_E (const vector<double>& E)
{
  double E_min = std::numeric_limits<double>::max();
  double E_total = 0.0;
  double E_max = 0.0;
  for (auto e : E) {
    E_total += e;
    E_min = std::min (E_min, e);
    E_max = std::max (E_max, e);
  }

  return { 0.5*E_total, E_total/E.size(), E_min, E_max };
}



class Metrics {
  public:
    vector<default_type> BN, UN, BE, UE, SH;
    default_type ASYM;
    size_t ndirs;
};






Metrics compute (Eigen::MatrixXd& directions)
{
  if (directions.cols() < 3)
    throw Exception ("unexpected matrix size for scheme \"" + str(argument[0]) + "\"");
  DWI::normalise_grad (directions);

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

  Metrics metrics;
  metrics.ndirs = directions.rows();
  metrics.UN = summarise_NN (NN_unipolar);
  metrics.BN = summarise_NN (NN_bipolar);
  metrics.UE = summarise_E (E_unipolar);
  metrics.BE = summarise_E (E_bipolar);

  for (size_t lmax = 2; lmax <= Math::SH::LforN (directions.rows()); lmax += 2)
    metrics.SH.push_back (DWI::condition_number_for_lmax (directions, lmax));

  metrics.ASYM = directions.leftCols(3).colwise().mean().norm();

  return metrics;
}




void output_selected (const Metrics& metrics, const std::string& selection)
{
  auto select = split (selection, ", \t\n", true);

  for (const auto& x : select) {
    const auto xl = lowercase(x);
    if (xl == "uet")       std::cout << metrics.UE[0] << " ";
    else if (xl == "ue")   std::cout << metrics.UE[1] << " ";
    else if (xl == "ue-")  std::cout << metrics.UE[2] << " ";
    else if (xl == "ue+")  std::cout << metrics.UE[3] << " ";
    else if (xl == "bet")  std::cout << metrics.BE[0] << " ";
    else if (xl == "be")   std::cout << metrics.BE[1] << " ";
    else if (xl == "be-")  std::cout << metrics.BE[2] << " ";
    else if (xl == "be+")  std::cout << metrics.BE[3] << " ";
    else if (xl == "un")   std::cout << metrics.UN[0] << " ";
    else if (xl == "un-")  std::cout << metrics.UN[1] << " ";
    else if (xl == "un+")  std::cout << metrics.UN[2] << " ";
    else if (xl == "bn")   std::cout << metrics.BN[0] << " ";
    else if (xl == "bn-")  std::cout << metrics.BN[1] << " ";
    else if (xl == "bn+")  std::cout << metrics.BN[2] << " ";
    else if (xl == "asym") std::cout << metrics.ASYM << " ";
    else if (xl == "n")    std::cout << metrics.ndirs << " ";
    else if (xl.substr(0,2) == "sh") {
      size_t order = to<size_t>(x.substr(2));
      if (order & 1U || order < 2)
        throw Exception ("spherical harmonic order must be an even positive integer");
      order = (order/2)-1;
      if (order >= metrics.SH.size())
        throw Exception ("spherical harmonic order requested is too large given number of directions");
      std::cout << metrics.SH[order] << " ";
    }
    else
      throw Exception ("unknown output specifier \"" + x + "\"");
  }

  std::cout << "\n";
}



void report (const std::string& title, Eigen::MatrixXd& directions)
{
  auto metrics = compute (directions);

  auto opt = get_options ("output");
  if (opt.size()) {
    output_selected (metrics, opt[0][0]);
    return;
  }

  std::string output = title + " [ " + str(metrics.ndirs, precision) + " directions ]\n";

  output += "\n  Bipolar electrostatic repulsion model:\n";
  output += "    nearest-neighbour angles: mean = " + str(metrics.BN[0], precision) + ", range [ " + str(metrics.BN[1], precision) + " - " + str(metrics.BN[2], precision) + " ]\n";
  output += "    energy: total = " + str(metrics.BE[0], precision) + ", mean = " + str(metrics.BE[1], precision) + ", range [ " + str(metrics.BE[2], precision) + " - " + str(metrics.BE[3], precision) + " ]\n";

  output += "\n  Unipolar electrostatic repulsion model:\n";
  output += "    nearest-neighbour angles: mean = " + str(metrics.UN[0], precision) + ", range [ " + str(metrics.UN[1], precision) + " - " + str(metrics.UN[2], precision) + " ]\n";
  output += "    energy: total = " + str(metrics.UE[0], precision) + ", mean = " + str(metrics.UE[1], precision) + ", range [ " + str(metrics.UE[2], precision) + " - " + str(metrics.UE[3], precision) + " ]\n";


  output += "\n  Spherical Harmonic fit:\n    condition numbers for lmax = 2 -> " + str(metrics.SH.size()*2) + ": " + str(metrics.SH, precision) + "\n";

  output += "\n  Asymmetry of sampling:\n    norm of mean direction vector = " + str(metrics.ASYM, precision) + "\n";
  if (metrics.ASYM >= 0.1)
    output += std::string("    WARNING: sampling is ") + ( metrics.ASYM >= 0.4 ? "strongly" : "moderately" )
            + " asymmetric - this may affect resiliance to eddy-current distortions\n";

  output += "\n";
  print (output);
}

