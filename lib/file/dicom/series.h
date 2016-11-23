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

#ifndef __file_dicom_series_h__
#define __file_dicom_series_h__

#include "memory.h"
#include "progressbar.h"
#include "file/dicom/image.h"

namespace MR {
  namespace File {
    namespace Dicom {

      class Study;
      class Image;

      class Series : public std::vector<std::shared_ptr<Image>> { NOMEMALIGN
        public:
          Series (Study* parent, const std::string& series_name, size_t series_number,
              const std::string& series_modality = "", const std::string& series_date = "", const std::string& series_time = "") :
            study (parent), name (series_name), modality (series_modality),
            date (series_date), time (series_time) { 
              number = series_number; 
            }

          Study* study;
          std::string name;
          size_t number;
          std::string modality;
          std::string date;
          std::string time;

          void read () {
            ProgressBar progress ("reading DICOM series \"" + name + "\"", size());
            for (size_t i = 0; i < size(); i++) {
              (*this)[i]->read(); 
              ++progress;
            }
          }

          std::vector<int> count () const;
          bool operator< (const Series& s) const {
            return number < s.number;
          }

          friend std::ostream& operator<< (std::ostream& stream, const Series& item);
      };


    }
  }
}


#endif



