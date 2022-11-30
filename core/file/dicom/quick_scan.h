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

#ifndef __file_dicom_quick_scan_h__
#define __file_dicom_quick_scan_h__

#include <map>
#include "mrtrix.h"

namespace MR {
  namespace File {
    namespace Dicom {

      class QuickScan { NOMEMALIGN

        public:
          bool read (const std::string& file_name, bool print_DICOM_fields = false, bool print_CSA_fields = false, bool print_Phoenix = false, bool force_read = false);

          std::string filename, modality;
          std::string patient, patient_ID, patient_DOB;
          std::string study, study_ID, study_UID, study_date, study_time;
          std::string series, series_ref_UID, series_date, series_time, sequence;
          std::map<std::string, size_t> image_type;
          size_t series_number, bits_alloc, dim[2], data;
          bool transfer_syntax_supported;
      };

      std::ostream& operator<< (std::ostream& stream, const QuickScan& file);

    }
  }
}

#endif

