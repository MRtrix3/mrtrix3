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
        bool series_time_mismatch_warning_issued = false;
        bool series_number_UID_mismatch_warning_issued = false;
      }

      std::shared_ptr<Series> Study::find (const std::string& series_name, size_t series_number,
          const std::string& image_type, const std::string& series_ref_UID,
          const std::string& series_modality, const std::string& series_date, const std::string& series_time)
      {
        for (size_t n = 0; n < size(); n++) {
          bool match = true;
          if (series_name == (*this)[n]->name) {
            match = (series_number == (*this)[n]->number);
            if (!match && series_ref_UID.size() && (*this)[n]->series_ref_UID.size()) {
              if (series_ref_UID == (*this)[n]->series_ref_UID) {
                if (!series_number_UID_mismatch_warning_issued) {
                  series_number_UID_mismatch_warning_issued = true;
                  WARN ("mismatched series number and UID - this may cause problems with series grouping");
                }
                match = true;
              }
            }

            if (match) {
              if (image_type != (*this)[n]->image_type)
                match = false;
              if (series_modality.size() && (*this)[n]->modality.size())
                if (series_modality != (*this)[n]->modality)
                  match = false;
              if (match) {
                if (series_date.size() && (*this)[n]->date.size())
                  if (series_date != (*this)[n]->date)
                    match = false;
              }
              if (match) {
                float stime = NaN, stime_ref = NaN;
                try {
                  stime = to<float> (series_time);
                  stime_ref = to<float> ((*this)[n]->time);
                }
                catch (Exception& E) {
                  INFO ("error reading DICOM series time - field does not exist or is empty?");
                }
                if (stime != stime_ref) {
                  if (!series_time_mismatch_warning_issued) {
                    INFO ("WARNING: series times do not match - this may cause problems with series grouping");
                    series_time_mismatch_warning_issued = true;
                  }
                  if (stime < stime_ref)
                    (*this)[n]->time = series_time;
                }
              }
              if (match)
                return (*this)[n];
            }
          }
        }

        push_back (std::shared_ptr<Series> (new Series (this, series_name, series_number, image_type, series_ref_UID, series_modality, series_date, series_time)));
        return back();
      }













      std::ostream& operator<< (std::ostream& stream, const Study& item)
      {
        stream << MR::printf ("    %-30s %-16s %10s %8s\n",
              item.name.c_str(),
              format_ID(item.ID).c_str(),
              format_date(item.date).c_str(),
              format_time(item.time).c_str() );

        for (size_t n = 0; n < item.size(); n++)
          stream << *item[n];

        return stream;
      }


    }
  }
}


