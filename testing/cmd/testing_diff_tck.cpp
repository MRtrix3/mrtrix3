/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include <limits>

#include "command.h"
#include "types.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"

#define DEFAULT_HAUSDORFF 1e-5
#define DEFAULT_MAXFAIL 0

using namespace MR;
using namespace App;

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Compare two track files for differences, within specified tolerance";

  DESCRIPTION
  + "This uses the symmetric Hausdorff distance to compare streamline pairs. For each "
    "streamline in the first input, the distance to the corresponding streamline in the "
    "second file is used and compared to the tolerance."

  + "If probabilistic streamlines tractography is to be tested, provide a larger file "
    "as the reference second input (streamlines from first file are matched to the second "
    "file, but not the other way around), and use the -unordered option."

  + "If the streamlines are not guaranteed to be provided in the same order between the "
    "two files, using the -unordered option will result in, for every streamline in the "
    "first input file, comparison against every streamline in the second file, with the "
    "distance to the nearest streamline in the second file compared against the threshold.";

  ARGUMENTS
  + Argument ("tck1", "the file from which all tracks will be checked.").type_file_in ()
  + Argument ("tck2", "the reference track file").type_file_in ();

  OPTIONS
  + Option ("distance", "maximum permissible Hausdorff distance in mm (default: " + str(DEFAULT_HAUSDORFF) + "mm)")
    + Argument ("value").type_float (0.0)

  + Option ("maxfail", "the maximum number of streamlines permitted to exceed the "
                       "Hausdorff distance before the unit test will fail (default: " + str(DEFAULT_MAXFAIL) + ")")

  + Option ("unordered", "compare the streamlines in an unordered fashion; "
            " i.e. compare every streamline in the first file to all streamlines in the second file");
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
    if (within_haussdorf (tck, tck2, tol))
      return true;
  }
  return false;
}



void run ()
{
  const size_t maxfail = get_option_value ("maxfail", DEFAULT_MAXFAIL);
  const float tol = get_option_value ("distance", DEFAULT_HAUSDORFF);

  size_t mismatch_count = 0;

  DWI::Tractography::Properties properties1, properties2;
  DWI::Tractography::Reader<> reader1 (argument[0], properties1);
  DWI::Tractography::Reader<> reader2 (argument[1], properties2);

  if (get_options ("unordered").size()) {

    vector<DWI::Tractography::Streamline<>> ref_list;
    DWI::Tractography::Streamline<> tck;

    while (reader2 (tck))
      ref_list.push_back (tck);

    while (reader1 (tck))
      if (!within_haussdorf (tck, ref_list, tol))
        ++mismatch_count;

  } else {

    DWI::Tractography::Streamline<> tck1, tck2;

    while (reader1 (tck1)) {
      if (!reader2 (tck2))
        throw Exception ("More streamlines in first file than second file");
      if (!within_haussdorf (tck1, tck2, tol))
        ++mismatch_count;
    }

    if (reader2 (tck2))
      throw Exception ("More streamlines in second file than first file");

  }

  if (mismatch_count > maxfail)
    throw Exception (str(mismatch_count) + " mismatched streamlines - test FAILED");

  CONSOLE (str(mismatch_count) + " mismatched streamlines - data checked OK");
}
