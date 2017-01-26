/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __file_dicom_quick_scan_h__
#define __file_dicom_quick_scan_h__

#include "mrtrix.h"

namespace MR {
  namespace File {
    namespace Dicom {

      class QuickScan { NOMEMALIGN

        public:
          bool read (const std::string& file_name, bool print_DICOM_fields = false, bool print_CSA_fields = false, bool force_read = false);

          std::string filename, modality;
          std::string patient, patient_ID, patient_DOB;
          std::string study, study_ID, study_date, study_time;
          std::string series, series_date, series_time, sequence;
          size_t series_number, bits_alloc, dim[2], data;
      };

      std::ostream& operator<< (std::ostream& stream, const QuickScan& file);

    }
  }
}

#endif

