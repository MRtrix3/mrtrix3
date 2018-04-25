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
#include "types.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/scalar_file.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/properties.h"


using namespace MR;
using namespace MR::DWI::Tractography;
using namespace App;


void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  SYNOPSIS = "Validate a track scalar file against the corresponding track data";

  ARGUMENTS
  + Argument ("tsf", "the input track scalar file").type_file_in()
  + Argument ("tracks", "the track file on which the TSF is based").type_file_in();
}


typedef float value_type;


void run ()
{
  Properties tsf_properties, tck_properties;
  ScalarReader<value_type> tsf_reader (argument[0], tsf_properties);
  Reader<value_type> tck_reader (argument[1], tck_properties);
  size_t error_count = 0;

  Properties::const_iterator tsf_count_field = tsf_properties.find ("count");
  Properties::const_iterator tck_count_field = tck_properties.find ("count");
  size_t tsf_header_count = 0, tck_header_count = 0;
  if (tsf_count_field == tsf_properties.end() || tck_count_field == tck_properties.end()) {
    WARN ("Unable to verify equal track counts: \"count\" field absent from file header");
  } else {
    tsf_header_count = to<size_t> (tsf_count_field->second);
    tck_header_count = to<size_t> (tck_count_field->second);
    if (tsf_header_count != tck_header_count) {
      CONSOLE ("\"count\" fields in file headers do not match");
      ++error_count;
    }
  }

  Properties::const_iterator tsf_timestamp_field = tsf_properties.find ("timestamp");
  Properties::const_iterator tck_timestamp_field = tck_properties.find ("timestamp");
  if (tsf_timestamp_field == tsf_properties.end() || tck_timestamp_field == tck_properties.end()) {
    WARN ("Unable to verify equal file timestamps: \"timestamp\" field absent from file header");
  } else if (tsf_timestamp_field->second != tck_timestamp_field->second) {
    CONSOLE ("\"timestamp\" fields in file headers do not match");
    ++error_count;
  }

  Streamline<value_type> track;
  vector<value_type> scalar;
  size_t tck_counter = 0, tsf_counter = 0, length_mismatch_count = 0;

  {
    ProgressBar progress ("Validating track scalar file", tck_header_count);
    while (tck_reader (track)) {
      ++tck_counter;
      if (tsf_reader (scalar)) {
        ++tsf_counter;
        if (track.size() != scalar.size())
          ++length_mismatch_count;
      }
      ++progress;
    }

    while (tsf_reader (scalar)) {
      ++tsf_counter;
      ++progress;
    }
  }

  if (tsf_header_count && tsf_counter != tsf_header_count) {
    CONSOLE ("Actual number of tracks counted in scalar file (" + str(tsf_counter) + ") does not match number reported in header (" + str(tsf_header_count) + ")");
    ++error_count;
  }
  if (tck_header_count && tck_counter != tck_header_count) {
    CONSOLE ("Actual number of tracks counted in track file (" + str(tck_counter) + ") does not match number reported in header (" + str(tck_header_count) + ")");
    ++error_count;
  }
  if (tck_counter != tsf_counter) {
    CONSOLE ("Actual number of tracks counter in scalar file (" + str(tsf_counter) + ") does not match actual number of tracks counted in track file (" + str(tck_counter) + ")");
    ++error_count;
  }
  if (length_mismatch_count) {
    CONSOLE (str(length_mismatch_count) + " track" + (length_mismatch_count == 1 ? " was" : "s were") + " detected with different lengths between track and scalar data");
    ++error_count;
  }
  if (error_count > 1) {
    throw Exception ("Multiple errors detected");
  } else if (error_count) {
    throw Exception ("Error detected");
  } else {
    CONSOLE ("Track scalar file data checked OK");
  }
}
