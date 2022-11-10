/* Copyright (c) 2008-2022 the MRtrix3 contributors.
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

#include "file/dicom/patient.h"
#include "file/dicom/study.h"
#include "file/dicom/series.h"

namespace MR {
  namespace File {
    namespace Dicom {

      namespace {
        bool mismatched_UID_time_warning_issued = false;
      }

      std::shared_ptr<Study> Patient::find (const std::string& study_name, const std::string& study_ID,
          const std::string& study_UID, const std::string& study_date, const std::string& study_time)
      {
        for (size_t n = 0; n < size(); n++) {
          bool match = true;
          if (study_name == (*this)[n]->name) {
            if (study_ID.size() && (*this)[n]->ID.size())
              if (study_ID != (*this)[n]->ID)
                match = false;
            if (match) {
              if (study_date.size() && (*this)[n]->date.size())
                if (study_date != (*this)[n]->date)
                  match = false;
            }
            if (match) {
              if (study_time.size() && (*this)[n]->time.size())
                if (study_time != (*this)[n]->time)
                  match = false;
            }
            if (!match) {
              if (study_UID.size() && (*this)[n]->UID.size()) {
                if (study_UID == (*this)[n]->UID) {
                  if (!mismatched_UID_time_warning_issued) {
                    WARN ("mismatched study time and UID - this may cause problems with series grouping");
                    mismatched_UID_time_warning_issued = true;
                  }
                  match = true;
                }
              }
            }
            if (match)
              return (*this)[n];
          }
        }

        push_back (std::shared_ptr<Study> (new Study (this, study_name, study_ID, study_UID, study_date, study_time)));
        return back();
      }






      std::ostream& operator<< (std::ostream& stream, const Patient& item)
      {
        stream << MR::printf ("  %-30s %-16s %10s\n",
            item.name.c_str(),
            format_ID (item.ID).c_str(),
            format_date (item.DOB).c_str());

        for (size_t n = 0; n < item.size(); n++)
          stream << *item[n];

        return stream;
      }

    }
  }
}


