/* Copyright (c) 2008-2021 the MRtrix3 contributors.
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

#ifndef __file_dicom_study_h__
#define __file_dicom_study_h__

#include "memory.h"
#include "file/dicom/series.h"

namespace MR {
  namespace File {
    namespace Dicom {

      class Patient;
      class Series;

      class Study : public vector<std::shared_ptr<Series>> { NOMEMALIGN
        public:
          Study (Patient* parent, const std::string& study_name, const std::string& study_ID = "",
              const std::string& study_date = "", const std::string& study_time = "") :
        patient (parent), name (study_name), ID (study_ID),
        date (study_date), time (study_time) { }

          Patient* patient;
          std::string name, ID, date, time;

          std::shared_ptr<Series> find (const std::string& series_name, size_t series_number, const std::string& image_type,
              const std::string& series_modality = "", const std::string& series_date = "", const std::string& series_time = "");

          bool operator< (const Study& s) const {
            if (date < s.date) return true;
            if (date > s.date) return false;
            if (time < s.time) return true;
            if (time > s.time) return false;
            if (name < s.name) return true;
            if (name > s.name) return false;
            return ID < s.ID;
          }
      };


      std::ostream& operator<< (std::ostream& stream, const Study& item);


    }
  }
}


#endif



