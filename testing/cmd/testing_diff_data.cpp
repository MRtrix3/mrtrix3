
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
#include "datatype.h"

#include "image.h"
#include "algo/loop.h"
#include "algo/threaded_loop.h"

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com) and David Raffelt (david.raffelt@florey.edu.au) and Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "compare two images for differences, optionally with a specified tolerance.";

  ARGUMENTS
  + Argument ("data1", "an image.").type_image_in()
  + Argument ("data2", "another image.").type_image_in()
  + Argument ("tolerance", "the tolerance value (default = 0.0).").type_float(0.0).optional();
  
  OPTIONS
  + Option ("abs", "test for absolute difference (the default)") 
  + Option ("frac", "test for fractional absolute difference") 
  + Option ("voxel", "test for fractional absolute difference relative to the maximum value in the voxel");
}


void run ()
{
  auto in1 = Image<cdouble>::open (argument[0]);
  auto in2 = Image<cdouble>::open (argument[1]);
  check_dimensions (in1, in2);
  for (size_t i = 0; i < in1.ndim(); ++i) {
    if (std::isfinite (in1.size(i)))
      if (in1.size(i) != in2.size(i))
        throw Exception ("images \"" + in1.name() + "\" and \"" + in2.name() + "\" do not have matching voxel spacings " +
                                       str(in1.size(i)) + " vs " + str(in2.size(i)));
  }
  for (size_t i  = 0; i < 3; ++i) {
    for (size_t j  = 0; j < 4; ++j) {
      if (std::abs (in1.transform().matrix()(i,j) - in2.transform().matrix()(i,j)) > 0.001)
        throw Exception ("images \"" + in1.name() + "\" and \"" + in2.name() + "\" do not have matching header transforms "
                           + "\n" + str(in1.transform().matrix()) + "vs \n " + str(in2.transform().matrix()) + ")");
    }
  }

  const double tol = argument.size() == 3 ? argument[2] : 0.0;

  const bool absolute = get_options ("abs").size();
  const bool fractional = get_options ("frac").size();
  const bool per_voxel = get_options ("voxel").size();

  double largest_diff = 0.0;
  size_t count = 0;

  if (absolute + fractional + per_voxel > 1)
    throw Exception ("options \"-abs\", \"-frac\", and \"-voxel\" are mutually exclusive");


  // per-voxel:
  if (per_voxel) {

    if (in1.ndim() != 4)
      throw Exception ("Option -voxel only works for 4D images");

    struct RunKernel {
      RunKernel (double& largest_diff, size_t& count, double tol) : 
        largest_diff (largest_diff), count (count), tol (tol), max_diff (0.0), n (0) { }

      void operator() (decltype(in1)& a, decltype(in2)& b) {
        double maxa = 0.0, maxb = 0.0;
        for (auto l = Loop(3) (a, b); l; ++l) {
          maxa = std::max (maxa, std::abs (cdouble(a.value())));
          maxb = std::max (maxb, std::abs (cdouble(b.value())));
        }
        const double threshold = tol * 0.5 * (maxa + maxb);
        for (auto l = Loop(3) (a, b); l; ++l) {
          auto diff = std::abs (cdouble (a.value()) - cdouble (b.value()));
          if (diff > threshold) {
            ++n;
            max_diff = std::max (max_diff, diff);
          }
        }
      }

      double& largest_diff;
      size_t& count;
      const double tol;
      double max_diff;
      size_t n;
    };

    ThreadedLoop (in1, 0, 3).run (RunKernel (largest_diff, count, tol), in1, in2);
    if (count)
      throw Exception ("images \"" + in1.name() + "\" and \"" + in2.name() + "\" do not match within " + str(tol) + " of maximal voxel value"
          + " (" + str(count) + " voxels over threshold, maximum per-voxel relative difference = " + str(largest_diff) + ")");
  } 
  else { // frac or abs:

    struct RunKernel { NOMEMALIGN
      RunKernel (double& largest_diff, size_t& count, double tol, bool fractional) : 
        largest_diff (largest_diff), count (count), tol (tol), fractional (fractional), max_diff (0.0), n (0) { }

      ~RunKernel() { largest_diff = std::max (largest_diff, max_diff); count += n; }

      void operator() (const decltype(in1)& a, const decltype(in2)& b) {
        auto diff = std::abs (a.value() - b.value());
        if (fractional)
          diff /= 0.5 * std::abs (a.value() + b.value()); 
        if (diff > tol) {
          ++n;
          max_diff = std::max (max_diff, diff);
        }
      }

      double& largest_diff;
      size_t& count;
      const double tol;
      const bool fractional;
      double max_diff;
      size_t n;
    };

    ThreadedLoop (in1).run (RunKernel (largest_diff, count, tol, fractional), in1, in2);
    if (count)
      throw Exception ("images \"" + in1.name() + "\" and \"" + in2.name() + "\" do not match within " 
          + ( fractional ? "relative" : "absolute" ) + " precision of " + str(tol)
          + " (" + str(count) + " voxels over threshold, maximum absolute difference = " + str(largest_diff) + ")");
  }

  CONSOLE ("data checked OK");
}

