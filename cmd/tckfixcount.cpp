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

#include "command.h"
#include "progressbar.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "file/key_value.h"



using namespace MR;
using namespace MR::DWI;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "fix a streamlines .tck file where the 'count' field has not been set correctly.";

  ARGUMENTS
  + Argument ("tracks", "the input track file.").type_file ();


}


class KeyValue : public File::KeyValue
{
  public:
    KeyValue (const std::string& file, const char* first_line = NULL)
    {
      open (file, first_line);
    }
    int64_t tellg() { return in.tellg(); }
};



void run ()
{

  const std::string path = argument[0];

  Tractography::Properties properties;
  Tractography::Reader<float> reader (path, properties);

  size_t init_count = 0;
  if (properties.find ("count") == properties.end()) {
    DEBUG ("\"count\" field not set in file " + path);
  } else {
    init_count = to<size_t>(properties["count"]);
    DEBUG ("Value of \"count\" in file " + path + " is " + str(init_count));
  }

  // Read the actual number of streamlines in the file
  Tractography::Streamline<float> tck;
  size_t count = 0;
  {
    ProgressBar progress ("evaluating actual streamline data count...");
    while (reader (tck)) {
      ++count;
      ++progress;
    }
  }
  reader.close();
  DEBUG ("Actual number of streamlines read is " + str(count));

  // Find the appropriate file offset in the header
  KeyValue kv (argument[0], "mrtrix tracks");
  std::string data_file;
  int64_t count_offset = 0;
  int64_t current_offset = 0;
  while (kv.next()) {
    std::string key = lowercase (kv.key());
    if (key == "count")
      count_offset = current_offset;
    current_offset = kv.tellg();
  }
  kv.close();

  if (!count_offset)
    throw Exception ("could not find location of 'count' field in file \"" + path + "\"");
  DEBUG ("File offset for 'count' field is " + str(count_offset));

  // Overwrite this data
  // Used C-style file access here as ofstream would either wipe the file contents, or
  //   append the data instead of inserting it
  FILE* fp;
  fp = fopen (path.c_str(), "r+");
  if (!fp)
    throw Exception ("could not open tracks file \"" + path + "\" for repair");
  if (fseek (fp, count_offset, SEEK_SET)) {
    fclose (fp);
    throw Exception ("unable to seek to the appropriate position in the file");
  }
  const std::string string = "count: " + str(count) + "\ntotal_count: " + str(count) + "\nEND\n";
  const int rvalue = fputs (string.c_str(), fp);
  fclose (fp);
  if (rvalue == EOF)
    WARN ("'count' field may not have been properly updated");

}

