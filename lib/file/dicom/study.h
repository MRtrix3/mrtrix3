/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
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

          std::shared_ptr<Series> find (const std::string& series_name, size_t series_number,
              const std::string& series_modality = "", const std::string& series_date = "", const std::string& series_time = "");
      };


      std::ostream& operator<< (std::ostream& stream, const Study& item);


    }
  }
}


#endif



