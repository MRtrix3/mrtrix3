#ifndef __file_dicom_quick_scan_h__
#define __file_dicom_quick_scan_h__

#include "mrtrix.h"

namespace MR {
  namespace File {
    namespace Dicom {

      class QuickScan {

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

