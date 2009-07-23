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


    15-09-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * handle files even when any of the study, series or patient description fields are blank

*/

#include "file/dicom/patient.h"
#include "file/dicom/study.h"
#include "file/dicom/series.h"

namespace MR {
  namespace File {
    namespace Dicom {

      RefPtr<Study> Patient::find (
          const std::string& study_name, 
          const std::string& study_ID, 
          const std::string& study_date, 
          const std::string& study_time)
      {
        bool match;

        for (uint n = 0; n < size(); n++) {
          match = true;
          if (study_name == (*this)[n]->name) {
            if (study_ID.size() && (*this)[n]->ID.size()) 
              if (study_ID != (*this)[n]->ID) match = false;
            if (match) {
              if (study_date.size() && (*this)[n]->date.size())
                if (study_date != (*this)[n]->date) match = false;
            }
            if (match) {
              if (study_time.size() && (*this)[n]->time.size()) 
                if (study_time != (*this)[n]->time) match = false;
            }
            if (match) return ((*this)[n]);
          }
        }

        push_back (RefPtr<Study> (new Study (this, study_name, study_ID, study_date, study_time)));
        return (back());
      }






      std::ostream& operator<< (std::ostream& stream, const Patient& item)
      {
        stream << MR::printf ("  %-30s %-16s %10s\n", 
            item.name.c_str(), 
            format_ID (item.ID).c_str(), 
            format_date (item.DOB).c_str());

        for (uint n = 0; n < item.size(); n++) stream << (*item[n]);
        return (stream);
      }

    }
  }
}


