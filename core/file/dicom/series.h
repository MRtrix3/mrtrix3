/* Copyright (c) 2008-2019 the MRtrix3 contributors.
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

      class Series : public vector<std::shared_ptr<Image>> { NOMEMALIGN
        public:
          Series (Study* parent, const std::string& series_name, size_t series_number, const std::string& image_type,
              const std::string& series_modality = "", const std::string& series_date = "", const std::string& series_time = "") :
            study (parent), name (series_name), image_type (image_type), modality (series_modality),
            date (series_date), time (series_time) { 
              number = series_number; 
            }

          Study* study;
          std::string name, image_type;
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

          vector<int> count () const;
          bool operator< (const Series& s) const {
            return number < s.number;
          }

          friend std::ostream& operator<< (std::ostream& stream, const Series& item);
      };


    }
  }
}


#endif



