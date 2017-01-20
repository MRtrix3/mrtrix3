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
#include "datatype.h"
#include "dwi/tractography/scalar_file.h"

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "David Raffelt (david.raffelt@florey.edu.au)";

  DESCRIPTION
  + "compare two track scalar files for differences, optionally with a specified tolerance.";

  ARGUMENTS
  + Argument ("tsf1", "a track scalar file.").type_file_in()
  + Argument ("tsf2", "another track scalar file.").type_file_in();
  
  OPTIONS
  + Option ("abs", "specify an absolute tolerance") 
    + Argument ("tolerance").type_float (0.0)
  + Option ("frac", "specify a fractional tolerance") 
    + Argument ("tolerance").type_float (0.0);
}

typedef float value_type;

void run ()
{
  DWI::Tractography::Properties properties1, properties2;
  DWI::Tractography::ScalarReader<value_type> reader1 (argument[0], properties1);
  DWI::Tractography::ScalarReader<value_type> reader2 (argument[1], properties2);

  DWI::Tractography::check_properties_match (properties1, properties2, "scalar", false);

  vector<value_type> tck_scalar1;
  vector<value_type> tck_scalar2;
  
  auto opt = get_options ("frac");
  if (opt.size()) {
  
    const double tol = opt[0][0];
  
    while (reader1 (tck_scalar1)) {
      if (!reader2 (tck_scalar2))
        throw Exception ("readable number of tracks mismatch - test FAILED");
      if (tck_scalar1.size() != tck_scalar2.size())
        throw Exception ("track scalar length mismatch - test FAILED");

      for (size_t i = 0; i < tck_scalar1.size(); ++i) {
        if (std::abs ((tck_scalar1[i] - tck_scalar2[i]) / (0.5 * (tck_scalar1[i] + tck_scalar2[i]))) > tol)
          throw Exception ("track scalar files \"" + str(argument[0]) + "\" and \"" + str(argument[1]) + "\" do not match within fractional precision of " + str(tol)
                           + " (" + str(cdouble (tck_scalar1[i])) + " vs " + str(cdouble (tck_scalar2[i])) + ")");
      }
    }
    
  } else {
  
    double tol = 0.0;
    opt = get_options ("abs");
    if (opt.size())
      tol = opt[0][0];
  
    while (reader1 (tck_scalar1)) {
      if (!reader2 (tck_scalar2))
        throw Exception ("readable number of tracks mismatch - test FAILED");
      if (tck_scalar1.size() != tck_scalar2.size())
        throw Exception ("track scalar length mismatch - test FAILED");

      for (size_t i = 0; i < tck_scalar1.size(); ++i) {
        if (std::abs (tck_scalar1[i] - tck_scalar2[i]) > tol)
          throw Exception ("track scalar files \"" + str(argument[0]) + "\" and \"" + str(argument[1]) + "\" do not match within absolute precision of " + str(tol)
                           + " (" + str(cdouble (tck_scalar1[i])) + " vs " + str(cdouble (tck_scalar2[i])) + ")");
      }
    }
  
  }
  
  
  CONSOLE ("track scalar files checked OK");
}

