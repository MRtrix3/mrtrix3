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


#include "file/dicom/patient.h"
#include "file/dicom/study.h"
#include "file/dicom/series.h"

namespace MR {
  namespace File {
    namespace Dicom {

      namespace {
        bool series_time_mismatch_warning_issued = false;
      }

      std::shared_ptr<Series> Study::find (const std::string& series_name, size_t series_number, const std::string& image_type,
          const std::string& series_modality, const std::string& series_date, const std::string& series_time)
      {
        for (size_t n = 0; n < size(); n++) {
          bool match = true;
          if (series_name == (*this)[n]->name) {
            if (series_number == (*this)[n]->number) {
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
              if (match && !series_time_mismatch_warning_issued) {
                float stime = NaN, stime_ref = NaN;
                try {
                  stime = to<float> (series_time);
                  stime_ref = to<float> ((*this)[n]->time);
                }
                catch (Exception& E) {
                  INFO ("error reading DICOM series time - field does not exist or is empty?");
                }
                if (stime != stime_ref) {
                  INFO ("WARNING: series times do not match - this may cause problem with series grouping");
                  series_time_mismatch_warning_issued = true;
                }
              }
              if (match)
                return (*this)[n];
            }
          }
        }

        push_back (std::shared_ptr<Series> (new Series (this, series_name, series_number, image_type, series_modality, series_date, series_time)));
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


