/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert E. Smith, 25/01/13.


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

#include <fstream>
#include <cstdio>

#include "app.h"
#include "progressbar.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"


MRTRIX_APPLICATION

using namespace MR;
using namespace MR::DWI;
using namespace MR::DWI::Tractography;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "merge streamlines from multiple input files into a single output .tck file.";

  ARGUMENTS
  + Argument ("in_tracks",  "the input track files").type_file().allow_multiple()
  + Argument ("out_tracks", "the output track file").type_file();

}




void run ()
{

  const size_t in_count = argument.size() - 1;

  Tractography::Properties mean_properties;
  size_t count = 0, total_count = 0;

  for (size_t file_index = 0; file_index != in_count; ++file_index) {

    Properties p;
    Tractography::Reader<float> reader (argument[file_index], p);

    for (std::vector<std::string>::const_iterator i = p.comments.begin(); i != p.comments.end(); ++i) {
      bool present = false;
      for (std::vector<std::string>::const_iterator j = mean_properties.comments.begin(); !present && j != mean_properties.comments.end(); ++j)
        present = (*i == *j);
      if (!present)
        mean_properties.comments.push_back (*i);
    }
    for (std::multimap<std::string, std::string>::const_iterator i = p.roi.begin(); i != p.roi.end(); ++i) {
      bool present = false;
      for (std::multimap<std::string, std::string>::const_iterator j = mean_properties.roi.begin(); !present && j != mean_properties.roi.end(); ++j)
        present = ((i->first == j->first) && (i->second == j->second));
      if (!present)
        mean_properties.roi.insert (*i);
    }

    size_t this_count = 0, this_total_count = 0;

    for (Properties::const_iterator i = p.begin(); i != p.end(); ++i) {
      if (i->first == "count") {
        this_count = to<float>(i->second);
      } else if (i->first == "total_count") {
        this_total_count += to<float>(i->second);
      } else {
        Properties::iterator existing = mean_properties.find (i->first);
        if (existing == mean_properties.end())
          mean_properties.insert (*i);
        else if (i->second != existing->second)
          existing->second = "variable";
      }
    }

    count += this_count;
    total_count += this_total_count;

  }

  DEBUG ("estimated number of tracks: " + str(count));
  DEBUG ("estimated total count: " + str(total_count));

  Tractography::Writer<float> writer (argument[in_count], mean_properties);
  VAR (writer.count);
  VAR (writer.total_count);
  std::vector< Point<float> > tck;

  ProgressBar progress ("concatenating track files...", in_count);
  for (size_t file_index = 0; file_index != in_count; ++file_index) {
    DEBUG ("current value of writer.count: " + str(writer.count));
    Properties p;
    Tractography::Reader<float> reader (argument[file_index], p);
    size_t this_count = 0;
    while (reader.next (tck)) {
      writer.append (tck);
      ++this_count;
    }
    DEBUG ("streamlines read from file \"" + str(argument[file_index]) + "\": " + str(this_count));
    reader.close();
    ++progress;
  }
  writer.total_count = std::max (writer.total_count, total_count);

  DEBUG ("actual number of streamlines written was " + str(writer.count));

}

