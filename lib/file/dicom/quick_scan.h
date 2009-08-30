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

#ifndef __file_dicom_quick_scan_h__
#define __file_dicom_quick_scan_h__

#include "mrtrix.h"

namespace MR {
  namespace File {
    namespace Dicom {

      class QuickScan {

        public:
          /*! \todo could exclude Siemens MPR info images by splitting the
           * series based on entry (0x0008U, 0x0008U). */
          bool read (const std::string& file_name, bool print_DICOM_fields = false, bool print_CSA_fields = false);

          std::string      filename, modality;
          std::string      patient, patient_ID, patient_DOB;
          std::string      study, study_ID, study_date, study_time;
          std::string      series, series_date, series_time;
          std::string      sequence;
          size_t           series_number, bits_alloc, dim[2], data;
      };

      std::ostream& operator<< (std::ostream& stream, const QuickScan& file);

    }
  }
}

#endif

