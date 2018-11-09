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
#include "progressbar.h"
#include "datatype.h"
#include "dwi/tractography/file.h"

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Compare two track files for differences, within specified tolerance";

  DESCRIPTION
  + "This uses the symmetric Hausdorff distance to compare streamlines. For each "
    "streamline in the first input, the distance to the closest match in the second "
    "file is used and compared to the tolerance. The test succeeds if fewer "
    "than 10 streamlines fail."
  + "To give this test the best chance of success, especially with multi-threading "
    "or probabilistic approaches, provide a larger file as the reference second input";

  ARGUMENTS
  + Argument ("tck1", "a track file.").type_file_in ()
  + Argument ("tck2", "another track file.").type_file_in ()
  + Argument ("tolerance", "the maximum distance to consider acceptable").type_float (0.0);
}


inline bool within_haussdorf (const DWI::Tractography::Streamline<>& tck1, const DWI::Tractography::Streamline<>& tck2, float tol) 
{
  tol *= tol;
  for (const auto& a : tck1) {
    float distance = std::numeric_limits<float>::max();
    for (const auto& b : tck2)
      distance = std::min (distance, (a - b).squaredNorm());
    if (distance > tol)
      return false;
  }
  return true;
}

inline bool within_haussdorf (const DWI::Tractography::Streamline<>& tck, const vector<DWI::Tractography::Streamline<>>& list, float tol)
{
  for (auto& tck2 : list) {
    if (within_haussdorf (tck, tck2, tol) || within_haussdorf (tck2, tck, tol)) 
      return true;
  }
  return false;
}



void run ()
{
  size_t mismatch_count = 0;
  float tol = argument[2];

  DWI::Tractography::Properties properties1, properties2;
  vector<DWI::Tractography::Streamline<>> ref_list;
  DWI::Tractography::Reader<> reader1 (argument[0], properties1);
  DWI::Tractography::Reader<> reader2 (argument[1], properties2);

  DWI::Tractography::Streamline<> tck;
  while (reader2 (tck))
    ref_list.push_back (tck);

  while (reader1 (tck)) 
    if (!within_haussdorf (tck, ref_list, tol))
      ++mismatch_count;

  if (mismatch_count > 10)
    throw Exception (str(mismatch_count) + " mismatched streamlines - test FAILED");

  CONSOLE (str(mismatch_count) + " mismatched streamlines - data checked OK");
}

