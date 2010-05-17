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

#ifndef __file_dicom_series_h__
#define __file_dicom_series_h__

#include "ptr.h"
#include "progressbar.h"
#include "file/dicom/image.h"

namespace MR {
  namespace File {
    namespace Dicom {

      class Study;
      class Image;

      class Series : public std::vector< RefPtr <Image> > {
        public:
          Series (
              Study*             parent, 
              const std::string& series_name, 
              size_t             series_number,
              const std::string& series_modality = "", 
              const std::string& series_date = "", 
              const std::string& series_time = "") :
            study (parent), name (series_name), modality (series_modality), date (series_date), time (series_time) { number = series_number; }

          Study*       study;
          std::string  name;
          size_t       number;
          std::string  modality;
          std::string  date;
          std::string  time;

          void read () {
            ProgressBar progress ("reading DICOM series \"" + name + "\"...", size());
            for (size_t i = 0; i < size(); i++) {
              (*this)[i]->read();
              ++progress;
            }
          }

          void print_fields (bool dcm, bool csa) const { for (size_t i = 0; i < size(); i++) (*this)[i]->print_fields (dcm, csa); }

          std::vector<int> count () const;

          bool operator< (const Series& s) const;

          friend std::ostream& operator<< (std::ostream& stream, const Series& item);
      };



    }
  }
}


#endif



