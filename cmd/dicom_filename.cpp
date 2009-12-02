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

#include "file/dicom/element.h"
#include "app.h"

using namespace std; 
using namespace MR; 
using namespace File::Dicom; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "read a DICOM file and output a suitable filename for its storage.",
  NULL
};

ARGUMENTS = {
  Argument ("file", "DICOM file", "the DICOM file to be scanned.").type_file (),
  Argument::End
};

OPTIONS = { Option::End };

  


void make_valid (std::string& str, const std::string& alternate) {
  if (str.empty()) { str = alternate; return; }
  str = strip (str);
  std::string::size_type pos = 0;
  while ((pos = str.find_first_of ("^/\\?*", pos)) != std::string::npos)
    str.replace (pos, 1, " ");
}



EXECUTE {
  Element item;
  item.set (argument[0].get_string());

  std::string patient_name, patient_id, study_date, study_name, 
    study_time, series_name, series_number, instance_number, SOP_instance_number;

  while (item.read()) {
    if      (item.is (0x0008U, 0x0020U)) study_date = item.get_string()[0];
    else if (item.is (0x0008U, 0x0018U)) SOP_instance_number = item.get_string()[0];
    else if (item.is (0x0008U, 0x0030U)) study_time = item.get_string()[0];
    else if (item.is (0x0008U, 0x1030U)) study_name = item.get_string()[0];
    else if (item.is (0x0008U, 0x103EU)) series_name = item.get_string()[0];
    else if (item.is (0x0010U, 0x0010U)) patient_name = item.get_string()[0];
    else if (item.is (0x0010U, 0x0020U)) patient_id = item.get_string()[0];
    else if (item.is (0x0020U, 0x0011U)) series_number = item.get_string()[0];
    else if (item.is (0x0020U, 0x0013U)) instance_number = item.get_string()[0];
  }

  if (study_date.empty()) study_date = "nodate";
  else study_date =  study_date.substr(0,4) + "-" + study_date.substr(4,2) + "-" + study_date.substr(6,2);

  if (study_time.empty()) study_time = "notime";
  else study_time = study_time.substr(0,2) + ":" + study_time.substr(2,2);

  make_valid (patient_name, "noname");
  make_valid (patient_id, "-");
  make_valid (study_name, "nodescription");
  make_valid (series_name, "nodescription");
  make_valid (series_number, "?");
  make_valid (SOP_instance_number, "");
  make_valid (instance_number, SOP_instance_number);

  if (instance_number.empty()) throw Exception ("no instance number");

  print (study_date + " - " + patient_name + " (" + patient_id + ")/" 
    + study_time + " - " + study_name + "/"
    + series_number + " - " + series_name + "/"
    + instance_number + ".dcm\n");
}

